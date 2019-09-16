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

GBEXPORT int gambatte_loadbios(GB *g, char const *biosfile, unsigned size, unsigned crc) {
	return g->loadBios(biosfile, size, crc);
}

GBEXPORT int gambatte_runfor(GB *g, unsigned *videoBuf, int pitch, unsigned *audioBuf, unsigned *samples) {
	return g->runFor(videoBuf, pitch, audioBuf, *(std::size_t *)samples);
}

GBEXPORT void gambatte_reset(GB *g) {
	g->reset();
}

GBEXPORT void gambatte_setinputgetter(GB *g, InputGetter *getInput, void *p) {
	g->setInputGetter(getInput, p);
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

GBEXPORT void gambatte_getregs(GB *g, int *dest) {
	g->getRegs(dest);
}

GBEXPORT void gambatte_setregs(GB *g, int *src) {
	g->setRegs(src);
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

}
