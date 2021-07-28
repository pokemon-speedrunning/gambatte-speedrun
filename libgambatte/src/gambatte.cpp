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

using namespace gambatte;

namespace {

std::string to_string(int i) {
	std::stringstream ss;
	ss << i;
	return ss.str();
}

std::string statePath(std::string const &basePath, int stateNo) {
	return basePath + '_' + to_string(stateNo) + ".gqs";
}

}

struct GB::Priv {
	CPU cpu;
	int stateNo;
	unsigned loadflags;
	unsigned layersMask;

	uint_least32_t vbuff[160 * 144];

	Priv() : stateNo(1), loadflags(0), layersMask(layer_mask_bg | layer_mask_window | layer_mask_obj) {}

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

std::ptrdiff_t GB::runFor(gambatte::uint_least32_t *const soundBuf, std::size_t &samples) {
	if (!p_->cpu.loaded()) {
		samples = 0;
		return -1;
	}

	p_->cpu.setVideoBuffer(p_->vbuff, 160);
	p_->cpu.setSoundBuffer(soundBuf);

	long const cyclesSinceBlit = p_->cpu.runFor(samples * 2);
	samples = p_->cpu.fillSoundBuffer();
	return cyclesSinceBlit >= 0
	     ? static_cast<std::ptrdiff_t>(samples) - (cyclesSinceBlit >> 1)
	     : cyclesSinceBlit;
}

void GB::setLayers(unsigned mask) {
	p_->cpu.setLayers(mask);
}

void GB::blitTo(gambatte::uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
	gambatte::uint_least32_t *src = p_->vbuff;
	gambatte::uint_least32_t *dst = videoBuf;

	for (int i = 0; i < 144; i++) {
		std::memcpy(dst, src, sizeof (gambatte::uint_least32_t) * 160);
		src += 160;
		dst += pitch;
	}
}

void GB::reset(std::size_t samplesToStall, std::string const &build) {
	if (p_->cpu.loaded()) {
		if (p_->implicitSave())
			p_->cpu.saveSavedata();

		SaveState state;
		p_->cpu.setStatePtrs(state);
		p_->cpu.saveState(state);
		setInitState(state, p_->loadflags & CGB_MODE, p_->loadflags & SGB_MODE, p_->loadflags & GBA_FLAG, samplesToStall > 0 ? samplesToStall << 1 : 0);
		if (p_->loadflags & NO_BIOS)
			setPostBiosState(state, p_->loadflags & CGB_MODE, p_->loadflags & GBA_FLAG, externalRead(0x143) & 0x80);

		p_->cpu.loadState(state);

		if (samplesToStall > 0)
			p_->cpu.stall(samplesToStall << 1);

		if (!build.empty())
			p_->cpu.setOsdElement(newResetElement(build, GB::pakInfo().crc()));
	}
}

void GB::setInputGetter(InputGetter *getInput, void *p) {
	p_->cpu.setInputGetter(getInput, p);
}

void GB::setReadCallback(MemoryCallback callback) {
	p_->cpu.setReadCallback(callback);
}

void GB::setWriteCallback(MemoryCallback callback) {
	p_->cpu.setWriteCallback(callback);
}

void GB::setExecCallback(MemoryCallback callback) {
	p_->cpu.setExecCallback(callback);
}

void GB::setCDCallback(CDCallback cdc) {
	p_->cpu.setCDCallback(cdc);
}

void GB::setTraceCallback(void (*callback)(void *)) {
	p_->cpu.setTraceCallback(callback);
}

void GB::setScanlineCallback(void (*callback)(), int sl) {
	p_->cpu.setScanlineCallback(callback, sl);
}

void GB::setLinkCallback(void(*callback)()) {
	p_->cpu.setLinkCallback(callback);
}

void GB::setCartBusPullUpTime(unsigned long const cartBusPullUpTime) {
	p_->cpu.setCartBusPullUpTime(cartBusPullUpTime);
}

void GB::setRtcDivisorOffset(long const rtcDivisorOffset) {
	p_->cpu.setRtcDivisorOffset(rtcDivisorOffset);
}

void GB::setSaveDir(std::string const &sdir) {
	p_->cpu.setSaveDir(sdir);
}

LoadRes GB::load(std::string const &romfile, unsigned const flags) {
	if (p_->cpu.loaded() && p_->implicitSave())
		p_->cpu.saveSavedata();

	LoadRes const loadres = p_->cpu.load(romfile, flags);

	if (loadres == LOADRES_OK) {
		SaveState state;
		p_->cpu.setStatePtrs(state);
		p_->loadflags = flags;
		setInitState(state, flags & CGB_MODE, flags & SGB_MODE, flags & GBA_FLAG, 0);
		if (flags & NO_BIOS)
			setPostBiosState(state, flags & CGB_MODE, flags & GBA_FLAG, externalRead(0x143) & 0x80);

		setInitStateCart(state, flags & CGB_MODE, flags & GBA_FLAG);
		p_->cpu.loadState(state);
		p_->cpu.loadSavedata();

		p_->stateNo = 1;
		p_->cpu.setOsdElement(transfer_ptr<OsdElement>());
	}

	return loadres;
}

LoadRes GB::load(char const *romfiledata, unsigned romfilelength, unsigned const flags) {
	LoadRes const loadres = p_->cpu.load(romfiledata, romfilelength, flags);

	if (loadres == LOADRES_OK) {
		SaveState state;
		p_->cpu.setStatePtrs(state);
		p_->loadflags = flags;
		setInitState(state, flags & CGB_MODE, flags & SGB_MODE, flags & GBA_FLAG, (flags & GBA_FLAG) ? 971616 : 0);
		if (flags & NO_BIOS)
			setPostBiosState(state, flags & CGB_MODE, flags & GBA_FLAG, externalRead(0x143) & 0x80);

		setInitStateCart(state, flags & CGB_MODE, flags & GBA_FLAG);
		p_->cpu.loadState(state);

		if (flags & GBA_FLAG && !(flags & NO_BIOS))
			p_->cpu.stall(971616); // GBA takes 971616 cycles to switch to CGB mode; CGB CPU is inactive during this time.
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

	if ((p_->loadflags & GBA_FLAG) && (crc32(0, newBiosBuffer, sz) == 0x41884E46)) { // patch cgb bios to re'd agb bios equal 
		newBiosBuffer[0xF3] ^= 0x03;
		for (int i = 0xF5; i < 0xFB; i++)
			newBiosBuffer[i] = newBiosBuffer[i + 1];

		newBiosBuffer[0xFB] ^= 0x74;
	}

	p_->cpu.setBios(newBiosBuffer, sz);
	return 0;
}

int GB::loadBios(char const *biosfiledata, std::size_t size) {
	p_->cpu.setBios((unsigned char*)biosfiledata, size);
	return 0;
}

bool GB::isCgb() const {
	return p_->cpu.isCgb();
}

bool GB::isCgbDmg() const {
	return p_->cpu.isCgbDmg();
}

bool GB::isLoaded() const {
	return p_->cpu.loaded();
}

void GB::saveSavedata() {
	if (p_->cpu.loaded())
		p_->cpu.saveSavedata();
}

int GB::saveSavedataLength(bool isDeterministic) {
	if (p_->cpu.loaded())
		return p_->cpu.saveSavedataLength(isDeterministic);
	else
		return -1;
}

void GB::loadSavedata(char const *data, bool isDeterministic) {
	if (p_->cpu.loaded())
		p_->cpu.loadSavedata(data, isDeterministic);
}

void GB::saveSavedata(char *dest, bool isDeterministic) {
	if (p_->cpu.loaded())
		p_->cpu.saveSavedata(dest, isDeterministic);
}

bool GB::getMemoryArea(int which, unsigned char **data, int *length) {
	if (p_->cpu.loaded())
		return p_->cpu.getMemoryArea(which, data, length);
	else
		return false;
}

void GB::setDmgPaletteColor(int palNum, int colorNum, unsigned long rgb32) {
	p_->cpu.setDmgPaletteColor(palNum, colorNum, rgb32);
}

void GB::setCgbPalette(unsigned *lut) {
	p_->cpu.setCgbPalette(lut);
}

void GB::setTimeMode(bool useCycles) {
	p_->cpu.setTimeMode(useCycles);
}

bool GB::loadState(std::string const &filepath) {
	if (p_->cpu.loaded()) {
		if (p_->implicitSave())
			p_->cpu.saveSavedata();

		SaveState state = SaveState();
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

int GB::linkStatus(int which) {
	return p_->cpu.linkStatus(which);
}

void GB::getRegs(int *dest) {
	p_->cpu.getRegs(dest);
}

void GB::setRegs(int *src) {
	p_->cpu.setRegs(src);
}

void GB::getRtcRegs(unsigned long *dest) {
	p_->cpu.getRtcRegs(dest);
}

void GB::setRtcRegs(unsigned long *src) {
	p_->cpu.setRtcRegs(src);
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

void GB::setSpeedupFlags(unsigned flags) {
	p_->cpu.setSpeedupFlags(flags);
}

SYNCFUNC(GB) {
	SSS(p_->cpu);
	NSS(p_->loadflags);
	NSS(p_->vbuff);
}
