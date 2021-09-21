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

#include "gambattesource.h"
#include "videolink/rgb32conv.h"
#include "videolink/vfilterinfo.h"

namespace {

using namespace gambatte;

struct ButtonInfo {
	char const *label;
	char const *category;
	int defaultKey;
	int defaultAltKey;
	unsigned char defaultFpp;
};

static ButtonInfo const buttonInfoGbUp[4] = {
{ "Up", "Game", Qt::Key_Up, 0, 0 },
{ "Up", "Player 2 (SGB)", 0, 0, 0 },
{ "Up", "Player 3 (SGB)", 0, 0, 0 },
{ "Up", "Player 4 (SGB)", 0, 0, 0 } };
static ButtonInfo const buttonInfoGbDown[4] = {
{ "Down", "Game", Qt::Key_Down, 0, 0 },
{ "Down", "Player 2 (SGB)", 0, 0, 0 },
{ "Down", "Player 3 (SGB)", 0, 0, 0 },
{ "Down", "Player 4 (SGB)", 0, 0, 0 } };
static ButtonInfo const buttonInfoGbLeft[4] = {
{ "Left", "Game", Qt::Key_Left, 0, 0 },
{ "Left", "Player 2 (SGB)", 0, 0, 0 },
{ "Left", "Player 3 (SGB)", 0, 0, 0 },
{ "Left", "Player 4 (SGB)", 0, 0, 0 } };
static ButtonInfo const buttonInfoGbRight[4] = {
{ "Right", "Game", Qt::Key_Right, 0, 0 },
{ "Right", "Player 2 (SGB)", 0, 0, 0 },
{ "Right", "Player 3 (SGB)", 0, 0, 0 },
{ "Right", "Player 4 (SGB)", 0, 0, 0 } };
static ButtonInfo const buttonInfoGbA[4] = {
{ "A", "Game", Qt::Key_D, 0, 0 },
{ "A", "Player 2 (SGB)", 0, 0, 0 },
{ "A", "Player 3 (SGB)", 0, 0, 0 },
{ "A", "Player 4 (SGB)", 0, 0, 0 } };
static ButtonInfo const buttonInfoGbB[4] = {
{ "B", "Game", Qt::Key_C, 0, 0 },
{ "B", "Player 2 (SGB)", 0, 0, 0 },
{ "B", "Player 3 (SGB)", 0, 0, 0 },
{ "B", "Player 4 (SGB)", 0, 0, 0 } };
static ButtonInfo const buttonInfoGbStart[4] = {
{ "Start", "Game", Qt::Key_Return, 0, 0 },
{ "Start", "Player 2 (SGB)", 0, 0, 0 },
{ "Start", "Player 3 (SGB)", 0, 0, 0 },
{ "Start", "Player 4 (SGB)", 0, 0, 0 } };
static ButtonInfo const buttonInfoGbSelect[4] = {
{ "Select", "Game", Qt::Key_Shift, 0, 0 },
{ "Select", "Player 2 (SGB)", 0, 0, 0 },
{ "Select", "Player 3 (SGB)", 0, 0, 0 },
{ "Select", "Player 4 (SGB)", 0, 0, 0 } };
static ButtonInfo const buttonInfoPause = { "Pause", "Play", Qt::Key_Pause, 0, 0 };
static ButtonInfo const buttonInfoFrameStep = { "Frame step", "Play", Qt::Key_F1, 0, 0 };
static ButtonInfo const buttonInfoDecFrameRate = { "Decrease frame rate", "Play", Qt::Key_F2, 0, 0 };
static ButtonInfo const buttonInfoIncFrameRate = { "Increase frame rate", "Play", Qt::Key_F3, 0, 0 };
static ButtonInfo const buttonInfoResetFrameRate = { "Reset frame rate", "Play", Qt::Key_F4, 0, 0 };
static ButtonInfo const buttonInfoFastFwd = { "Fast forward", "Play", Qt::Key_Tab, 0, 0 };
static ButtonInfo const buttonInfoReset = { "Hard reset", "Play", 0, 0, 0 };
static ButtonInfo const buttonInfoSaveState = { "Save state", "State", Qt::Key_F5, 0, 0 };
static ButtonInfo const buttonInfoLoadState = { "Load state", "State", Qt::Key_F8, 0, 0 };
static ButtonInfo const buttonInfoPrevState = { "Previous state slot", "State", Qt::Key_F6, 0, 0 };
static ButtonInfo const buttonInfoNextState = { "Next state slot", "State", Qt::Key_F7, 0, 0 };
static ButtonInfo const buttonInfoGbATurbo = { "Turbo A", "Other", 0, 0, 1 };
static ButtonInfo const buttonInfoGbBTurbo = { "Turbo B", "Other", 0, 0, 1 };
static ButtonInfo const buttonInfoQuit = { "Quit", "Other", 0, 0, 0 };

// separate non-template base to avoid obj code bloat
class ButtonBase : public InputDialog::Button {
public:
	// pass by ptr to disallow passing temporaries
	explicit ButtonBase(ButtonInfo const *info) : info_(info) {}
	virtual QString const label() const { return info_->label; }
	virtual QString const category() const { return info_->category; }
	virtual int defaultKey() const { return info_->defaultKey; }
	virtual int defaultAltKey() const { return info_->defaultAltKey; }
	virtual unsigned char defaultFpp() const { return info_->defaultFpp; }

private:
	ButtonInfo const *info_;
};

template<class T>
static void addButton(auto_vector<InputDialog::Button> &v,
		ButtonInfo const *info, void (T::*onPressed)(), T *t) {
	class Button : public ButtonBase {
	public:
		Button(ButtonInfo const *info, void (T::*onPressed)(), T *t)
		: ButtonBase(info), onPressed_(onPressed), t_(t)
		{
		}

		virtual void pressed() { (t_->*onPressed_)(); }
		virtual void released() {}

	private:
		void (T::*onPressed_)();
		T *t_;
	};

	v.push_back(new Button(info, onPressed, t));
}

template<class Action>
static void addButton(auto_vector<InputDialog::Button> &v, ButtonInfo const *info, Action action) {
	class Button : public ButtonBase {
	public:
		Button(ButtonInfo const *info, Action action)
		: ButtonBase(info), action_(action)
		{
		}

		virtual void pressed() { action_.pressed(); }
		virtual void released() { action_.released(); }

	private:
		Action action_;
	};

	v.push_back(new Button(info, action));
}

template<void (GambatteSource::*setPressed)(bool)>
struct CallSetterAct {
	GambatteSource *source;
	explicit CallSetterAct(GambatteSource *source) : source(source) {}
	void  pressed() const { (source->*setPressed)(true); }
	void released() const { (source->*setPressed)(false); }
};

template<bool button_id>
class GbDirAct {
public:
	GbDirAct(bool &buttonPressed, bool &lastPressedButtonId)
	: buttonPressed_(buttonPressed), lastPressedButtonId_(lastPressedButtonId)
	{
	}

