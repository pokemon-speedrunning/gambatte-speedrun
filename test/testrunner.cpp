#include "gambatte.h"
#include "transfer_ptr.h"
#include <png.h>
#include <algorithm>
#include <string>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>

namespace {

struct FileDeleter { static void del(std::FILE *f) { if (f) std::fclose(f); } };
typedef transfer_ptr<std::FILE, FileDeleter> file_ptr;

unsigned const gb_width = 160, gb_height = 144;
std::size_t const samples_per_frame = 35112;
std::size_t const audiobuf_size = samples_per_frame + 2064;
std::size_t const framebuf_size = gb_width * gb_height;

bool useDmgBios = true;
bool useCgbBios = true;

static void readPng(gambatte::uint_least32_t out[], std::FILE &file) {
	struct PngContext {
		png_structp png;
		png_infop info;
		png_infop endinfo;

		PngContext()
		: png(png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0))
		, info(png ? png_create_info_struct(png) : 0)
		, endinfo(png ? png_create_info_struct(png) : 0)
		{
			assert(png);
			assert(info);
			assert(endinfo);
		}

		~PngContext() {
			png_destroy_read_struct(&png, &info, &endinfo);
		}
	} const pngCtx;

	if (setjmp(png_jmpbuf(pngCtx.png)))
		std::abort();

	png_init_io(pngCtx.png, &file);
	png_read_png(pngCtx.png, pngCtx.info, 0, 0);

	assert(png_get_image_height(pngCtx.png, pngCtx.info) == gb_height);
	assert(png_get_rowbytes(pngCtx.png, pngCtx.info) == gb_width * 4);

	png_bytep const *const rows = png_get_rows(pngCtx.png, pngCtx.info);

	for (std::size_t y = 0; y < gb_height; ++y)
	for (std::size_t x = 0; x < gb_width; ++x) {
		out[y * gb_width + x] = rows[y][x * 4] << 16
			| rows[y][x * 4 + 1] << 8
			| rows[y][x * 4 + 2];
	}
}

static gambatte::uint_least32_t const * tileFromChar(char const c) {
	static gambatte::uint_least32_t const tiles[0x10 * 8 * 8] = {
		#define _ 0xF8F8F8
		#define O 0x000000

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,

		_,_,_,_,_,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,_,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,

		_,_,_,_,_,_,_,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,_,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,O,O,O,O,O,O,_,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,O,_,
		_,_,_,_,_,O,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,O,_,_,_,_,
		_,_,_,O,_,_,_,_,

		_,_,_,_,_,_,_,_,
		_,_,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,_,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,_,O,O,O,O,O,_,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,

		_,_,_,_,_,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,O,_,_,_,O,_,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,_,

		_,_,_,_,_,_,_,_,
		_,_,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,O,
		_,_,O,O,O,O,O,_,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,_,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,

		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_

		#undef O
		#undef _
	};

	unsigned num = isdigit(c) ? c - '0' : toupper(c) - 'A' + 0xA;
	return num < 0x10 ? tiles + num * 8*8 : 0;
}

static bool tilesAreEqual(
		gambatte::uint_least32_t const lhs[],
		gambatte::uint_least32_t const rhs[]) {
	for (unsigned y = 0; y < 8; ++y)
	for (unsigned x = 0; x < 8; ++x)
		if ((lhs[y * gb_width + x] & 0xF8F8F8) != rhs[y * 8 + x])
			return false;

	return true;
}

static bool frameBufferMatchesOut(gambatte::uint_least32_t const framebuf[], std::string const &out) {
	gambatte::uint_least32_t const *outTile;

	for (std::size_t i = 0; (outTile = tileFromChar(out[i])) != 0; ++i) {
		if (!tilesAreEqual(framebuf + i * 8, outTile))
			return false;
	}

	return true;
}

static bool frameBufsEqual(
		gambatte::uint_least32_t const lhs[],
		gambatte::uint_least32_t const rhs[]) {
	for (std::size_t i = 0; i < framebuf_size; ++i) {
		if ((lhs[i] ^ rhs[i]) & 0xF8F8F8)
			return false;
	}

	return true;
}

