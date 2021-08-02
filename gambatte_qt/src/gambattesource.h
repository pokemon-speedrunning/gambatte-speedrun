//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef GAMBATTESOURCE_H
#define GAMBATTESOURCE_H

#include "mediasource.h"
#include "inputdialog.h"
#include "pixelbuffer.h"
#include "scoped_ptr.h"
#include "videodialog.h"
#include "videolink/videolink.h"
#include <gambatte.h>
#include <pakinfo.h>
#include <QObject>
#include <QTimer>
#include <cstring>
#include <algorithm>
#include <random>

class GambatteSource : public QObject, public MediaSource {
public:
	GambatteSource();
	std::vector<VideoDialog::VideoSourceInfo> const generateVideoSourceInfos();

	gambatte::LoadRes load(std::string const &romfile, unsigned flags) {
		gambatte::LoadRes res = gb_.load(romfile, flags);
	#ifdef ENABLE_INPUT_LOG
		inputLog_.restart(gb_);
	#endif

		setBreakpoint(-1);
		enableBreakpoint(false);

		return res;
	}

	unsigned int loadBios(std::string const &biosfile, std::size_t size, unsigned crc) {
		return gb_.loadBios(biosfile, size, crc);
	}

	void setGameGenie(std::string const &codes) { gb_.setGameGenie(codes); }
	void setGameShark(std::string const &codes) { gb_.setGameShark(codes); }

	void reset(unsigned samplesToStall) {
		gb_.reset(samplesToStall, GSR_VERSION_STR);
	#ifdef ENABLE_INPUT_LOG
		inputLog_.push(0, 0xFF);
	#endif
	}

	void setDmgPaletteColor(int palNum, int colorNum, unsigned long rgb32) {
		gb_.setDmgPaletteColor(palNum, colorNum, rgb32);
	}

	void setSavedir(std::string const &sdir) { gb_.setSaveDir(sdir); }
	void saveSavedata() { gb_.saveSavedata(); }
	void setVideoSource(std::size_t videoSourceIndex);
	bool isCgb() const { return gb_.isCgb(); }
	std::string const romTitle() const { return gb_.romTitle(); }
	gambatte::PakInfo pakInfo() const { return gb_.pakInfo(); }
	void selectState(int n) { gb_.selectState(n); }
	int currentState() const { return gb_.currentState(); }
	void saveState(PixelBuffer const &fb, std::string const &filepath);

	void loadState(std::string const &filepath) {
		gb_.loadState(filepath);
	#ifdef ENABLE_INPUT_LOG
		inputLog_.restart(gb_);
	#endif
	}

	InputDialog * inputDialog() const { return inputDialog_; }
	void saveState(PixelBuffer const &fb);

	void loadState() {
		gb_.loadState();
	#ifdef ENABLE_INPUT_LOG
		inputLog_.restart(gb_);
	#endif
	}

	void tryReset();
	void setResetParams(unsigned fade, unsigned stall);
	std::vector<char> inputLogState() const { return inputLog_.initialState; }
	std::vector<std::pair<std::uint32_t, std::uint8_t>> inputLog() const { return inputLog_.data; }

	void setBreakpoint(int address) { breakpoint_[0] = address; }
	int getBreakpoint() { return breakpoint_[0]; }

	virtual void keyPressEvent(QKeyEvent const *);
	virtual void keyReleaseEvent(QKeyEvent const *);
	virtual void joystickEvent(SDL_Event const &);
	virtual void clearKeyPresses();
	virtual std::ptrdiff_t update(PixelBuffer const &fb, qint16 *soundBuf, std::size_t &samples);
	virtual void generateVideoFrame(PixelBuffer const &fb);

public slots:
	void setTrueColors(bool trueColor) {
		unsigned lut[32768];
		int i = 0;
		if (trueColor) {
			for (int b = 0; b < 32; b++)
				for (int g = 0; g < 32; g++)
					for (int r = 0; r < 32; r++)
						lut[i++] = ((b * 255 + 15) / 31) | (((g * 255 + 15) / 31) << 8) | (((r * 255 + 15) / 31) << 16) | 255 << 24;
		} else {
			for (int b = 0; b < 32; b++)
				for (int g = 0; g < 32; g++)
					for (int r = 0; r < 32; r++)
						lut[i++] = ((r * 3 + g * 2 + b * 11) >> 1) | ((g * 3 + b) << 1) << 8 | ((r * 13 + g * 2 + b) >> 1) << 16 | 255 << 24;
		}
		gb_.setCgbPalette(lut);
	}
	void setTimeMode(bool useCycles) { gb_.setTimeMode(useCycles); }

signals:
	void setTurbo(bool on);
	void togglePause();
	void frameStep();
	void decFrameRate();
	void incFrameRate();
	void resetFrameRate();
	void prevStateSlot();
	void nextStateSlot();
	void saveStateSignal();
	void loadStateSignal();
	void resetSignal();
	void resetting(bool state);
	void quit();

private:
	Q_OBJECT

