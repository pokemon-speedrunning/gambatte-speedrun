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

#include "gambatte.h"
#include "cpu.h"
#include "initstate.h"
#include "savestate.h"
#include "state_osd_elements.h"
#include "statesaver.h"
#include "file/file.h"
#include <cstring>
#include <sstream>
#include <zlib.h>

static std::string const itos(int i) {
	std::stringstream ss;
	ss << i;
	return ss.str();
}

static std::string const statePath(std::string const &basePath, int stateNo) {
	return basePath + "_" + itos(stateNo) + ".gqs";
}

namespace gambatte {

struct GB::Priv {
	CPU cpu;
	int stateNo;
	unsigned loadflags;

	Priv() : stateNo(1), loadflags(0) {}

	unsigned criticalLoadflags() {
		return loadflags & (CGB_MODE | SGB_MODE);
	}

	bool implicitSave() {
		return !(loadflags & READONLY_SAV);
	}
};

GB::GB() : p_(new Priv) {}

GB::~GB() {
	if (p_->cpu.loaded() && p_->implicitSave())
		p_->cpu.saveSavedata();

	delete p_;
}

std::ptrdiff_t GB::runFor(gambatte::uint_least32_t *const videoBuf, std::ptrdiff_t const pitch,
                          gambatte::uint_least32_t *const soundBuf, std::size_t &samples) {
	if (!p_->cpu.loaded()) {
		samples = 0;
		return -1;
	}

	p_->cpu.setVideoBuffer(videoBuf, pitch);
	p_->cpu.setSoundBuffer(soundBuf);

	long const cyclesSinceBlit = p_->cpu.runFor(samples * 2);
	samples = p_->cpu.fillSoundBuffer();
	return cyclesSinceBlit >= 0
	     ? static_cast<std::ptrdiff_t>(samples) - (cyclesSinceBlit >> 1)
	     : cyclesSinceBlit;
}

void GB::reset(std::string const &build) {
	if (p_->cpu.loaded()) {
		if (p_->implicitSave())
			p_->cpu.saveSavedata();

		SaveState state;
		p_->cpu.setStatePtrs(state);
		p_->cpu.saveState(state);
		unsigned flags = p_->loadflags;
		setInitState(state, flags & CGB_MODE, flags & GBA_FLAG, flags & SGB_MODE);
		p_->cpu.loadState(state);

		if (!build.empty())
			p_->cpu.setOsdElement(newResetElement(build, GB::pakInfo().crc()));
	}
}

void GB::setInputGetter(InputGetter *getInput, void *p) {
	p_->cpu.setInputGetter(getInput, p);
}

void GB::setSaveDir(std::string const &sdir) {
	p_->cpu.setSaveDir(sdir);
}

LoadRes GB::load(std::string const &romfile, unsigned const flags) {
	if (p_->cpu.loaded() && p_->implicitSave())
		p_->cpu.saveSavedata();

	LoadRes const loadres = p_->cpu.load(romfile,
	                                     flags & CGB_MODE,
	                                     flags & MULTICART_COMPAT);
	if (loadres == LOADRES_OK) {
		SaveState state;
		p_->cpu.setStatePtrs(state);
		p_->loadflags = flags;
		setInitState(state, flags & CGB_MODE, flags & GBA_FLAG, flags & SGB_MODE);
		setInitStateCart(state);
		p_->cpu.loadState(state);
		p_->cpu.loadSavedata();

		p_->stateNo = 1;
		p_->cpu.setOsdElement(transfer_ptr<OsdElement>());
	}

	return loadres;
}

int GB::loadBios(std::string const &biosfile, std::size_t size, unsigned crc) {
	scoped_ptr<File> const bios(newFileInstance(biosfile));
	
	if (bios->fail())
		return -1;
	
	std::size_t sz = bios->size();
	
	if (size != 0 && sz != size)
		return -2;
	
	unsigned char newBiosBuffer[sz];
	bios->read((char *)newBiosBuffer, sz);
	
	if (bios->fail())
		return -1;
	
	if (crc != 0) {
		unsigned char maskedBiosBuffer[sz];
		std::memcpy(maskedBiosBuffer, newBiosBuffer, sz);
		maskedBiosBuffer[0xFD] = 0;

		if (crc32(0, maskedBiosBuffer, sz) != crc)
			return -3;
	}
	
	p_->cpu.setBios(newBiosBuffer, sz);
	return 0;
}

bool GB::isCgb() const {
	return p_->cpu.isCgb();
}

bool GB::isLoaded() const {
	return p_->cpu.loaded();
}

void GB::saveSavedata() {
	if (p_->cpu.loaded())
		p_->cpu.saveSavedata();
}

void GB::setDmgPaletteColor(int palNum, int colorNum, unsigned long rgb32) {
	p_->cpu.setDmgPaletteColor(palNum, colorNum, rgb32);
}

void GB::setTrueColors(bool trueColors) {
	p_->cpu.setTrueColors(trueColors);
}

void GB::setTimeMode(bool useCycles) {
	p_->cpu.setTimeMode(useCycles);
}

bool GB::loadState(std::string const &filepath) {
	if (p_->cpu.loaded()) {
		if (p_->implicitSave())
			p_->cpu.saveSavedata();

		SaveState state;
		p_->cpu.setStatePtrs(state);

		if (StateSaver::loadState(state, filepath, true, p_->criticalLoadflags())) {
			p_->cpu.loadState(state);
			return true;
		}
	}

	return false;
}

bool GB::saveState(gambatte::uint_least32_t const *videoBuf, std::ptrdiff_t pitch) {
	if (saveState(videoBuf, pitch, statePath(p_->cpu.saveBasePath(), p_->stateNo))) {
		p_->cpu.setOsdElement(newStateSavedOsdElement(p_->stateNo));
		return true;
	}

	return false;
}

bool GB::loadState() {
	if (loadState(statePath(p_->cpu.saveBasePath(), p_->stateNo))) {
		p_->cpu.setOsdElement(newStateLoadedOsdElement(p_->stateNo));
		return true;
	}

	return false;
}

bool GB::saveState(gambatte::uint_least32_t const *videoBuf, std::ptrdiff_t pitch,
                   std::string const &filepath) {
	if (p_->cpu.loaded()) {
		SaveState state;
		p_->cpu.setStatePtrs(state);
		p_->cpu.saveState(state);
		return StateSaver::saveState(state, videoBuf, pitch, filepath, p_->criticalLoadflags());
	}

	return false;
}

std::size_t GB::saveState(gambatte::uint_least32_t const *videoBuf, std::ptrdiff_t pitch,
                          char *stateBuf) {
	if (p_->cpu.loaded()) {
		SaveState state;
		p_->cpu.setStatePtrs(state);
		p_->cpu.saveState(state);
		return StateSaver::saveState(state, videoBuf, pitch, stateBuf, p_->criticalLoadflags());
	}

	return 0;
}

bool GB::loadState(char const *stateBuf, std::size_t size) {
	if (p_->cpu.loaded()) {
		SaveState state;
		p_->cpu.setStatePtrs(state);

		if (StateSaver::loadState(state, stateBuf, size, true, p_->criticalLoadflags())) {
			p_->cpu.loadState(state);
			return true;
		}
	}

	return false;
}

void GB::selectState(int n) {
	n -= (n / 10) * 10;
	p_->stateNo = n < 0 ? n + 10 : n;

	if (p_->cpu.loaded()) {
		std::string const &path = statePath(p_->cpu.saveBasePath(), p_->stateNo);
		p_->cpu.setOsdElement(newSaveStateOsdElement(path, p_->stateNo));
	}
}

int GB::currentState() const { return p_->stateNo; }

std::string const GB::romTitle() const {
	if (p_->cpu.loaded()) {
		char title[0x11];
		std::memcpy(title, p_->cpu.romTitle(), 0x10);
		title[title[0xF] & 0x80 ? 0xF : 0x10] = '\0';
		return std::string(title);
	}

	return std::string();
}

PakInfo const GB::pakInfo() const { return p_->cpu.pakInfo(p_->loadflags & MULTICART_COMPAT); }

void GB::setGameGenie(std::string const &codes) {
	p_->cpu.setGameGenie(codes);
}

void GB::setGameShark(std::string const &codes) {
	p_->cpu.setGameShark(codes);
}

unsigned char GB::externalRead(unsigned short addr) {
	if (p_->cpu.loaded())
		return p_->cpu.externalRead(addr);
	else
		return 0;
}

void GB::externalWrite(unsigned short addr, unsigned char val) {
	if (p_->cpu.loaded())
		p_->cpu.externalWrite(addr, val);
}

void GB::getRegs(int *dest) {
	p_->cpu.getRegs(dest);
}

void GB::setRegs(int *src) {
	p_->cpu.setRegs(src);
}

void GB::setInterruptAddresses(int *addrs, int numAddrs) {
	p_->cpu.setInterruptAddresses(addrs, numAddrs);
}

int GB::getHitInterruptAddress() {
	return p_->cpu.getHitInterruptAddress();
}

unsigned GB::timeNow() const {
	return p_->cpu.timeNow();
}

int GB::getDivState() {
    int cc = p_->cpu.getCycleCounter();
    int divOff = cc - p_->cpu.getDivLastUpdate();
    int div = p_->cpu.getRawIOAMHRAM(0x104);
    return (((div << 8) + divOff) >> 2) & 0x3FFF;
}

}
