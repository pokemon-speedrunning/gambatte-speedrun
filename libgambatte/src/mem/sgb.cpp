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

#include "sgb.h"
#include "../savestate.h"
#include <cstring>

namespace gambatte {

static unsigned long gbcToRgb32(unsigned const bgr15, bool trueColor) {
	unsigned long const r = bgr15       & 0x1F;
	unsigned long const g = bgr15 >>  5 & 0x1F;
	unsigned long const b = bgr15 >> 10 & 0x1F;
    
    if (trueColor) {
        return (r << 19) | (g << 11) | (b << 3);
    }

	return ((r * 13 + g * 2 + b) >> 1) << 16
	     | (g * 3 + b) << 9
	     | (r * 3 + g * 2 + b * 11) >> 1;
}

Sgb::Sgb()
: transfer(0xFF)
, pending(0xFF)
{
}

void Sgb::setStatePtrs(SaveState &state) {
	state.mem.sgb.packet.set(packet, sizeof packet);
	state.mem.sgb.command.set(command, sizeof command);
	state.mem.sgb.systemColors.set(systemColors, sizeof systemColors / sizeof systemColors[0]);
	state.mem.sgb.colors.set(colors, sizeof colors / sizeof colors[0]);
	state.mem.sgb.attributes.set(attributes, sizeof attributes);
}

void Sgb::saveState(SaveState &state) const {
	state.mem.sgb.transfer = transfer;
	state.mem.sgb.commandIndex = commandIndex;
	state.mem.sgb.joypadIndex = joypadIndex;
	state.mem.sgb.joypadMask = joypadMask;
	state.mem.sgb.pending = pending;
	state.mem.sgb.pendingCount = pendingCount;
	state.mem.sgb.mask = mask;
}

void Sgb::loadState(SaveState const &state) {
	transfer = state.mem.sgb.transfer;
	commandIndex = state.mem.sgb.commandIndex;
	joypadIndex = state.mem.sgb.joypadIndex;
	joypadMask = state.mem.sgb.joypadMask;
	pending = state.mem.sgb.pending;
	pendingCount = state.mem.sgb.pendingCount;
	mask = state.mem.sgb.mask;

	refreshPalettes();
}

void Sgb::onJoypad(unsigned data) {
	handleTransfer(data);

	if ((data & 0x20) == 0)
		joypadIndex = (joypadIndex + 1) & joypadMask;
}

void Sgb::updateScreen() {
	unsigned char frame[160 * 144];
	
	for (int j = 0; j < 144; j++) {
		for (int i = 0; i < 160; i++)
			frame[j * 160 + i] = 3 - (videoBuf_[j * pitch_ + i] >> 4 & 3);
	}

	if (pending != 0xFF && --pendingCount == 0)
		onTransfer(frame);

	if (mask != 0)
		std::memset(frame, 0, sizeof frame);

	for (int j = 0; j < 144; j++) {
		for (int i = 0; i < 160; i++) {
			unsigned attribute = attributes[(j / 8) * 20 + (i / 8)];
			videoBuf_[j * pitch_ + i] = palette[attribute * 4 + frame[j * 160 + i]];
		}
	}
}

void Sgb::handleTransfer(unsigned data) {
	if ((data & 0x30) == 0) {
		std::memset(packet, 0, sizeof packet);
		transfer = 0;
	} else if (transfer != 0xFF) {
		if (transfer < 128) {
			packet[transfer >> 3] |= ((data & 0x20) == 0) << (transfer & 7);
			transfer++;
		} else if ((data & 0x10) == 0) {
			transfer = 0xFF;
			std::memcpy(command + commandIndex * sizeof packet, packet, sizeof packet);

			if (++commandIndex == (command[0] & 7)) {
				onCommand();
				commandIndex = 0;
			}
		}
	}
}

void Sgb::onCommand() {
	switch (command[0] >> 3) {
	case PAL01:
		palnn(0, 1);
		break;
	case PAL23:
		palnn(2, 3);
		break;
	case PAL03:
		palnn(0, 3);
		break;
	case PAL12:
		palnn(1, 2);
		break;
	case ATTR_BLK:
		attr_blk();
		break;
	case PAL_SET:
		pal_set();
		break;
	case PAL_TRN:
		pending = PAL_TRN;
		pendingCount = 4;
		break;
	case MLT_REQ:
		joypadMask = command[1];
		joypadIndex &= joypadMask;
		break;
	case MASK_EN:
		mask = command[1];
		break;
	}
}

void Sgb::onTransfer(unsigned char *frame) {
	unsigned char vram[4096];
	unsigned pos = 0;

	for (int i = 0; i < 256; i++) {
		unsigned char *src = frame + ((i / 20) * 160 + (i % 20)) * 8;

		for (int j = 0; j < 8; j++) {
			unsigned char a = (src[0] & 1) << 7;
			unsigned char b = (src[7] & 2) >> 1;

			for (int k = 0; k < 6; k++) {
				a |= (src[7 - k] & 1) << k;
				b |= (src[6 - k] & 2) << k;
			}

			vram[pos++] = a;
			vram[pos++] = b;
			src += 160;
		}
	}

	switch (pending) {
	case PAL_TRN:
		for (int i = 0; i < 2048; i++)
			systemColors[i] = vram[i * 2] | vram[i * 2 + 1] << 8;
		break;
	}

	pending = 0xFF;
}

void Sgb::refreshPalettes() {
	for (int i = 0; i < 16; i++)
		palette[i] = gbcToRgb32(colors[i * ((i & 3) != 0)], trueColors_);
}

void Sgb::palnn(unsigned a, unsigned b) {
	unsigned short color[7];

	for (int i = 0; i < 7; i++)
		color[i] = command[1 + i * 2] | command[1 + i * 2 + 1] << 8;

	colors[0] = color[0];
	colors[a * 4 + 1] = color[1];
	colors[a * 4 + 2] = color[2];
	colors[a * 4 + 3] = color[3];
	colors[b * 4 + 1] = color[4];
	colors[b * 4 + 2] = color[5];
	colors[b * 4 + 3] = color[6];

	refreshPalettes();
}

void Sgb::attr_blk() {
	for (int i = 0; i < command[1]; i++) {
		unsigned ctrl = command[2 + i * 6 + 0];
		unsigned pals = command[2 + i * 6 + 1];
		unsigned x1 = command[2 + i * 6 + 2];
		unsigned y1 = command[2 + i * 6 + 3];
		unsigned x2 = command[2 + i * 6 + 4];
		unsigned y2 = command[2 + i * 6 + 5];

		unsigned inside = ctrl & 1;
		unsigned line = ctrl & 2;
		unsigned outside = ctrl & 4;

		unsigned insidepal = pals & 3;
		unsigned linepal = pals >> 2 & 3;
		unsigned outsidepal = pals >> 4 & 3;

		if (ctrl == 1) {
			line = 2;
			linepal = insidepal;
		} else if (ctrl == 4) {
			line = 2;
			linepal = outsidepal;
		}

		for (int j = 0; j < 20 * 18; j++) {
			unsigned x = j % 20;
			unsigned y = j / 20;

			if (x < x1 || y < y1 || x > x2 || y > y2) {
				if (outside)
					attributes[j] = outsidepal;
			} else if (x == x1 || y == y1 || x == x2 || y == y2) {
				if (line)
					attributes[j] = linepal;
			} else if (inside)
				attributes[j] = insidepal;
		}
	}
}

void Sgb::pal_set() {
	for (int i = 0; i < 4; i++) {
		unsigned p = command[1 + i * 2] | command[1 + i * 2 + 1] << 8;

		colors[0] = systemColors[p * 4 + 0];
		colors[i * 4 + 1] = systemColors[p * 4 + 1];
		colors[i * 4 + 2] = systemColors[p * 4 + 2];
		colors[i * 4 + 3] = systemColors[p * 4 + 3];
	}

	refreshPalettes();
}

}