	void  pressed() const { buttonPressed_ = true; lastPressedButtonId_ = button_id; }
	void released() const { buttonPressed_ = false; }

private:
	bool &buttonPressed_, &lastPressedButtonId_;
};

struct SetPressedAct {
	bool &v;
	explicit SetPressedAct(bool &v) : v(v) {}
	void pressed() const { v = true; }
	void released() const { v = false; }
};

enum { a_but, b_but, select_but, start_but, right_but, left_but, up_but, down_but };

} // anon ns

GambatteSource::GambatteSource()
: MediaSource(2064)
, inputGetter_(gb_)
, inputDialog_(createInputDialog())
, pxformat_(PixelBuffer::RGB32)
, vsrci_(0)
, inputState_()
, tryReset_(false)
, isResetting_(false)
, resetStage_(RESET_NOT)
, resetCounter_(0)
, resetFade_(1234567)
, resetStall_(101 * (2 << 14))
, sgbSampRm_(0)
, rng_(std::random_device()())
, dist35112_(0, 35111)
{
	std::memset(dpadUp_,       0, sizeof dpadUp_);
	std::memset(dpadDown_,     0, sizeof dpadDown_);
	std::memset(dpadLeft_,     0, sizeof dpadLeft_);
	std::memset(dpadRight_,    0, sizeof dpadRight_);
	std::memset(dpadUpLast_,   0, sizeof dpadUpLast_);
	std::memset(dpadLeftLast_, 0, sizeof dpadLeftLast_);
	gb_.setInputGetter((gambatte::InputGetter *)&GetInput::get, &inputGetter_);
	setBreakpoint(-1);
}

