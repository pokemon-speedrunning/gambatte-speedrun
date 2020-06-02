//
//   Copyright (C) 2011 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef PSRDATA_H
#define PSRDATA_H

#include <gambatte.h>
#include <QString>

struct GambatteBiosInfo {
	std::size_t size;
	unsigned crc; // CRC-32 after byte @ 0xFE is set to 0x00

	QString name;
	QString filter;
	QString key;
};

const GambatteBiosInfo dmg_bios_info = { 0x100, 0x580A33B9, "DMG", "*.gb",  "biosFilenameDMG" };
const GambatteBiosInfo sgb_bios_info = { 0x100, 0xED48E98E, "SGB", "*.sgb", "biosFilenameSGB" };
const GambatteBiosInfo cgb_bios_info = { 0x900, 0x31672598, "GBC", "*.gbc", "biosFilename"    };

struct GambattePlatformInfo {
	unsigned loadFlags;
	GambatteBiosInfo biosInfo;
	unsigned resetFade;  // uniform n=[0,35111] will be added
	unsigned resetStall; // must be multiple of (2 << 14)
};

enum GambattePlatform {
	PLATFORM_GB,
	PLATFORM_GBC,
	PLATFORM_GBA,
	PLATFORM_GBP,
	PLATFORM_SGB,

	PLATFORM_COUNT
};

const GambattePlatformInfo gambatte_platform_info[PLATFORM_COUNT] = {
	[PLATFORM_GB] = {
		.loadFlags = 0,
		.biosInfo = dmg_bios_info,
		.resetFade = 0,
		.resetStall = 0
	},

	[PLATFORM_GBC] = {
		.loadFlags = gambatte::GB::CGB_MODE,
		.biosInfo = cgb_bios_info,
		.resetFade = 0,
		.resetStall = 0
	},

	[PLATFORM_GBA] = {
		.loadFlags = gambatte::GB::CGB_MODE | gambatte::GB::GBA_FLAG,
		.biosInfo = cgb_bios_info, // patched with GBA_FLAG
		.resetFade = 0,
		.resetStall = 0
	},

	[PLATFORM_GBP] = {
		.loadFlags = gambatte::GB::CGB_MODE | gambatte::GB::GBA_FLAG,
		.biosInfo = cgb_bios_info, // patched with GBA_FLAG
		.resetFade = 1234567,
		.resetStall = 101 * (2 << 14)
	},

	[PLATFORM_SGB] = {
		.loadFlags = gambatte::GB::SGB_MODE,
		.biosInfo = sgb_bios_info,
		.resetFade = 0,
		.resetStall = 128 * (2 << 14)
	}
};

#define DEFAULT_GAMBATTE_PLATFORM PLATFORM_GBP

struct GambatteGoodromInfo {
	std::string label;
	std::string title; // from cartridge header
	unsigned crc;      // CRC-32
	int savBreakpoint;
};

const std::string psr_label("<PSR>");

const GambatteGoodromInfo gambatte_goodroms[] = {
	{ psr_label, "POKEMON RED",      0x9F7FDD53, 0x1C7847 },
	{ psr_label, "POKEMON BLUE",     0xD6DA8A1A, 0x1C7847 },
	{ psr_label, "POKEMON YELLOW",   0x7D527D62, 0x1C7B90 },
	{ psr_label, "POKEMON_GLDAAUE",  0x6BDE3C3E, 0x054D0D },
	{ psr_label, "POKEMON_SLVAAXE",  0x8AD48636, 0x054D0D },
	{ psr_label, "PM_CRYSTAL",       0xEE6F5188, 0x054C6A }, // 1.0
	{ psr_label, "PM_CRYSTAL",       0x3358E30A, 0x054C6A }, // 1.1
	{ psr_label, "PM_CRYSTAL",       0xBB6DD80C, 0x054C6A }, // AU
	{ psr_label, "POKECARD",         0x81069D53, 0x04524C }  // USA
};

#endif
