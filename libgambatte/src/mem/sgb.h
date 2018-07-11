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

#ifndef SGB_H
#define SGB_H

#include "gbint.h"
#include <cstddef>

namespace gambatte {

struct SaveState;

class Sgb {
public:
	Sgb();
	void setStatePtrs(SaveState &state);
	void saveState(SaveState &state) const;
	void loadState(SaveState const &state);

	unsigned getJoypadIndex() { return joypadIndex; }

	void setVideoBuffer(uint_least32_t *videoBuf, std::ptrdiff_t pitch) {
		videoBuf_ = videoBuf;
		pitch_ = pitch;
	}

	void setTrueColors(bool trueColors) {
		trueColors_ = trueColors;
		refreshPalettes();
	}

	void onJoypad(unsigned data);
	void updateScreen();

private:
	unsigned char transfer;
	unsigned char packet[0x10];
	unsigned char command[0x10 * 7];
	unsigned char commandIndex;

	unsigned char joypadIndex;
	unsigned char joypadMask;

	uint_least32_t *videoBuf_;
	std::ptrdiff_t pitch_;
	bool trueColors_;

	unsigned short systemColors[512 * 4];
	unsigned short colors[4 * 4];
	unsigned long palette[4 * 4];
	unsigned char attributes[20 * 18];

	unsigned char pending;
	unsigned char pendingCount;
	unsigned char mask;

	enum Command {
		PAL01    = 0x00,
		PAL23    = 0x01,
		PAL03    = 0x02,
		PAL12    = 0x03,
		ATTR_BLK = 0x04,
		PAL_SET  = 0x0A,
		PAL_TRN  = 0x0B,
		MLT_REQ  = 0x11,
		MASK_EN  = 0x17
	};

	void handleTransfer(unsigned data);
	void onCommand();
	void onTransfer(unsigned char *frame);
	void refreshPalettes();

	void palnn(unsigned a, unsigned b);
	void attr_blk();
	void pal_set();
};

}

#endif