InputDialog * GambatteSource::createInputDialog() {
	auto_vector<InputDialog::Button> v;
#ifdef SHOW_PLATFORM_SGB
	for (unsigned i = 0; i < 4; i++) {
		addButton(v, &buttonInfoGbUp[i], GbDirAct<true>(dpadUp_[i], dpadUpLast_[i]));
		addButton(v, &buttonInfoGbDown[i], GbDirAct<false>(dpadDown_[i], dpadUpLast_[i]));
		addButton(v, &buttonInfoGbLeft[i], GbDirAct<true>(dpadLeft_[i], dpadLeftLast_[i]));
		addButton(v, &buttonInfoGbRight[i], GbDirAct<false>(dpadRight_[i], dpadLeftLast_[i]));
		addButton(v, &buttonInfoGbA[i], SetPressedAct(inputState_[i][a_but]));
		addButton(v, &buttonInfoGbB[i], SetPressedAct(inputState_[i][b_but]));
		addButton(v, &buttonInfoGbStart[i], SetPressedAct(inputState_[i][start_but]));
		addButton(v, &buttonInfoGbSelect[i], SetPressedAct(inputState_[i][select_but]));
	}
#else
	addButton(v, &buttonInfoGbUp[0], GbDirAct<true>(dpadUp_[0], dpadUpLast_[0]));
	addButton(v, &buttonInfoGbDown[0], GbDirAct<false>(dpadDown_[0], dpadUpLast_[0]));
	addButton(v, &buttonInfoGbLeft[0], GbDirAct<true>(dpadLeft_[0], dpadLeftLast_[0]));
	addButton(v, &buttonInfoGbRight[0], GbDirAct<false>(dpadRight_[0], dpadLeftLast_[0]));
	addButton(v, &buttonInfoGbA[0], SetPressedAct(inputState_[0][a_but]));
	addButton(v, &buttonInfoGbB[0], SetPressedAct(inputState_[0][b_but]));
	addButton(v, &buttonInfoGbStart[0], SetPressedAct(inputState_[0][start_but]));
	addButton(v, &buttonInfoGbSelect[0], SetPressedAct(inputState_[0][select_but]));
#endif
	addButton(v, &buttonInfoPause, &GambatteSource::emitPause, this);
	addButton(v, &buttonInfoFrameStep, &GambatteSource::emitFrameStep, this);
#ifdef ENABLE_TURBO_BUTTONS
	addButton(v, &buttonInfoDecFrameRate, &GambatteSource::emitDecFrameRate, this);
	addButton(v, &buttonInfoIncFrameRate, &GambatteSource::emitIncFrameRate, this);
	addButton(v, &buttonInfoResetFrameRate, &GambatteSource::emitResetFrameRate, this);
#endif
	addButton(v, &buttonInfoFastFwd, CallSetterAct<&GambatteSource::emitSetTurbo>(this));
	addButton(v, &buttonInfoReset, &GambatteSource::emitReset, this);
	addButton(v, &buttonInfoSaveState, &GambatteSource::emitSaveState, this);
	addButton(v, &buttonInfoLoadState, &GambatteSource::emitLoadState, this);
	addButton(v, &buttonInfoPrevState, &GambatteSource::emitPrevStateSlot, this);
	addButton(v, &buttonInfoNextState, &GambatteSource::emitNextStateSlot, this);
#ifdef ENABLE_TURBO_BUTTONS
	addButton(v, &buttonInfoGbATurbo, SetPressedAct(inputState_[8 + a_but]));
	addButton(v, &buttonInfoGbBTurbo, SetPressedAct(inputState_[8 + b_but]));
#endif
	addButton(v, &buttonInfoQuit, &GambatteSource::emitQuit, this);

	InputDialog *dialog = new InputDialog(v);
	QObject *o = dialog;
	o->setParent(this);
	return dialog;
}

