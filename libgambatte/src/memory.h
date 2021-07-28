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

#include "mem/cartridge.h"
#include "mem/sgb.h"
#include "inputgetter.h"
#include "interrupter.h"
#include "pakinfo.h"
#include "sound.h"
#include "tima.h"
#include "video.h"
#include "gambatte.h"
#include "newstate.h"

#include <cstring>

namespace gambatte {

class FilterInfo;

class Memory {
public:
	explicit Memory(Interrupter const &interrupter);
	~Memory();
	bool loaded() const { return cart_.loaded(); }
	unsigned curRomBank() const { return cart_.curRomBank(); }
	char const * romTitle() const { return cart_.romTitle(); }
	int peekLY() const { return lcd_.peekLy(); }
	int getLy(unsigned long const cc) { return nontrivial_ff_read(0x44, cc); }
	PakInfo const pakInfo(bool multicartCompat) const { return cart_.pakInfo(multicartCompat); }
	void setStatePtrs(SaveState &state);
	unsigned long saveState(SaveState &state, unsigned long cc);
	void loadState(SaveState const &state);
	void loadSavedata(unsigned long const cc) { cart_.loadSavedata(cc); }
	void saveSavedata(unsigned long const cc) { cart_.saveSavedata(cc); }
	int saveSavedataLength(bool isDeterministic) { return cart_.saveSavedataLength(isDeterministic); }
	void loadSavedata(char const *data, unsigned long const cc, bool isDeterministic) { cart_.loadSavedata(data, cc, isDeterministic); }
	void saveSavedata(char* dest, unsigned long const cc, bool isDeterministic) { cart_.saveSavedata(dest, cc, isDeterministic); }

	std::string const saveBasePath() const { return cart_.saveBasePath(); }

	void setOsdElement(transfer_ptr<OsdElement> osdElement) {
		lcd_.setOsdElement(osdElement);
	}

	bool getMemoryArea(int which, unsigned char **data, int *length);

	unsigned long stop(unsigned long cycleCounter, bool &prefetched);
	void stall(unsigned long cycleCounter, unsigned long cycles);
	bool isCgb() const { return lcd_.isCgb(); }
	bool isCgbDmg() const { return lcd_.isCgbDmg(); }
	bool isSgb() const { return gbIsSgb_; }
	bool ime() const { return intreq_.ime(); }
	bool halted() const { return intreq_.halted(); }
	unsigned long nextEventTime() const { return intreq_.minEventTime(); }
	void setLayers(unsigned mask) { lcd_.setLayers(mask); }
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
		return bios_[p];
	}

	unsigned ff_read(unsigned p, unsigned long cc) {
		if (readCallback_)
			readCallback_(p, (cc - basetime_) >> 1);

		return p < 0x80 ? nontrivial_ff_read(p, cc) : ioamhram_[p + 0x100];
	}

	struct CDMapResult {
		eCDLog_AddrType type;
		unsigned addr;
	};

	CDMapResult CDMap(const unsigned p) const {
		if (p < mm_rom1_begin) {
			CDMapResult ret = { eCDLog_AddrType_ROM, p };
			return ret;
		} else if (p < mm_vram_begin) {
			unsigned bank = cart_.rmem(p >> 12) - cart_.rmem(0);
			unsigned addr = p + bank;
			CDMapResult ret = { eCDLog_AddrType_ROM, addr };
			return ret;
		} else if (p < mm_sram_begin) {
		} else if (p < mm_wram_begin) {
			if (cart_.wsrambankptr()) {
				//not bankable. but. we're not sure how much might be here
				unsigned char *data;
				int length;
				bool has = cart_.getMemoryArea(3, &data, &length);
				unsigned addr = p & (length - 1);
				if (has && length) {
					CDMapResult ret = { eCDLog_AddrType_CartRAM, addr };
					return ret;
				}
			}
		} else if (p < mm_wram_mirror_begin) {
			unsigned bank = cart_.wramdata(p >> 12 & 1) - cart_.wramdata(0);
			unsigned addr = (p & 0xFFF) + bank;
			CDMapResult ret = { eCDLog_AddrType_WRAM, addr };
			return ret;
		} else if (p < mm_hram_begin) {
		} else {
			////this is just for debugging, really, it's pretty useless
			//CDMapResult ret = { eCDLog_AddrType_HRAM, (p - mm_hram_begin) };
			//return ret;
		}
		CDMapResult ret = { eCDLog_AddrType_None, p };
		return ret;
	}

