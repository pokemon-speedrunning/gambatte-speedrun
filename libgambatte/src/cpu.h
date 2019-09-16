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

#ifndef CPU_H
#define CPU_H

#include "memory.h"

namespace gambatte {

class CPU {
public:
	CPU();
	long runFor(unsigned long cycles);
	void setStatePtrs(SaveState &state);
	void saveState(SaveState &state);
	void loadState(SaveState const &state);
	void loadSavedata() { mem_.loadSavedata(cycleCounter_); }
	void saveSavedata() { mem_.saveSavedata(cycleCounter_); }

	void setVideoBuffer(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
		mem_.setVideoBuffer(videoBuf, pitch);
	}

	void setInputGetter(InputGetter *getInput, void *p) {
		mem_.setInputGetter(getInput, p);
	}

	void setSaveDir(std::string const &sdir) {
		mem_.setSaveDir(sdir);
	}

	std::string const saveBasePath() const {
		return mem_.saveBasePath();
	}

	void setOsdElement(transfer_ptr<OsdElement> osdElement) {
		mem_.setOsdElement(osdElement);
	}

	LoadRes load(std::string const &romfile, bool cgbMode, bool multicartCompat) {
		return mem_.loadROM(romfile, cgbMode, multicartCompat);
	}

	bool loaded() const { return mem_.loaded(); }
	char const * romTitle() const { return mem_.romTitle(); }
	PakInfo const pakInfo(bool multicartCompat) const { return mem_.pakInfo(multicartCompat); }
	void setSoundBuffer(uint_least32_t *buf) { mem_.setSoundBuffer(buf); }
	std::size_t fillSoundBuffer() { return mem_.fillSoundBuffer(cycleCounter_); }
	bool isCgb() const { return mem_.isCgb(); }

	void setDmgPaletteColor(int palNum, int colorNum, unsigned long rgb32) {
		mem_.setDmgPaletteColor(palNum, colorNum, rgb32);
	}

	void setTrueColors(bool trueColors) { mem_.setTrueColors(trueColors); }
	void setTimeMode(bool useCycles) { mem_.setTimeMode(useCycles, cycleCounter_); }

	void setGameGenie(std::string const &codes) { mem_.setGameGenie(codes); }
	void setGameShark(std::string const &codes) { mem_.setGameShark(codes); }
	void setBios(unsigned char *buffer, std::size_t size) { mem_.setBios(buffer, size); }
	bool gbIsCgb() { return mem_.gbIsCgb(); }

	unsigned char externalRead(unsigned short addr) {
		return mem_.read(addr, cycleCounter_);
	}

	void externalWrite(unsigned short addr, unsigned char val) {
		mem_.write(addr, val, cycleCounter_);
	}

	void getRegs(int *dest);
	void setRegs(int *src);
	void setInterruptAddresses(int *addrs, int numAddrs);
	int getHitInterruptAddress();

	unsigned timeNow() const { return mem_.timeNow(cycleCounter_); }

    unsigned long getCycleCounter() { return cycleCounter_; }
    unsigned long getDivLastUpdate() { return mem_.getDivLastUpdate(); }
    unsigned char getRawIOAMHRAM(int offset) { return mem_.getRawIOAMHRAM(offset); }

private:
	Memory mem_;
	unsigned long cycleCounter_;
	unsigned short pc_;
	unsigned short sp;
	unsigned hf1, hf2, zf, cf;
	unsigned char a_, b, c, d, e, /*f,*/ h, l;
	bool skip_;

	int *interruptAddresses;
	int numInterruptAddresses;
	int hitInterruptAddress;

	void process(unsigned long cycles);
};

}

#endif