	struct GbVidBuf;
	struct GetInput {
		unsigned is;
		GetInput() : is(0) {}
		static unsigned get(GetInput *p) { return p->is; }
	};

	struct InputLog {
		std::vector<char> initialState;
		std::vector<std::pair<std::uint32_t, std::uint8_t>> data;

		void restart(gambatte::GB &gb) {
			initialState.resize(gb.saveState(NULL, 0, NULL));
			gb.saveState(NULL, 0, initialState.data());
			data.clear();
		}

		void push(std::uint32_t samples, std::uint8_t input) {
			if (!data.empty() && data.back().second == input) {
				if (data.back().first + samples < 0x80000000) {
					data.back().first += samples;
					return;
				}
			}
			data.push_back({ samples, input });
		}
	};

	enum ResetStage { RESET_NOT, RESET_FADE, RESET_STALL };

	gambatte::GB gb_;
	GetInput inputGetter_;
	InputLog inputLog_;
	InputDialog *const inputDialog_;
	scoped_ptr<VideoLink> cconvert_;
	scoped_ptr<VideoLink> vfilter_;
	PixelBuffer::PixelFormat pxformat_;
	std::size_t vsrci_;
	bool inputState_[10];
	bool dpadUp_, dpadDown_;
	bool dpadLeft_, dpadRight_;
	bool dpadUpLast_, dpadLeftLast_;
	int breakpoint_[1];
	bool tryReset_;
	bool isResetting_;
	ResetStage resetStage_;
	signed resetCounter_;
	unsigned resetFade_;
	unsigned resetStall_;

	std::mt19937 rng_;
	std::uniform_int_distribution<std::mt19937::result_type> dist35112_;
	unsigned extraSamples() { return dist35112_(rng_); }

	InputDialog * createInputDialog();
	GbVidBuf setPixelBuffer(void *pixels, PixelBuffer::PixelFormat format, std::ptrdiff_t pitch);
	std::ptrdiff_t runFor(uint_least32_t *pixels, std::ptrdiff_t pitch, quint32 *soundBuf, std::size_t &samples);
	void setResetting(bool state);
	void resetStepPre(std::size_t &samples);
	void resetStepPost(PixelBuffer const &pb, qint16 *const soundBuf, std::size_t &samples);
	void applyFade(PixelBuffer const &pb, qint16 *const soundBuf, std::size_t &samples);

	void enableBreakpoint(bool enable) { gb_.setInterruptAddresses(breakpoint_, enable ? 1 : 0); }
	int getHitAddress() { return gb_.getHitInterruptAddress(); }

	void emitSetTurbo(bool on) { if(!isResetting_) { emit setTurbo(on);} }
	void emitPause() { if(!isResetting_) { emit togglePause();} }
	void emitFrameStep() { if(!isResetting_) { emit frameStep();} }
	void emitDecFrameRate() { if(!isResetting_) { emit decFrameRate();} }
	void emitIncFrameRate() { if(!isResetting_) { emit incFrameRate();} }
	void emitResetFrameRate() { if(!isResetting_) { emit resetFrameRate();} }
	void emitPrevStateSlot() { if(!isResetting_) { emit prevStateSlot();} }
	void emitNextStateSlot() { if(!isResetting_) { emit nextStateSlot();} }
	void emitSaveState() { if(!isResetting_) { emit saveStateSignal();} }
	void emitLoadState() { if(!isResetting_) { emit loadStateSignal();} }
	void emitReset() { emit resetSignal(); }
	void emitQuit() { emit quit(); }
};

#endif