	unsigned read(unsigned p, unsigned long cc) {
		if (readCallback_)
			readCallback_(p, (cc - basetime_) >> 1);

		if (biosMode_ && p < biosSize_ && !(p >= 0x100 && p < 0x200))
			return readBios(p);
		else if (cdCallback_) {
			CDMapResult map = CDMap(p);
			if (map.type != eCDLog_AddrType_None)
				cdCallback_(map.addr, map.type, eCDLog_Flags_Data);
		}
		if (cart_.disabledRam() && (p >= mm_sram_begin && p < mm_wram_begin)) {
			return cart_.rmem(p >> 12)
				? (lastCartBusUpdate_ + (cartBusPullUpTime_ << isDoubleSpeed()) > cc ? cartBus_ : 0xFF)
				: nontrivial_read(p, cc);
		}
		if (cart_.isMbc2() && (p >= mm_sram_begin && p < mm_wram_begin)) {
			p &= 0xA1FF;
			return cart_.rmem(p >> 12)
				? (cart_.rmem(p >> 12)[p] & 0x0F) | (lastCartBusUpdate_ + (cartBusPullUpTime_ << isDoubleSpeed()) > cc ? (cartBus_ & 0xF0) : 0xF0)
				: nontrivial_read(p, cc);
		}
		if ((p < mm_vram_begin) || (!isCgb() && (p >= mm_wram_begin && p < mm_oam_begin))) {
			cartBus_ = cart_.rmem(p >> 12) ? cart_.rmem(p >> 12)[p] : nontrivial_read(p, cc);
			lastCartBusUpdate_ = cc;
			return cartBus_;
		}
		return cart_.rmem(p >> 12) ? cart_.rmem(p >> 12)[p] : nontrivial_read(p, cc);
	}

	unsigned read_excb(unsigned p, unsigned long cc, bool opcode) {
		if (opcode && execCallback_)
			execCallback_(p, (cc - basetime_) >> 1);

		if (biosMode_ && p < biosSize_ && !(p >= 0x100 && p < 0x200))
			return readBios(p);
		else if (cdCallback_) {
			CDMapResult map = CDMap(p);
			if (map.type != eCDLog_AddrType_None)
				cdCallback_(map.addr, map.type, opcode ? eCDLog_Flags_ExecOpcode : eCDLog_Flags_ExecOperand);
		}
		if (cart_.disabledRam() && (p >= mm_sram_begin && p < mm_wram_begin)) {
			return cart_.rmem(p >> 12)
				? (lastCartBusUpdate_ + (cartBusPullUpTime_ << isDoubleSpeed()) > cc ? cartBus_ : 0xFF)
				: nontrivial_read(p, cc);
		}
		if (cart_.isMbc2() && (p >= mm_sram_begin && p < mm_wram_begin)) {
			p &= 0xA1FF;
			return cart_.rmem(p >> 12)
				? (cart_.rmem(p >> 12)[p] & 0x0F) | (lastCartBusUpdate_ + (cartBusPullUpTime_ << isDoubleSpeed()) > cc ? (cartBus_ & 0xF0) : 0xF0)
				: nontrivial_read(p, cc);
		}
		if ((p < mm_vram_begin) || (!isCgb() && (p >= mm_wram_begin && p < mm_oam_begin))) {
			cartBus_ = cart_.rmem(p >> 12) ? cart_.rmem(p >> 12)[p] : nontrivial_read(p, cc);
			lastCartBusUpdate_ = cc;
			return cartBus_;
		}
		return cart_.rmem(p >> 12) ? cart_.rmem(p >> 12)[p] : nontrivial_read(p, cc);
	}

	unsigned peek(unsigned p, unsigned long cc) {
		if (biosMode_ && p < biosSize_ && !(p >= 0x100 && p < 0x200))
			return readBios(p);

		if (cart_.disabledRam() && (p >= mm_sram_begin && p < mm_wram_begin)) {
			return cart_.rmem(p >> 12)
				? (lastCartBusUpdate_ + (cartBusPullUpTime_ << isDoubleSpeed()) > cc ? cartBus_ : 0xFF)
				: nontrivial_peek(p, cc);
		}
		if (cart_.isMbc2() && (p >= mm_sram_begin && p < mm_wram_begin)) {
			p &= 0xA1FF;
			return cart_.rmem(p >> 12)
				? (cart_.rmem(p >> 12)[p] & 0x0F) | (lastCartBusUpdate_ + (cartBusPullUpTime_ << isDoubleSpeed()) > cc ? (cartBus_ & 0xF0) : 0xF0)
				: nontrivial_peek(p, cc);
		}
		return cart_.rmem(p >> 12) ? cart_.rmem(p >> 12)[p] : nontrivial_peek(p, cc);
	}

	void write_nocb(unsigned p, unsigned data, unsigned long cc) {
		if (cart_.isMbc2() && (p >= mm_sram_begin && p < mm_wram_begin))
			p &= 0xA1FF;

		if (cart_.wmem(p >> 12))
			cart_.wmem(p >> 12)[p] = data;
		else
			nontrivial_write(p, data, cc);
	}