std::vector<VideoDialog::VideoSourceInfo> const GambatteSource::generateVideoSourceInfos() {
	std::vector<VideoDialog::VideoSourceInfo> v(VfilterInfo::numVfilters());
	for (std::size_t i = 0; i < v.size(); ++i) {
		VfilterInfo const &vfi = VfilterInfo::get(i);
		VideoDialog::VideoSourceInfo vsi =
			{ vfi.handle, QSize(vfi.outWidth, vfi.outHeight) };
		v[i] = vsi;
	}

	return v;
}

void GambatteSource::keyPressEvent(QKeyEvent const *e) {
	inputDialog_->keyPress(e);
}

void GambatteSource::keyReleaseEvent(QKeyEvent const *e) {
	inputDialog_->keyRelease(e);
}

void GambatteSource::joystickEvent(SDL_Event const &e) {
	inputDialog_->joystickEvent(e);
}

void GambatteSource::clearKeyPresses() {
	inputDialog_->clearKeyPresses();
}

struct GambatteSource::GbVidBuf {
	uint_least32_t *pixels;
	std::ptrdiff_t pitch;

	GbVidBuf(uint_least32_t *pixels, std::ptrdiff_t pitch)
	: pixels(pixels), pitch(pitch)
	{
	}
};

GambatteSource::GbVidBuf GambatteSource::setPixelBuffer(
		void *pixels, PixelBuffer::PixelFormat format, std::ptrdiff_t pitch) {
	if (pxformat_ != format && pixels) {
		pxformat_ = format;
		cconvert_.reset();
		cconvert_.reset(Rgb32Conv::create(static_cast<Rgb32Conv::PixelFormat>(pxformat_),
		                                  VfilterInfo::get(vsrci_).outWidth,
		                                  VfilterInfo::get(vsrci_).outHeight));
	}

	if (VideoLink *gblink = vfilter_ ? vfilter_.get() : cconvert_.get())
		return GbVidBuf(static_cast<uint_least32_t *>(gblink->inBuf()), gblink->inPitch());

	return GbVidBuf(static_cast<uint_least32_t *>(pixels), pitch);
}

static void setGbDir(bool &a, bool &b,
		bool const aPressed, bool const bPressed, bool const /*preferA*/) {
	if (aPressed & bPressed) {
		a = false;
		b = false;
	} else {
		a = aPressed;
		b = bPressed;
	}
}

static unsigned packedInputState(bool const inputState[], std::size_t const len) {
	unsigned is = 0;
	for (std::size_t i = 0; i < len; ++i)
		is |= inputState[i] << (i & 7);

	return is;
}

static void * getpbdata(PixelBuffer const &pb, std::size_t vsrci) {
	return    pb.width  == VfilterInfo::get(vsrci).outWidth
	       && pb.height == VfilterInfo::get(vsrci).outHeight
	     ? pb.data
	     : 0;
}

template<class T>
static T * ptr_cast(void *p) { return static_cast<T *>(p); }

