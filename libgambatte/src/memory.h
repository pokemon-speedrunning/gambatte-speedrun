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

#ifndef MEMORY_H
#define MEMORY_H

static unsigned char const agbOverride[0xD] = { 0xFF, 0x00, 0xCD, 0x03, 0x35, 0xAA, 0x31, 0x90, 0x94, 0x00, 0x00, 0x00, 0x00 };

#include "mem/cartridge.h"
#include "mem/sgb.h"
#include "inputgetter.h"
#include "interrupter.h"
#include "pakinfo.h"
#include "sound.h"
#include "tima.h"
#include "video.h"

#include <cstring>

namespace gambatte {

class FilterInfo;

class Memory {
public:
	explicit Memory(Interrupter const &interrupter);
	~Memory();
	bool loaded() const { return cart_.loaded(); }
	unsigned char curRomBank() const { return cart_.curRomBank(); }
	char const * romTitle() const { return cart_.romTitle(); }
	PakInfo const pakInfo(bool multicartCompat) const { return cart_.pakInfo(multicartCompat); }
	void setStatePtrs(SaveState &state);
	unsigned long saveState(SaveState &state, unsigned long cc);
	void loadState(SaveState const &state);
	void loadSavedata(unsigned long const cc) { cart_.loadSavedata(cc); }
	void saveSavedata(unsigned long const cc) { cart_.saveSavedata(cc); }
	std::string const saveBasePath() const { return cart_.saveBasePath(); }

	void setOsdElement(transfer_ptr<OsdElement> osdElement) {
		lcd_.setOsdElement(osdElement);
	}

	unsigned long stop(unsigned long cycleCounter, bool &skip);
	void stall(unsigned long cycleCounter, unsigned long cycles);
	bool isCgb() const { return lcd_.isCgb(); }
	bool isCgbDmg() const { return lcd_.isCgbDmg(); }
	bool isSgb() const { return gbIsSgb_; }
	bool ime() const { return intreq_.ime(); }
	bool halted() const { return intreq_.halted(); }
	unsigned long nextEventTime() const { return intreq_.minEventTime(); }
	bool isActive() const { return intreq_.eventTime(intevent_end) != disabled_time; }

	long cyclesSinceBlit(unsigned long cc) const {
		if (cc < intreq_.eventTime(intevent_blit))
			return -1;

		return (cc - intreq_.eventTime(intevent_blit)) >> isDoubleSpeed();
	}

	void freeze(unsigned long cc);
	bool halt(unsigned long cc);
	void ei(unsigned long cycleCounter) { if (!ime()) { intreq_.ei(cycleCounter); } }
	void di() { intreq_.di(); }
	unsigned pendingIrqs(unsigned long cc);
	void ackIrq(unsigned bit, unsigned long cc);

	unsigned readBios(unsigned p) {
		if (agbFlag_ && p >= 0xF3 && p < 0x100)
			return (agbOverride[p - 0xF3] + bios_[p]) & 0xFF;

		return bios_[p];
	}

	unsigned ff_read(unsigned p, unsigned long cc) {
		bus_ = p < 0x80 ? nontrivial_ff_read(p, cc) : ioamhram_[p + 0x100];
		cartbus_ = 0xFF;
		return bus_;
	}

	unsigned read(unsigned p, unsigned long cc, bool pcRead) {
		if (biosMode_ && (p < biosSize_ && !(p >= 0x100 && p < 0x200))) {
			bus_ = readBios(p);
			cartbus_ = 0xFF;
		} else if (p >= mm_sram_begin && p < mm_wram_begin && cart_.disabledRam()) {
			if (pcRead)
				cartbus_ = 0xFF;
			bus_ = cart_.rmem(p >> 12) ? cartbus_ : nontrivial_read(p, cc);
			cartbus_ = 0xFF;
		} else {
			bus_ = cart_.rmem(p >> 12) ? cart_.rmem(p >> 12)[p] : nontrivial_read(p, cc);
				if ((p < mm_vram_begin) || (!isCgb() && (p >= mm_wram_begin && p < mm_oam_begin)))
					cartbus_ = bus_;
				else
					cartbus_ = 0xFF;
		}
		return bus_;
	}

	void write(unsigned p, unsigned data, unsigned long cc) {
		if (cart_.wmem(p >> 12)) {
			cart_.wmem(p >> 12)[p] = data;
		} else
			nontrivial_write(p, data, cc);
	}

