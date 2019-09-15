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

class GambatteSource : public QObject, public MediaSource {
public:
	GambatteSource();
	std::vector<VideoDialog::VideoSourceInfo> const generateVideoSourceInfos();
	gambatte::LoadRes load(std::string const &romfile, unsigned flags) { return gb_.load(romfile, flags); }
	unsigned int loadBios(std::string const &biosfile, std::size_t size, unsigned crc) {
		return gb_.loadBios(biosfile, size, crc);
	}
	void setGameGenie(std::string const &codes) { gb_.setGameGenie(codes); }
	void setGameShark(std::string const &codes) { gb_.setGameShark(codes); }
	void reset() { isResetting_ = false; gb_.reset(QT_VERSION_STR); }

	void setDmgPaletteColor(int palNum, int colorNum, unsigned long rgb32) {
		gb_.setDmgPaletteColor(palNum, colorNum, rgb32);
	}

	void setSavedir(std::string const &sdir) { gb_.setSaveDir(sdir); }
	void setVideoSource(std::size_t videoSourceIndex);
	bool isCgb() const { return gb_.isCgb(); }
	std::string const romTitle() const { return gb_.romTitle(); }
	gambatte::PakInfo pakInfo() const { return gb_.pakInfo(); }
	void selectState(int n) { gb_.selectState(n); }
	int currentState() const { return gb_.currentState(); }
	void saveState(PixelBuffer const &fb, std::string const &filepath);
	void loadState(std::string const &filepath) { gb_.loadState(filepath); }
	QDialog * inputDialog() const { return inputDialog_; }
	void saveState(PixelBuffer const &fb);
	void loadState() { gb_.loadState(); }
    void tryReset();
	void setResetParams(unsigned before, unsigned fade, unsigned limit);

	virtual void keyPressEvent(QKeyEvent const *);
	virtual void keyReleaseEvent(QKeyEvent const *);
	virtual void joystickEvent(SDL_Event const &);
    virtual void clearKeyPresses();
	virtual std::ptrdiff_t update(PixelBuffer const &fb, qint16 *soundBuf, std::size_t &samples);
	virtual void generateVideoFrame(PixelBuffer const &fb);

public slots:
	void setTrueColors(bool trueColors) { gb_.setTrueColors(trueColors); }
	void setTimeMode(bool useCycles) { gb_.setTimeMode(useCycles); }

signals:
	void setTurbo(bool on);
	void togglePause();
    void pauseAndReset();
	void frameStep();
	void decFrameRate();
	void incFrameRate();
	void resetFrameRate();
	void prevStateSlot();
	void nextStateSlot();
	void saveStateSignal();
	void loadStateSignal();
    void startResetting();
	void quit();

private:
	Q_OBJECT

	struct GbVidBuf;
	struct GetInput {
		unsigned is;
		GetInput() : is(0) {}
		static unsigned get(GetInput *p) { return p->is; }
	};

	gambatte::GB gb_;
	GetInput inputGetter_;
	InputDialog *const inputDialog_;
	scoped_ptr<VideoLink> cconvert_;
	scoped_ptr<VideoLink> vfilter_;
	PixelBuffer::PixelFormat pxformat_;
	std::size_t vsrci_;
	bool inputState_[10];
	bool dpadUp_, dpadDown_;
	bool dpadLeft_, dpadRight_;
	bool dpadUpLast_, dpadLeftLast_;
    bool isResetting_;
    unsigned resetFrameCount_;
	unsigned resetBefore_;
	unsigned resetFade_;
	unsigned resetLimit_;

	InputDialog * createInputDialog();
	GbVidBuf setPixelBuffer(void *pixels, PixelBuffer::PixelFormat format, std::ptrdiff_t pitch);
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
	void emitReset() { emit tryReset(); }
	void emitQuit() { emit quit(); }
};

#endif