std::ptrdiff_t GambatteSource::update(
		PixelBuffer const &pb, qint16 *const soundBuf, std::size_t &samples) {
	GbVidBuf const gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci_), pb.pixelFormat, pb.pitch);
	if (samples < overUpdate) {
		samples = 0;
		return -1;
	}

	unsigned const players = gb_.isSgb() ? 4 : 1;
	for (unsigned i = 0; i < players; i++) {
		setGbDir(inputState_[i][up_but], inputState_[i][down_but],
				 dpadUp_[i], dpadDown_[i], dpadUpLast_[i]);
		setGbDir(inputState_[i][left_but], inputState_[i][right_but],
				 dpadLeft_[i], dpadRight_[i], dpadLeftLast_[i]);

		inputGetter_.is[i] = packedInputState(inputState_[i], sizeof inputState_[0] / sizeof inputState_[0][0]);
	}

	samples -= overUpdate;

	resetStepPre(samples);

	uint_least32_t *gbPixels;
	if (gb_.isSgb() && gbvidbuf.pitch == (256 + 3))
		gbPixels = &gbvidbuf.pixels[40 * (256 + 3) + 48];
	else
		gbPixels = gbvidbuf.pixels;

	std::ptrdiff_t const vidFrameSampleNo =
		runFor(gbPixels, gbvidbuf.pitch,
		       ptr_cast<quint32>(soundBuf), samples);

	if (gb_.isSgb()) {
		std::size_t sgbSamples;
		qint16 sgbSoundBuf[2048 * 2];
		unsigned t = 65 - sgbSampRm_;
		sgbSampRm_ = gb_.generateSgbSamples(sgbSoundBuf, sgbSamples);
		if (!soundBuf || !sgbSamples)
			goto end;

		short ls = sgbSoundBuf[0];
		short rs = sgbSoundBuf[1];
		for (unsigned i = 0; i < t; i++) {
			int sumLs = soundBuf[i * 2] + ls;
			soundBuf[(i * 2)] = std::clamp(sumLs, -0x8000, 0x7FFF);
			int sumRs = soundBuf[(i * 2) + 1] + rs;
			soundBuf[(i * 2) + 1] = std::clamp(sumRs, -0x8000, 0x7FFF);
		}
		for (unsigned i = 1; i < (sgbSamples - 1); i++, t += 65) {
			ls = sgbSoundBuf[i * 2];
			rs = sgbSoundBuf[(i * 2) + 1];
			for (unsigned j = 0; j < 65; j++) {
				int sumLs = soundBuf[(t + j) * 2] + ls;
				soundBuf[(t + j) * 2] = std::clamp(sumLs, -0x8000, 0x7FFF);
				int sumRs = soundBuf[((t + j) * 2) + 1] + rs;
				soundBuf[((t + j) * 2) + 1] = std::clamp(sumRs, -0x8000, 0x7FFF);
			}
		}
		ls = sgbSoundBuf[(sgbSamples - 1) * 2];
		rs = sgbSoundBuf[((sgbSamples - 1) * 2) + 1];
		for (unsigned i = 0; i < (samples - t); i++) {
			int sumLs = soundBuf[(t + i) * 2] + ls;
			soundBuf[(t + i) * 2] = std::clamp(sumLs, -0x8000, 0x7FFF);
			int sumRs = soundBuf[((t + i) * 2) + 1] + rs;
			soundBuf[((t + i) * 2) + 1] = std::clamp(sumRs, -0x8000, 0x7FFF);
		}
	}
end:

#ifdef ENABLE_INPUT_LOG
	inputLog_.push(samples, inputGetter_.is[0]);
#endif

	resetStepPost(pb, soundBuf, samples);

	if (vidFrameSampleNo >= 0) {
		inputDialog_->consumeAutoPress();
		if (gbPixels != gbvidbuf.pixels)
			gb_.updateScreenBorder(gbvidbuf.pixels, gbvidbuf.pitch);
	}

	return vidFrameSampleNo;
}

std::ptrdiff_t GambatteSource::runFor(
		uint_least32_t *pixels, std::ptrdiff_t pitch, quint32 *soundBuf, std::size_t &samples) {
	std::size_t targetSamples = samples;
	std::size_t actualSamples = 0;
	std::ptrdiff_t vidFrameSampleNo = -1;

	if (getBreakpoint() != -1)
		enableBreakpoint(true);

	while (actualSamples < targetSamples && vidFrameSampleNo < 0) {
		samples = targetSamples - actualSamples;
		std::ptrdiff_t const vfsn = gb_.runFor(pixels, pitch, soundBuf + actualSamples, samples);

		if (vfsn >= 0)
			vidFrameSampleNo = actualSamples + vfsn;

		actualSamples += samples;

		if (getHitAddress() != -1) {
			saveSavedata();
			enableBreakpoint(false);
		}
	}

	samples = actualSamples;
	return vidFrameSampleNo;
}

void GambatteSource::generateVideoFrame(PixelBuffer const &pb) {
	if (void *const pbdata = getpbdata(pb, vsrci_)) {
		setPixelBuffer(pbdata, pb.pixelFormat, pb.pitch);
		if (vfilter_) {
			void          *dstbuf   = cconvert_ ? cconvert_->inBuf()   : pbdata;
			std::ptrdiff_t dstpitch = cconvert_ ? cconvert_->inPitch() : pb.pitch;
			vfilter_->draw(dstbuf, dstpitch);
		}

		if (cconvert_)
			cconvert_->draw(pbdata, pb.pitch);
	}
}

