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

#include "cinterface.h"
#include "gambatte.h"
#include "newstate.h"

#include <cstdlib>
#include <cstring>

// new is actually called in a few different places, so replace all of them for determinism guarantees
void *operator new(std::size_t n) {
	void *p = std::malloc(n);
	std::memset(p, 0, n);
	return p;
}

void operator delete(void *p) {
	std::free(p);
}

// this is so the compiler will shut up really
void operator delete(void *p, std::size_t) {
	std::free(p);
}

namespace {

using namespace gambatte;

GBEXPORT int gambatte_revision() {
	return REVISION;
}

GBEXPORT GB * gambatte_create() {
	return new GB();
}

GBEXPORT void gambatte_destroy(GB *g) {
	delete g;
}

GBEXPORT int gambatte_load(GB *g, char const *romfile, unsigned flags) {
	return g->load(romfile, flags);
}

GBEXPORT int gambatte_loadbuf(GB *g, char const *romfiledata, unsigned romfilelength, unsigned flags) {
	return g->load(romfiledata, romfilelength, flags);
}

GBEXPORT int gambatte_loadbios(GB *g, char const *biosfile, unsigned size, unsigned crc) {
	return g->loadBios(biosfile, size, crc);
}

GBEXPORT int gambatte_loadbiosbuf(GB *g, char const *biosfiledata, unsigned size) {
	return g->loadBios(biosfiledata, size);
}

GBEXPORT int gambatte_runfor(GB *g, unsigned *videoBuf, int pitch, unsigned *audioBuf, unsigned *samples) {
	return g->runFor(videoBuf, pitch, audioBuf, *(std::size_t *)samples);
}

GBEXPORT int gambatte_altrunfor(GB *g, short *soundbuf, unsigned *samples) {
	std::size_t sampv = *samples;
	int ret = g->runFor((unsigned int *) soundbuf, sampv);
	*samples = sampv;
	return ret;
}

GBEXPORT void gambatte_blitto(GB *g, unsigned int *videobuf, int pitch) {
	g->blitTo((unsigned int *)videobuf, pitch);
}

GBEXPORT void gambatte_setlayers(GB *g, unsigned mask) {
	g->setLayers(mask);
}

GBEXPORT void gambatte_settimemode(GB *g, bool useCycles) {
	g->setTimeMode(useCycles);
}

GBEXPORT void gambatte_setrtcdivisoroffset(GB *g, int rtcDivisorOffset) {
	g->setRtcDivisorOffset(rtcDivisorOffset);
}

GBEXPORT void gambatte_reset(GB *g, unsigned samplesToStall) {
	g->reset(samplesToStall);
}

GBEXPORT void gambatte_setdmgpalettecolor(GB *g, unsigned palnum, unsigned colornum, unsigned rgb32) {
	g->setDmgPaletteColor(palnum, colornum, rgb32);
}

GBEXPORT void gambatte_setcgbpalette(GB *g, unsigned *lut) {
	g->setCgbPalette(lut);
}

GBEXPORT void gambatte_setinputgetter(GB *g, InputGetter *getInput, void *p) {
	g->setInputGetter(getInput, p);
}

GBEXPORT void gambatte_setreadcallback(GB *g, MemoryCallback callback) {
	g->setReadCallback(callback);
}

GBEXPORT void gambatte_setwritecallback(GB *g, MemoryCallback callback) {
	g->setWriteCallback(callback);
}

GBEXPORT void gambatte_setexeccallback(GB *g, MemoryCallback callback) {
	g->setExecCallback(callback);
}

GBEXPORT void gambatte_setcdcallback(GB *g, CDCallback cdc) {
	g->setCDCallback(cdc);
}

GBEXPORT void gambatte_settracecallback(GB *g, void (*callback)(void *)) {
	g->setTraceCallback(callback);
}

GBEXPORT void gambatte_setscanlinecallback(GB *g, void (*callback)(), int sl) {
	g->setScanlineCallback(callback, sl);
}

GBEXPORT void gambatte_setlinkcallback(GB *g, void(*callback)()) {
	g->setLinkCallback(callback);
}

GBEXPORT void gambatte_setcartbuspulluptime(GB *g, unsigned cartBusPullUpTime) {
	g->setCartBusPullUpTime(cartBusPullUpTime);
}

GBEXPORT int gambatte_iscgb(GB *g) {
	return g->isCgb();
}

GBEXPORT int gambatte_iscgbdmg(GB *g) {
	return g->isCgbDmg();
}

GBEXPORT int gambatte_isloaded(GB *g) {
	return g->isLoaded();
}

GBEXPORT void gambatte_savesavedata(GB *g, char *dest, bool isDeterministic) {
	g->saveSavedata(dest, isDeterministic);
}

GBEXPORT void gambatte_loadsavedata(GB *g, char const *data, bool isDeterministic) {
	g->loadSavedata(data, isDeterministic);
}

GBEXPORT int gambatte_savesavedatalength(GB *g, bool isDeterministic) {
	return g->saveSavedataLength(isDeterministic);
}

GBEXPORT int gambatte_newstatelen(GB *g) {
	NewStateDummy dummy;
	g->SyncState<false>(&dummy);
	return dummy.GetLength();
}

GBEXPORT int gambatte_newstatesave(GB *g, char *data, int len) {
	NewStateExternalBuffer saver(data, len);
	g->SyncState<false>(&saver);
	return !saver.Overflow() && saver.GetLength() == len;
}

GBEXPORT int gambatte_newstateload(GB *g, char const *data, int len) {
	NewStateExternalBuffer loader((char *)data, len);
	g->SyncState<true>(&loader);
	return !loader.Overflow() && loader.GetLength() == len;
}

GBEXPORT void gambatte_newstatesave_ex(GB *g, FPtrs *ff) {
	NewStateExternalFunctions saver(ff);
	g->SyncState<false>(&saver);
}

GBEXPORT void gambatte_newstateload_ex(GB *g, FPtrs *ff) {
	NewStateExternalFunctions loader(ff);
	g->SyncState<true>(&loader);
}

GBEXPORT void gambatte_romtitle(GB *g, char *dest) {
	std::strcpy(dest, g->romTitle().c_str());
}

GBEXPORT int gambatte_getmemoryarea(GB *g, int which, unsigned char **data, int *length) {
	return g->getMemoryArea(which, data, length);
}

GBEXPORT unsigned gambatte_savestate(GB *g, unsigned const *videoBuf, int pitch, char *stateBuf) {
	return g->saveState(videoBuf, pitch, stateBuf);
}

GBEXPORT bool gambatte_loadstate(GB *g, char const *stateBuf, unsigned size) {
	return g->loadState(stateBuf, size);
}

GBEXPORT unsigned char gambatte_cpuread(GB *g, unsigned short addr) {
	return g->externalRead(addr);
}

GBEXPORT void gambatte_cpuwrite(GB *g, unsigned short addr, unsigned char val) {
	g->externalWrite(addr, val);
}

GBEXPORT int gambatte_linkstatus(GB *g, int which) {
	return g->linkStatus(which);
}

GBEXPORT void gambatte_getregs(GB *g, int *dest) {
	g->getRegs(dest);
}

GBEXPORT void gambatte_setregs(GB *g, int *src) {
	g->setRegs(src);
}

GBEXPORT void gambatte_getrtcregs(GB *g, unsigned long *dest) {
	g->getRtcRegs(dest);
}

GBEXPORT void gambatte_setrtcregs(GB *g, unsigned long *src) {
	g->setRtcRegs(src);
}

GBEXPORT void gambatte_setinterruptaddresses(GB *g, int *addrs, int numAddrs) {
	g->setInterruptAddresses(addrs, numAddrs);
}

GBEXPORT int gambatte_gethitinterruptaddress(GB *g) {
	return g->getHitInterruptAddress();
}

GBEXPORT unsigned gambatte_timenow(GB *g) {
	return g->timeNow();
}

GBEXPORT int gambatte_getdivstate(GB *g) {
    return g->getDivState();
}

GBEXPORT void gambatte_setspeedupflags(GB *g, unsigned flags) {
	g->setSpeedupFlags(flags);
}

}
