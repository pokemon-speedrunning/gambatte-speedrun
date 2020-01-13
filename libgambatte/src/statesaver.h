//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef STATESAVER_H
#define STATESAVER_H

#include "gbint.h"

#include <cstddef>
#include <string>

namespace gambatte {

struct SaveState;

class StateSaver {
public:
	enum { ss_shift = 2 };
	enum { ss_div = 1 << 2 };
	enum { ss_width = 160 >> ss_shift };
	enum { ss_height = 144 >> ss_shift };

	static std::size_t saveState(SaveState const &state,
			uint_least32_t const *videoBuf, std::ptrdiff_t pitch,
			char *stateBuf, int mode);
	static bool loadState(SaveState &state,
			char const *stateBuf, std::size_t size, bool checkMode, int mode);

	static bool saveState(SaveState const &state,
			uint_least32_t const *videoBuf, std::ptrdiff_t pitch,
			std::string const &filename, int mode);
	static bool loadState(SaveState &state,
			std::string const &filename, bool checkMode, int mode);

private:
	StateSaver();
};

}

#endif