void GambatteSource::setVideoSource(std::size_t const videoSourceIndex) {
	if (videoSourceIndex != vsrci_) {
		vsrci_ = videoSourceIndex;
		vfilter_.reset();
		cconvert_.reset();
		vfilter_.reset(VfilterInfo::get(vsrci_).create());
		cconvert_.reset(Rgb32Conv::create(static_cast<Rgb32Conv::PixelFormat>(pxformat_),
		                                  VfilterInfo::get(vsrci_).outWidth,
		                                  VfilterInfo::get(vsrci_).outHeight));
	}
}

void GambatteSource::saveState(PixelBuffer const &pb) {
	GbVidBuf gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci_), pb.pixelFormat, pb.pitch);
	gb_.saveState((gb_.isSgb() && gbvidbuf.pitch == (256 + 3)) ? &gbvidbuf.pixels[40 * (256 + 3) + 48] : gbvidbuf.pixels, gbvidbuf.pitch);
}

void GambatteSource::saveState(PixelBuffer const &pb, std::string const &filepath) {
	GbVidBuf gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci_), pb.pixelFormat, pb.pitch);
	gb_.saveState((gb_.isSgb() && gbvidbuf.pitch == (256 + 3)) ? &gbvidbuf.pixels[40 * (256 + 3) + 48] : gbvidbuf.pixels, gbvidbuf.pitch, filepath);
}

void GambatteSource::tryReset() {
	if(isResetting_)
		return;
	tryReset_ = true;
}

void GambatteSource::setResetting(bool state) {
	isResetting_ = state;
	emit resetting(state);
}

void GambatteSource::resetStepPre(std::size_t &samples) {
	if (resetStage_ == RESET_NOT) {
		if (tryReset_) {
			tryReset_ = false;
			setResetting(true);
			resetStage_ = RESET_FADE;
			resetCounter_ = resetFade_ + extraSamples();
		}
	} else {
		samples = std::min(samples, (std::size_t)resetCounter_);
	}
}

void GambatteSource::resetStepPost(
		PixelBuffer const &pb, qint16 *const soundBuf, std::size_t &samples) {
	if (resetStage_ == RESET_NOT)
		return;

	resetCounter_ -= samples;

	if (resetCounter_ <= 0) {
		if (resetStage_ == RESET_FADE) {
			reset(resetStall_);
			resetStage_ = RESET_STALL;
			resetCounter_ = resetStall_;
		} else if (resetStage_ == RESET_STALL) {
			setResetting(false);
			resetStage_ = RESET_NOT;
			resetCounter_ = 0;
		}
	}

	if (!gb_.isSgb())
		applyFade(pb, soundBuf, samples);
}

void GambatteSource::applyFade(
		PixelBuffer const &pb, qint16 *const soundBuf, std::size_t &samples) {
	if (void *const pbdata = getpbdata(pb, vsrci_)) {
		float alpha = 0.0f;

		if (resetStage_ == RESET_FADE) {
			float part = resetFade_ / 9.0f;
			alpha = (resetCounter_ - part) / (resetFade_ - 2 * part);
			alpha = std::min(std::max(alpha, 0.0f), 1.0f);
		}

		void          *dstbuf   = cconvert_ ? cconvert_->inBuf()   : pbdata;
		std::ptrdiff_t dstpitch = cconvert_ ? cconvert_->inPitch() : pb.pitch;

		uint_least32_t *pixelData = static_cast<uint_least32_t *>(dstbuf);

		for (unsigned y = 0; y < pb.height; ++y) {
			for (unsigned x = 0; x < pb.width; ++x) {
				unsigned r = (pixelData[x] >> 16 & 0xFF) * alpha;
				unsigned g = (pixelData[x] >>  8 & 0xFF) * alpha;
				unsigned b = (pixelData[x]       & 0xFF) * alpha;

				pixelData[x] = r << 16 | g << 8 | b;
			}

			pixelData += dstpitch;
		}

		if (alpha < 1.0f) {
			quint32 *sampleData = ptr_cast<quint32>(soundBuf);

			for (unsigned i = 0; i < samples; ++i)
				sampleData[i] = 0;
		}
	}
}

void GambatteSource::setResetParams(unsigned fade, unsigned stall) {
	resetFade_ = fade;
	resetStall_ = stall;
}