static bool evaluateStrTestResults(
		gambatte::uint_least32_t const audiobuf[],
		gambatte::uint_least32_t const framebuf[],
		std::string const &file,
		std::string const &outstr) {
	std::size_t const outpos = file.find(outstr);
	assert(outpos != std::string::npos);

	if (file.compare(outpos + outstr.size(), 6, "audio0") == 0) {
		if (std::count(audiobuf, audiobuf + samples_per_frame, audiobuf[0]) == samples_per_frame)
			return true;
	} else if (file.compare(outpos + outstr.size(), 6, "audio1") == 0) {
		if (std::count(audiobuf, audiobuf + samples_per_frame, audiobuf[0]) != samples_per_frame)
			return true;
	} else {
		if (frameBufferMatchesOut(framebuf, file.substr(outpos + outstr.size())))
			return true;
	}

	std::printf("\nFAILED: %s %s\n", file.c_str(), outstr.c_str());
	return false;
}

static void runTestRom(
		gambatte::uint_least32_t framebuf[],
		gambatte::uint_least32_t audiobuf[],
		std::string const &file,
		bool const cgb,
		bool const agb) {
	gambatte::GB gb;

	if (cgb && useCgbBios) {
		if (gb.loadBios("bios.gbc", 0x900, 0x31672598)) {
			std::fprintf(stderr, "Failed to load bios image file bios.gbc, using post BIOS state instead.\n");
			useCgbBios = false;
		}
	} else if (!cgb && useDmgBios) {
		if (gb.loadBios("bios.gb", 0x100, 0x580A33B9)) {
			std::fprintf(stderr, "Failed to load bios image file bios.gb, using post BIOS state instead.\n");
			useDmgBios = false;
		}
	}
	
	int flags = (cgb * gambatte::GB::LoadFlag::CGB_MODE)
	| ((cgb && agb) * gambatte::GB::LoadFlag::GBA_FLAG)
	| (!((cgb && useCgbBios) || (!cgb && useDmgBios)) * gambatte::GB::LoadFlag::NO_BIOS);

	if (gb.load(file, flags)) {
		std::fprintf(stderr, "Failed to load ROM image file %s\n", file.c_str());
		std::abort();
	}

	if (cgb) {
		unsigned lut[32768];
		int i = 0;
		for (int b = 0; b < 32; b++)
			for (int g = 0; g < 32; g++)
				for (int r = 0; r < 32; r++)
					lut[i++] = ((r * 3 + g * 2 + b * 11) >> 1) | ((g * 3 + b) << 1) << 8 | ((r * 13 + g * 2 + b) >> 1) << 16 | 255 << 24;

		gb.setCgbPalette(lut);
	} else {
		for (int i = 0; i < 12; ++i)
			gb.setDmgPaletteColor(i / 4, i % 4, (3 - (i & 3)) * 85 * 0x010101ul);
	}

	std::putchar(cgb ? (agb ? 'a' : 'c') : 'd');
	std::fflush(stdout);

	long biosLength = cgb ? 186 : 334;
	if (flags & gambatte::GB::LoadFlag::NO_BIOS)
		biosLength = 0;

	long samplesLeft = samples_per_frame * (biosLength + 15);

	while (samplesLeft >= 0) {
		std::size_t samples = samples_per_frame;
		gb.runFor(framebuf, gb_width, audiobuf, samples);
		samplesLeft -= samples;
	}
}

static bool runStrTest(std::string const &romfile, bool cgb, bool agb, std::string const &outstr) {
	gambatte::uint_least32_t audiobuf[audiobuf_size];
	gambatte::uint_least32_t framebuf[framebuf_size];
	runTestRom(framebuf, audiobuf, romfile, cgb, agb);
	return evaluateStrTestResults(audiobuf, framebuf, romfile, outstr);
}

static bool runPngTest(std::string const &romfile, bool cgb, bool agb, std::FILE &pngfile) {
	gambatte::uint_least32_t audiobuf[audiobuf_size];
	gambatte::uint_least32_t framebuf[framebuf_size];
	runTestRom(framebuf, audiobuf, romfile, cgb, agb);

	gambatte::uint_least32_t pngbuf[framebuf_size];
	readPng(pngbuf, pngfile);

	if (!frameBufsEqual(framebuf, pngbuf)) {
		std::printf("\nFAILED: %s png\n", romfile.c_str());
		return false;
	}

	return true;
}

static std::string extensionStripped(std::string const &s) {
	return s.substr(0, s.rfind('.'));
}

static file_ptr openFile(std::string const &filename) {
	return file_ptr(std::fopen(filename.c_str(), "rb"));
}

} // anon ns