	void write(unsigned p, unsigned data, unsigned long cc) {
		if (cart_.isMbc2() && (p >= mm_sram_begin && p < mm_wram_begin))
			p &= 0xA1FF;

		if (cart_.wmem(p >> 12))
			cart_.wmem(p >> 12)[p] = data;
		else
			nontrivial_write(p, data, cc);

		if (writeCallback_)
			writeCallback_(p, (cc - basetime_) >> 1);

		if (cdCallback_ && !biosMode_) {
			CDMapResult map = CDMap(p);
			if (map.type != eCDLog_AddrType_None)
				cdCallback_(map.addr, map.type, eCDLog_Flags_Data);
		}
	}

	void ff_write(unsigned p, unsigned data, unsigned long cc) {
		if (p - 0x80u < 0x7Fu)
			ioamhram_[p + 0x100] = data;
		else
			nontrivial_ff_write(p, data, cc);

		if (writeCallback_)
			writeCallback_(mm_io_begin + p, (cc - basetime_) >> 1);
		if (cdCallback_ && !biosMode_) {
			CDMapResult map = CDMap(mm_io_begin + p);
			if (map.type != eCDLog_AddrType_None)
				cdCallback_(map.addr, map.type, eCDLog_Flags_Data);
		}
	}

	unsigned long event(unsigned long cycleCounter);
	unsigned long resetCounters(unsigned long cycleCounter);
	LoadRes loadROM(std::string const &romfile, unsigned flags);
	LoadRes loadROM(char const *romfiledata, unsigned romfilelength, unsigned flags);
	void setSaveDir(std::string const &dir) { cart_.setSaveDir(dir); }

	void setInputGetter(InputGetter *getInput, void *p) {
		getInput_ = getInput;
		getInputP_ = p;
	}

	void setReadCallback(MemoryCallback callback) {
		this->readCallback_ = callback;
	}

	void setWriteCallback(MemoryCallback callback) {
		this->writeCallback_ = callback;
	}

	void setExecCallback(MemoryCallback callback) {
		this->execCallback_ = callback;
	}

	void setCDCallback(CDCallback cdc) {
		this->cdCallback_ = cdc;
	}

	void setScanlineCallback(void (*callback)(), int sl) {
		lcd_.setScanlineCallback(callback, sl);
	}

	void setLinkCallback(void(*callback)()) {
		this->linkCallback_ = callback;
	}

	void setEndtime(unsigned long cc, unsigned long inc);
	void setBasetime(unsigned long cc) { basetime_ = cc; }

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

	void setCgbPalette(unsigned *lut) {
		lcd_.setCgbPalette(lut);
		sgb_.setCgbPalette(lut);
	}

	void setTimeMode(bool useCycles, unsigned long const cc) {
		cart_.setTimeMode(useCycles, cc);
	}
	void setRtcDivisorOffset(long const rtcDivisorOffset) { cart_.setRtcDivisorOffset(rtcDivisorOffset); }
	
	void setCartBusPullUpTime(unsigned long const cartBusPullUpTime) { cartBusPullUpTime_ = cartBusPullUpTime; }
	
	void getRtcRegs(unsigned long *dest, unsigned long cc) { cart_.getRtcRegs(dest, cc); }
	void setRtcRegs(unsigned long *src) { cart_.setRtcRegs(src); }

	int linkStatus(int which);

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

	template<bool isReader>void SyncState(NewState *ns);

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
	unsigned long lastCartBusUpdate_;
	unsigned long cartBusPullUpTime_; 
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
	unsigned char cartBus_;
	bool blanklcd_;
	bool biosMode_;
	bool agbFlag_;
	bool gbIsSgb_;
	unsigned long basetime_;
	bool stopped_;
	enum HdmaState { hdma_low, hdma_high, hdma_requested } haltHdmaState_;

	MemoryCallback readCallback_;
	MemoryCallback writeCallback_;
	MemoryCallback execCallback_;
	CDCallback cdCallback_;
	void(*linkCallback_)();
	bool linkCable_;
	bool linkClockTrigger_;

	void decEventCycles(IntEventId eventId, unsigned long dec);
	void oamDmaInitSetup();
	void updateOamDma(unsigned long cycleCounter);
	void startOamDma(unsigned long cycleCounter);
	void endOamDma(unsigned long cycleCounter);
	unsigned char const * oamDmaSrcPtr() const;
	unsigned long dma(unsigned long cc);
	unsigned nontrivial_ff_read(unsigned p, unsigned long cycleCounter);
	unsigned nontrivial_read(unsigned p, unsigned long cycleCounter);
	unsigned nontrivial_ff_peek(unsigned p, unsigned long cycleCounter);
	unsigned nontrivial_peek(unsigned p, unsigned long cycleCounter);
	void nontrivial_ff_write(unsigned p, unsigned data, unsigned long cycleCounter);
	void nontrivial_write(unsigned p, unsigned data, unsigned long cycleCounter);
	void updateSerial(unsigned long cc);
	void updateTimaIrq(unsigned long cc);
	void updateIrqs(unsigned long cc);
	bool isDoubleSpeed() const { return lcd_.isDoubleSpeed(); }
};

}

#endif