	void ff_write(unsigned p, unsigned data, unsigned long cc) {
		if (p - 0x80u < 0x7Fu) {
			ioamhram_[p + 0x100] = data;
		} else
			nontrivial_ff_write(p, data, cc);
	}

	unsigned long event(unsigned long cycleCounter);
	unsigned long resetCounters(unsigned long cycleCounter);
	LoadRes loadROM(std::string const &romfile, unsigned flags);
	void setSaveDir(std::string const &dir) { cart_.setSaveDir(dir); }

	void setInputGetter(InputGetter *getInput, void *p) {
		getInput_ = getInput;
		getInputP_ = p;
	}

	void setEndtime(unsigned long cc, unsigned long inc);
	void setSoundBuffer(uint_least32_t *buf) { psg_.setBuffer(buf); }
	std::size_t fillSoundBuffer(unsigned long cc);

	void setVideoBuffer(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
		lcd_.setVideoBuffer(videoBuf, pitch);
		sgb_.setVideoBuffer(videoBuf, pitch);
	}

	void setDmgPaletteColor(int palNum, int colorNum, unsigned long rgb32) {
		if (!gbIsSgb_)
			lcd_.setDmgPaletteColor(palNum, colorNum, rgb32);
	}
    
	void blackScreen() {
		lcd_.blackScreen();
	}

	void setTrueColors(bool trueColors) {
		lcd_.setTrueColors(trueColors);
		sgb_.setTrueColors(trueColors);
	}

	void setTimeMode(bool useCycles, unsigned long const cc) {
		cart_.setTimeMode(useCycles, cc);
	}
	void setRtcDivisorOffset(long const rtcDivisorOffset) { cart_.setRtcDivisorOffset(rtcDivisorOffset); }

	void setGameGenie(std::string const &codes) { cart_.setGameGenie(codes); }
	void setGameShark(std::string const &codes) { interrupter_.setGameShark(codes); }
	void updateInput();

	void setBios(unsigned char *buffer, std::size_t size) {
		delete []bios_;
		bios_ = new unsigned char[size];
		std::memcpy(bios_, buffer, size);
		biosSize_ = size;
	}

	unsigned timeNow(unsigned long const cc) const { return cart_.timeNow(cc); }

	unsigned long getDivLastUpdate() { return divLastUpdate_; }
	unsigned char getRawIOAMHRAM(int offset) { return ioamhram_[offset]; }

	void setSpeedupFlags(unsigned flags) {
		lcd_.setSpeedupFlags(flags);
		psg_.setSpeedupFlags(flags);
	}

private:
	Cartridge cart_;
	Sgb sgb_;
	unsigned char ioamhram_[0x200];
	unsigned char *bios_;
	std::size_t biosSize_;
	InputGetter *getInput_;
	void *getInputP_;
	unsigned long divLastUpdate_;
	unsigned long lastOamDmaUpdate_;
	InterruptRequester intreq_;
	Tima tima_;
	LCD lcd_;
	PSG psg_;
	Interrupter interrupter_;
	unsigned short dmaSource_;
	unsigned short dmaDestination_;
	unsigned char oamDmaPos_;
	unsigned char oamDmaStartPos_;
	unsigned char serialCnt_;
	unsigned char bus_;
	unsigned char cartbus_;
	bool blanklcd_;
	bool biosMode_;
	bool agbFlag_;
	bool gbIsSgb_;
	bool stopped_;
	enum HdmaState { hdma_low, hdma_high, hdma_requested } haltHdmaState_;

	void decEventCycles(IntEventId eventId, unsigned long dec);
	void oamDmaInitSetup();
	void updateOamDma(unsigned long cycleCounter);
	void startOamDma(unsigned long cycleCounter);
	void endOamDma(unsigned long cycleCounter);
	unsigned char const * oamDmaSrcPtr() const;
	unsigned long dma(unsigned long cc);
	unsigned nontrivial_ff_read(unsigned p, unsigned long cycleCounter);
	unsigned nontrivial_read(unsigned p, unsigned long cycleCounter);
	void nontrivial_ff_write(unsigned p, unsigned data, unsigned long cycleCounter);
	void nontrivial_write(unsigned p, unsigned data, unsigned long cycleCounter);
	void updateSerial(unsigned long cc);
	void updateTimaIrq(unsigned long cc);
	void updateIrqs(unsigned long cc);
	bool isDoubleSpeed() const { return lcd_.isDoubleSpeed(); }
};

}

#endif