int main(int const argc, char *argv[]) {
	int totalNumTestsRun = 0;
	int totalNumTestsSucceeded = 0;
	int dmgNumTestsRun = 0;
	int dmgNumTestsSucceeded = 0;
	int cgbNumTestsRun = 0;
	int cgbNumTestsSucceeded = 0;
	int agbNumTestsRun = 0;
	int agbNumTestsSucceeded = 0;

	for (int i = 1; i < argc; ++i) {
		std::string const s = extensionStripped(argv[i]);
		char const *dmgout = 0;
		char const *cgbout = 0;
		char const *agbout = 0; // FIXME: Actual AGB results

		if (s.find("dmg08_cgb04c_out") != std::string::npos) {
			dmgout = cgbout = agbout = "dmg08_cgb04c_out";
		} else {
			if (s.find("dmg08_out") != std::string::npos) {
				dmgout = "dmg08_out";

				if (s.find("cgb04c_out") != std::string::npos)
					cgbout = agbout = "cgb04c_out";
			} else if (s.find("_out") != std::string::npos)
				cgbout = agbout = "_out";
		}
		if (agbout) {
			if (runStrTest(argv[i],  true,  true, agbout)) {
				++totalNumTestsSucceeded;
				++agbNumTestsSucceeded;
			}
			++agbNumTestsRun;
			++totalNumTestsRun;
		}
		if (cgbout) {
			if (runStrTest(argv[i],  true, false, cgbout)) {
				++totalNumTestsSucceeded;
				++cgbNumTestsSucceeded;
			}
			++totalNumTestsRun;
			++cgbNumTestsRun;
		}
		if (dmgout) {
			if (runStrTest(argv[i], false, false, dmgout)) {
				++totalNumTestsSucceeded;
				++dmgNumTestsSucceeded;
			}
			++totalNumTestsRun;
			++dmgNumTestsRun;
		}

		if (file_ptr png = openFile(s + "_dmg08_cgb04c.png")) { // FIXME: confirm if agb is identical here
			if (runPngTest(argv[i],  true,  true, *png)) {
				++totalNumTestsSucceeded;
				++agbNumTestsSucceeded;
			}
			if (runPngTest(argv[i],  true, false, *png)) {
				++totalNumTestsSucceeded;
				++cgbNumTestsSucceeded;
			}
			if (runPngTest(argv[i], false, false, *png)) {
				++totalNumTestsSucceeded;
				++dmgNumTestsSucceeded;
			}
			totalNumTestsRun += 3;
			++agbNumTestsRun;
			++cgbNumTestsRun;
			++dmgNumTestsRun;
		} else {
			if (file_ptr p = openFile(s + "_cgb04c.png")) { // FIXME: we need agb images
				if (runPngTest(argv[i],  true,  true, *p)) {
					++totalNumTestsSucceeded;
					++agbNumTestsSucceeded;
				}
				++totalNumTestsRun;
				++agbNumTestsRun;
			}
			if (file_ptr p = openFile(s + "_cgb04c.png")) {
				if (runPngTest(argv[i],  true, false, *p)) {
					++totalNumTestsSucceeded;
					++cgbNumTestsSucceeded;
				}
				++totalNumTestsRun;
				++cgbNumTestsRun;
			}
			if (file_ptr p = openFile(s + "_dmg08.png")) {
				if (runPngTest(argv[i], false, false, *p)) {
					++totalNumTestsSucceeded;
					++dmgNumTestsSucceeded;
				}
				++totalNumTestsRun;
				++dmgNumTestsRun;
			}
		}
	}

	std::printf("\n\nRan %d total tests.\n", totalNumTestsRun);
	std::printf("%d total failures.\n", totalNumTestsRun - totalNumTestsSucceeded);

	std::printf("\nRan %d AGB tests.\n", agbNumTestsRun);
	std::printf("%d AGB failures.\n", agbNumTestsRun - agbNumTestsSucceeded);

	std::printf("\nRan %d CGB tests.\n", cgbNumTestsRun);
	std::printf("%d CGB failures.\n", cgbNumTestsRun - cgbNumTestsSucceeded);

	std::printf("\nRan %d DMG tests.\n", dmgNumTestsRun);
	std::printf("%d DMG failures.\n\n", dmgNumTestsRun - dmgNumTestsSucceeded);
	
	return (cgbNumTestsRun - cgbNumTestsSucceeded) || (dmgNumTestsRun - dmgNumTestsSucceeded);
}
