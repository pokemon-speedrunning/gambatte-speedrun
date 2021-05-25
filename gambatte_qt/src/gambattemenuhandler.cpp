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

#include "gambattemenuhandler.h"
#include "blitterconf.h"
#include "cheatdialog.h"
#include "gambattesource.h"
#include "mainwindow.h"
#include "miscdialog.h"
#include "palettedialog.h"
#include "sounddialog.h"
#include "videodialog.h"
#include <QActionGroup>
#include <QFileInfo>
#include <QSettings>
#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
#include <fstream>
#include <iostream>

namespace {

static QString const strippedName(QString const &fullFileName) {
	return QFileInfo(fullFileName).fileName();
}

struct TmpPauser : private Uncopyable {
	MainWindow &mw;
	int const inc;

	explicit TmpPauser(MainWindow &mw, int inc = 4)
	: mw(mw), inc(inc)
	{
		mw.incPause(inc);
	}

	~TmpPauser() {
		mw.decPause(inc);
	}
};

}

FrameRateAdjuster::FrameTime::FrameTime(unsigned baseNum, unsigned baseDenom)
: index_(num_steps)
{
	setBaseFrameTime(baseNum, baseDenom);
}

void FrameRateAdjuster::FrameTime::setBaseFrameTime(unsigned const baseNum, unsigned const baseDenom) {
	frameTimes_[num_steps] = Rational(baseNum, baseDenom);

	unsigned const bnum = baseNum * 0x10000ul / baseDenom;
	for (std::size_t i = num_steps, num = bnum; i < num_steps * 2; ++i)
		frameTimes_[i + 1] = Rational(num = num * 11 / 10, 0x10000);
	for (std::size_t i = num_steps, num = bnum; i; --i)
		frameTimes_[i - 1] = Rational(num = num * 10 / 11, 0x10000);
}

FrameRateAdjuster::FrameRateAdjuster(MiscDialog const &miscDialog, MainWindow &mw, QObject *parent)
: QObject(parent)
, frameTime_(miscDialog.baseFps().height(), miscDialog.baseFps().width())
, miscDialog_(miscDialog)
, mw_(mw)
, decFrameRateAction_(new QAction(tr("&Decrease Frame Rate"), &mw))
, incFrameRateAction_(new QAction(tr("&Increase Frame Rate"), &mw))
, resetFrameRateAction_(new QAction(tr("&Reset Frame Rate"), &mw))
, enabled_(true)
{
#ifdef ENABLE_FRAMERATE_BUTTONS
	decFrameRateAction_->setShortcut(QString("Ctrl+D"));
	incFrameRateAction_->setShortcut(QString("Ctrl+I"));
	resetFrameRateAction_->setShortcut(QString("Ctrl+U"));

	connect(decFrameRateAction_,   SIGNAL(triggered()), this, SLOT(decFrameRate()));
	connect(incFrameRateAction_,   SIGNAL(triggered()), this, SLOT(incFrameRate()));
	connect(resetFrameRateAction_, SIGNAL(triggered()), this, SLOT(resetFrameRate()));
#endif
	connect(&miscDialog, SIGNAL(accepted()), this, SLOT(miscDialogChange()));
	changed();
}

QList<QAction *> const FrameRateAdjuster::actions() {
	QList<QAction *> l;
	l.append(decFrameRateAction_);
	l.append(incFrameRateAction_);
	l.append(resetFrameRateAction_);
	return l;
}

void FrameRateAdjuster::miscDialogChange() {
	QSize const &baseFps = miscDialog_.baseFps();
	frameTime_.setBaseFrameTime(baseFps.height(), baseFps.width());
	changed();
}

void FrameRateAdjuster::setDisabled(bool disabled) {
	enabled_ = !disabled;
	changed();
}

void FrameRateAdjuster::decFrameRate() {
	if (static_cast<GambatteMenuHandler *>(this->parent())->isResetting())
		return;
	if (enabled_) {
		frameTime_.inc();
		changed();
	}
}

void FrameRateAdjuster::incFrameRate() {
	if (static_cast<GambatteMenuHandler *>(this->parent())->isResetting())
		return;
	if (enabled_) {
		frameTime_.dec();
		changed();
	}
}

void FrameRateAdjuster::resetFrameRate() {
	if (static_cast<GambatteMenuHandler *>(this->parent())->isResetting())
		return;
	if (enabled_) {
		frameTime_.reset();
		changed();
	}
}

void FrameRateAdjuster::changed() {
	incFrameRateAction_->setEnabled(enabled_ && frameTime_.decPossible());
	decFrameRateAction_->setEnabled(enabled_ && frameTime_.incPossible());
	resetFrameRateAction_->setEnabled(enabled_ && frameTime_.resetPossible());

	FrameTime::Rational const &ft = enabled_ ? frameTime_.get() : frameTime_.base();
	mw_.setFrameTime(ft.num, ft.denom);
}

WindowSizeMenu::WindowSizeMenu(MainWindow &mw, VideoDialog const &vd)
: mw_(mw)
, menu_(new QMenu(tr("&Window Size"), &mw))
, group_(new QActionGroup(menu_))
, maxSize_(QApplication::desktop()->screen()->size())
{
	fillMenu(vd.sourceSize(), vd.scalingMethod());
	setCheckedSize(QSettings().value("video/windowSize", QSize()).toSize());
	connect(group_, SIGNAL(triggered(QAction *)), this, SLOT(triggered()));

	QSize const &size = checkedSize();
	mw_.setWindowSize(size);
	if (size.isEmpty())
		mw_.resize(QSettings().value("mainwindow/size", QSize(320, 288)).toSize());
}

WindowSizeMenu::~WindowSizeMenu() {
	QSettings settings;
	settings.setValue("video/windowSize", checkedSize());
}

void WindowSizeMenu::videoDialogChange(VideoDialog const &vd) {
	QSize const &oldSize = checkedSize();
	disconnect(group_, SIGNAL(triggered(QAction *)), this, SLOT(triggered()));
	menu_->clear();
	delete group_;
	group_ = new QActionGroup(menu_);

	fillMenu(vd.sourceSize(), vd.scalingMethod());
	setCheckedSize(oldSize);
	connect(group_, SIGNAL(triggered(QAction *)), this, SLOT(triggered()));

	QSize const &newSize = checkedSize();
	if (newSize != oldSize)
		mw_.setWindowSize(newSize);
}

void WindowSizeMenu::triggered() {
	mw_.setWindowSize(checkedSize());
}

void WindowSizeMenu::fillMenu(QSize const &sourceSize, ScalingMethod const scalingMethod) {
	QSize const aspectRatio(160, 144);
	QSize const basesz(scalingMethod == scaling_integer ? sourceSize : aspectRatio);
	QSize sz(basesz);
	while (sz.width() <= maxSize_.width() && sz.height() <= maxSize_.height()) {
		if (sz.width() >= sourceSize.width() && sz.height() >= sourceSize.height()) {
			QAction *a = menu_->addAction(
				'&' + QString::number(sz.width()) + 'x' + QString::number(sz.height()));
			a->setData(sz);
			a->setCheckable(true);
			group_->addAction(a);
		}

		sz += basesz;
	}

	QAction *a = menu_->addAction(tr("&Variable"));
	a->setData(QSize());
	a->setCheckable(true);
	group_->addAction(a);
}

void WindowSizeMenu::setCheckedSize(QSize const &size) {
	QList<QAction *> const &actions = group_->actions();
	foreach (QAction *a, actions) {
		if (a->data() == size) {
			a->setChecked(true);
			return;
		}
	}

	if (!group_->checkedAction())
		actions.front()->setChecked(true);
}

QSize const WindowSizeMenu::checkedSize() const {
	return group_->checkedAction() ? group_->checkedAction()->data().toSize() : QSize();
}

GambattePlatformMenu::GambattePlatformMenu(MainWindow &mw)
: mw_(mw)
, menu_(new QMenu(tr("&Platform"), &mw))
, group_(new QActionGroup(menu_))
{
	fillMenu();
	setCheckedPlatform(QSettings().value("platform", DEFAULT_GAMBATTE_PLATFORM).toInt());
	connect(group_, SIGNAL(triggered(QAction *)), this, SLOT(triggered()));

	QSettings settings;
	settings.setValue("platform", checkedPlatform());
}

GambattePlatformMenu::~GambattePlatformMenu() {
	QSettings settings;
	settings.setValue("platform", checkedPlatform());
}

void GambattePlatformMenu::triggered() {
	QSettings settings;
	settings.setValue("platform", checkedPlatform());
}

void GambattePlatformMenu::addPlatform(int platformId, QString const &platformName) {
	QAction *a = menu_->addAction(platformName);
	a->setData(platformId);
	a->setCheckable(true);
	group_->addAction(a);
}

void GambattePlatformMenu::fillMenu() {
#ifdef SHOW_PLATFORM_GB
	addPlatform(PLATFORM_GB, tr("&Game Boy"));
#endif
#ifdef SHOW_PLATFORM_GBC
	addPlatform(PLATFORM_GBC, tr("Game Boy &Color"));
#endif
#ifdef SHOW_PLATFORM_GBA
	addPlatform(PLATFORM_GBA, tr("Game Boy &Advance"));
#endif
	addPlatform(PLATFORM_GBP, tr("Game Boy &Player"));
#ifdef SHOW_PLATFORM_SGB
	addPlatform(PLATFORM_SGB, tr("&Super Game Boy 2"));
#endif
}

void GambattePlatformMenu::setCheckedPlatform(int platformId) {
	QList<QAction *> const &actions = group_->actions();
	foreach (QAction *a, actions) {
		if (a->data() == platformId) {
			a->setChecked(true);
			return;
		}
	}

	if (!group_->checkedAction())
		setCheckedPlatform(DEFAULT_GAMBATTE_PLATFORM);
}

int GambattePlatformMenu::checkedPlatform() const {
	return group_->checkedAction() ? group_->checkedAction()->data().toInt() : -1;
}

static QString const settingsPath() {
	QString path = QSettings(QSettings::IniFormat, QSettings::UserScope,
		QCoreApplication::organizationName(), QCoreApplication::applicationName()).fileName();
	path.truncate(path.lastIndexOf('/'));
	return path;
}

static QString const toCmdString(QAction const *a) {
	QString text = a->text().toLower();
	text.replace('&', QString());
	text.replace(' ', '-');
	return text;
}

static char toCmdChar(QAction const *a) {
	return a->shortcut().count() == 1
	     ? (a->shortcut()[0] - Qt::Key_A + 'a') & 0xff
	     : 0;
}

static QAction * findCmdStringAction(QList<QAction *> const &l, QString const &cmdstr) {
	foreach (QAction *a, l) {
		if (cmdstr == toCmdString(a))
			return a;
	}

	return 0;
}

static QAction * findCmdCharAction(QList<QAction *> const &l, char const c) {
	foreach (QAction *a, l) {
		if (c == toCmdChar(a))
			return a;
	}

	return 0;
}

static void printUsage(char const *const arg0, QList<QAction *> const &actions) {
	std::cout << "Usage: " << arg0 << " [OPTION]... [romfile]\n";

	foreach (QAction const *const a, actions) {
		if (a->isCheckable() && a->isEnabled()) {
			std::string const &text = toCmdString(a).toStdString();
			char const c = toCmdChar(a);
			std::cout << "  "
			          << (c ? '-' + std::string(1, c) + ", " : std::string("    "))
			          << "--" << text << "[=0]\n";
		}
	}
}

GambatteMenuHandler::GambatteMenuHandler(MainWindow &mw,
                                         GambatteSource &source,
                                         int const argc,
                                         char const *const argv[])
: mw_(mw)
, source_(source)
, soundDialog_(new SoundDialog(mw, &mw))
, videoDialog_(new VideoDialog(mw, source.generateVideoSourceInfos(),
                               QString("Video filter:"), &mw))
, miscDialog_(new MiscDialog(settingsPath() + "/saves", &mw))
, cheatDialog_(new CheatDialog(settingsPath() + "/cheats.ini", &mw))
, recentFileActs_()
, pauseAction_()
, syncFrameRateAction_()
, fsAct_()
, recentMenu_()
, globalPaletteDialog_()
, romPaletteDialog_()
, stateSlotGroup_(new QActionGroup(&mw))
, windowSizeMenu_(mw, *videoDialog_)
, gambattePlatformMenu_(mw)
, pauseInc_(4)
, isResetting_(false)
{
	setWindowPrefix("");
	source.inputDialog()->setParent(&mw, source.inputDialog()->windowFlags());

	{
		QString const &settingspath = settingsPath();
		QString const &palpath = settingspath + "/palettes";
		QDir::root().mkpath(settingspath + "/saves");
		QDir::root().mkpath(palpath);
		globalPaletteDialog_ = new PaletteDialog(palpath, 0, &mw);
		romPaletteDialog_ = new PaletteDialog(palpath, globalPaletteDialog_, &mw);
		connect(globalPaletteDialog_, SIGNAL(accepted()), this, SLOT(globalPaletteChange()));
		connect(romPaletteDialog_, SIGNAL(accepted()), this, SLOT(romPaletteChange()));
	}

	QActionGroup *const romLoadedActions = new QActionGroup(&mw);
	romLoadedActions->setExclusive(false);

	{
		for (int i = 0; i < max_recent_files; ++i) {
			recentFileActs_[i] = new QAction(&mw);
			recentFileActs_[i]->setVisible(false);
			connect(recentFileActs_[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
		}

		QMenu *fileMenu = mw.menuBar()->addMenu(tr("&File"));
		fileMenu->addAction(tr("&Open..."), this, SLOT(open()), tr("Ctrl+O"));

		recentMenu_ = fileMenu->addMenu(tr("O&pen Recent"));
		for (int i = 0; i < max_recent_files; ++i)
			recentMenu_->addAction(recentFileActs_[i]);

		romLoadedActions->addAction(fileMenu->addAction(tr("&Close ROM"), this, SLOT(close())));
		fileMenu->addSeparator();
		romLoadedActions->addAction(fileMenu->addAction(tr("&Reset"), this, SLOT(reset()), tr("Ctrl+R")));
		fileMenu->addSeparator();
		romLoadedActions->addAction(fileMenu->addAction(tr("Save State &As..."), this, SLOT(saveStateAs())));
		romLoadedActions->addAction(fileMenu->addAction(tr("Load State &From..."), this, SLOT(loadStateFrom())));
		fileMenu->addSeparator();
		romLoadedActions->addAction(fileMenu->addAction(tr("&Save State"),
		                            this, SLOT(saveState()), QString("Ctrl+S")));
		romLoadedActions->addAction(fileMenu->addAction(tr("&Load State"),
		                            this, SLOT(loadState()), QString("Ctrl+L")));

		{
			QMenu *const stateSlotMenu = fileMenu->addMenu(tr("S&elect State Slot"));
			stateSlotMenu->setEnabled(false);
			stateSlotMenu->addAction(tr("&Previous"), this, SLOT(prevStateSlot()), QString("Ctrl+Z"));
			stateSlotMenu->addAction(tr("&Next"), this, SLOT(nextStateSlot()), QString("Ctrl+X"));
			stateSlotMenu->addSeparator();

			for (int i = 0; i < 10; ++i) {
				int const no = i == 9 ? 0 : i + 1;
				QString const &strno = QString::number(no);
				QAction *action = stateSlotMenu->addAction(
					"Slot &" + strno, this, SLOT(selectStateSlot()), strno);
				action->setCheckable(true);
				action->setData(no);
				stateSlotGroup_->addAction(action);
			}

			connect(this, SIGNAL(romLoaded(bool)), stateSlotMenu, SLOT(setEnabled(bool)));
		}

		fileMenu->addAction(tr("Open Save Folder"), this, SLOT(openSaveFolder()));
		fileMenu->addSeparator();
		fileMenu->addAction(tr("&Quit"), qApp, SLOT(closeAllWindows()), tr("Ctrl+Q"));
		updateRecentFileActions();
	}

	FrameRateAdjuster *const frameRateAdjuster = new FrameRateAdjuster(*miscDialog_, mw, this);
	QList<QAction *> cmdactions;

	{
		QMenu *const playm = mw.menuBar()->addMenu(tr("&Play"));
		romLoadedActions->addAction(pauseAction_ = playm->addAction(
			tr("&Pause"), this, SLOT(pauseChange()), QString("Ctrl+P")));
		pauseAction_->setCheckable(true);
		romLoadedActions->addAction(playm->addAction(tr("Frame &Step"),
		                            this, SLOT(frameStep()), QString("Ctrl+.")));

	#ifdef ENABLE_FRAMERATE_BUTTONS
		playm->addSeparator();
		syncFrameRateAction_ = playm->addAction(tr("&Sync Frame Rate to Refresh Rate"));
		syncFrameRateAction_->setCheckable(true);
		connect(syncFrameRateAction_, SIGNAL(triggered(bool)),
		        frameRateAdjuster, SLOT(setDisabled(bool)));
		connect(syncFrameRateAction_, SIGNAL(triggered(bool)),
		        &mw, SLOT(setSyncToRefreshRate(bool)));

		foreach (QAction *action, frameRateAdjuster->actions())
			playm->addAction(romLoadedActions->addAction(action));
	#endif

	#ifdef ENABLE_INPUT_LOG
		playm->addSeparator();
		romLoadedActions->addAction(playm->addAction(
			tr("&Save Input Log As..."), this, SLOT(saveInputLogAs())));
	#endif

		cmdactions += playm->actions();
	}

	QMenu *const settingsm = mw.menuBar()->addMenu(tr("&Settings"));
	settingsm->addAction(tr("&Input..."), this, SLOT(execInputDialog()));
	settingsm->addAction(tr("&Miscellaneous..."), this, SLOT(execMiscDialog()));
	settingsm->addAction(tr("&Sound..."), this, SLOT(execSoundDialog()));
	settingsm->addAction(tr("&Video..."), this, SLOT(execVideoDialog()));
	settingsm->addSeparator();
	settingsm->addMenu(windowSizeMenu_.menu());
	settingsm->addSeparator();

	settingsm->addMenu(gambattePlatformMenu_.menu());

	{
		QMenu *const rtcModeMenu = settingsm->addMenu(tr("Real &Time Clock"));
		QActionGroup *const rtcModeActions = new QActionGroup(rtcModeMenu);
		cycleBasedAction_ = rtcModeMenu->addAction(tr("&Cycle-based"));
		cycleBasedAction_->setCheckable(true);
		cycleBasedAction_->setChecked(QSettings().value("rtc-mode", true).toBool());
		rtcModeActions->addAction(cycleBasedAction_);
#ifdef ENABLE_REAL_TIME_RTC
		realTimeAction_ = rtcModeMenu->addAction(tr("&Real-time"));
		realTimeAction_->setCheckable(true);
		realTimeAction_->setChecked(!cycleBasedAction_->isChecked());
		rtcModeActions->addAction(realTimeAction_);
#endif
		connect(rtcModeActions, SIGNAL(triggered(QAction *)), this, SLOT(setRtcMode()));
	}

	trueColorsAction_ = settingsm->addAction(tr("True &Colors"));
	trueColorsAction_->setCheckable(true);
	trueColorsAction_->setChecked(QSettings().value("true-colors", false).toBool());
	connect(trueColorsAction_, SIGNAL(toggled(bool)), &source, SLOT(setTrueColors(bool)));
	
	settingsm->addSeparator();
	attemptModeAction_ = settingsm->addAction(tr("Attempt &Mode"));
	attemptModeAction_->setCheckable(true);
	attemptModeAction_->setChecked(QSettings().value("attempt-mode", true).toBool());
	connect(attemptModeAction_, SIGNAL(toggled(bool)), &source, SLOT(setAttemptMode(bool)));

	settingsm->addSeparator();
	fsAct_ = settingsm->addAction(tr("&Full Screen"), this, SLOT(toggleFullScreen()), tr("Ctrl+F"));
	fsAct_->setCheckable(true);
	cmdactions += settingsm->actions();

	//romLoadedActions->addAction(mw.menuBar()->addMenu(tr("&Tools"))->addAction(tr("&Cheats..."),
	//                            cheatDialog_, SLOT(exec())));
	romLoadedActions->setEnabled(false);
	mw.menuBar()->addSeparator();

	QMenu *const helpMenu = mw.menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(tr("&About"), this, SLOT(about()));

	mw.addActions(mw.menuBar()->actions());

	{
		QAction *escAct = new QAction(&mw);
		escAct->setShortcut(tr("Esc"));
		connect(escAct, SIGNAL(triggered()), this, SLOT(escPressed()));
		mw.addAction(escAct);
	}

	mw.setSamplesPerFrame(35112);
	connect(&source, SIGNAL(setTurbo(bool)), &mw, SLOT(setFastForward(bool)));
	connect(&source, SIGNAL(togglePause()), pauseAction_, SLOT(trigger()));
	connect(&source, SIGNAL(frameStep()), this, SLOT(frameStep()));
	connect(&source, SIGNAL(decFrameRate()), frameRateAdjuster, SLOT(decFrameRate()));
	connect(&source, SIGNAL(incFrameRate()), frameRateAdjuster, SLOT(incFrameRate()));
	connect(&source, SIGNAL(resetFrameRate()), frameRateAdjuster, SLOT(resetFrameRate()));
	connect(&source, SIGNAL(prevStateSlot()), this, SLOT(prevStateSlot()));
	connect(&source, SIGNAL(nextStateSlot()), this, SLOT(nextStateSlot()));
	connect(&source, SIGNAL(saveStateSignal()), this, SLOT(saveState()));
	connect(&source, SIGNAL(loadStateSignal()), this, SLOT(loadState()));
	connect(&source, SIGNAL(resetSignal()), this, SLOT(reset()));
	connect(&source, SIGNAL(resetting(bool)), this, SLOT(setResetting(bool)));
	connect(&source, SIGNAL(quit()), qApp, SLOT(closeAllWindows()));
	connect(videoDialog_, SIGNAL(accepted()), this, SLOT(videoDialogChange()));
	connect(soundDialog_, SIGNAL(accepted()), this, SLOT(soundDialogChange()));
	connect(miscDialog_, SIGNAL(accepted()), this, SLOT(miscDialogChange()));
	connect(cheatDialog_, SIGNAL(accepted()), this, SLOT(cheatDialogChange()));
	connect(&mw, SIGNAL(videoBlitterFailure()), this, SLOT(videoBlitterFailure()));
	connect(&mw, SIGNAL(audioEngineFailure()), this, SLOT(audioEngineFailure()));
	connect(&mw, SIGNAL(closing()), this, SLOT(saveWindowSizeIfNotFullScreen()));
	connect(&mw, SIGNAL(dwmCompositionChange()), this, SLOT(reconsiderSyncFrameRateActionEnable()));
	connect(this, SIGNAL(romLoaded(bool)), romLoadedActions, SLOT(setEnabled(bool)));
	connect(this, SIGNAL(romLoaded(bool)), gambattePlatformMenu_.group(), SLOT(setDisabled(bool)));
	connect(this, SIGNAL(romLoaded(bool)), attemptModeAction_, SLOT(setDisabled(bool)));
	connect(this, SIGNAL(romLoaded(bool)), stateSlotGroup_->actions().at(0), SLOT(setChecked(bool)));

	mw.setAspectRatio(QSize(160, 144));
	videoDialogChange();
	soundDialogChange();
	miscDialogChange();

	bool unknownCmd = false;

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1]) {
			QString const argstr(argv[i] + 2);

			if (QAction *a = argv[i][1] == '-'
					? findCmdStringAction(cmdactions, argstr.left(argstr.lastIndexOf('=')))
					: findCmdCharAction(cmdactions, argv[i][1])) {
				if (argstr.endsWith("=0") == a->isChecked() && a->isEnabled())
					a->trigger();
			} else
				unknownCmd = true;
		}
	}

	if (unknownCmd)
		printUsage(argv[0], cmdactions);

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-') {
			if (fsAct_->isChecked())
				mw.menuBar()->hide();

			loadFile(QFileInfo(QString(argv[i])).absoluteFilePath());
			break;
		}
	}
}

GambatteMenuHandler::~GambatteMenuHandler() {
	QSettings settings;
	settings.setValue("rtc-mode", cycleBasedAction_->isChecked());
	settings.setValue("true-colors", trueColorsAction_->isChecked());
	settings.setValue("attempt-mode", attemptModeAction_->isChecked());
}

void GambatteMenuHandler::setWindowPrefix(QString const &windowPrefix) {
	QString separator(windowPrefix.isEmpty() ? "" : " - ");
	QString revision("(" GSR_VERSION_STR ")");
	mw_.setWindowTitle(windowPrefix + separator + "Gambatte-Speedrun " + revision);
}

void GambatteMenuHandler::updateRecentFileActions() {
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();

	int const numRecentFiles = qMin(files.size(), static_cast<int>(max_recent_files));

	for (int i = 0; i < numRecentFiles; ++i) {
		QString const &text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
		recentFileActs_[i]->setText(text);
		recentFileActs_[i]->setData(files[i]);
		recentFileActs_[i]->setVisible(true);
	}

	for (int j = numRecentFiles; j < max_recent_files; ++j)
		recentFileActs_[j]->setVisible(false);

	recentMenu_->setEnabled(numRecentFiles > 0);
}

void GambatteMenuHandler::setCurrentFile(QString const &fileName) {
	if (isResetting_)
		return;
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);
	while (files.size() > max_recent_files)
		files.removeLast();

	settings.setValue("recentFileList", files);
	updateRecentFileActions();
}

void GambatteMenuHandler::loadFile(QString const &fileName) {
	if (isResetting_)
		return;
	TmpPauser tmpPauser(mw_, 4);
	pauseAction_->setChecked(false);
	pauseChange();
	mw_.waitUntilPaused();

	QSettings settings;

	int platformId = settings.value("platform", DEFAULT_GAMBATTE_PLATFORM).toInt();
	GambattePlatformInfo platform = gambatte_platform_info[platformId];
	unsigned flags = platform.loadFlags;
	GambatteBiosInfo info = platform.biosInfo;

	source_.setResetParams(platform.resetFade, platform.resetStall);

	if (miscDialog_->multicartCompat())
		flags |= gambatte::GB::MULTICART_COMPAT;

	QString biosFilename = settings.value(info.key, "").toString();
	if(biosFilename.isEmpty() ||
			source_.loadBios(biosFilename.toLocal8Bit().constData(), info.size, info.crc) != 0) {
		mw_.stop();
		emit dmgRomLoaded(false);
		emit romLoaded(false);
		QMessageBox::StandardButton button = QMessageBox::warning(
			&mw_,
			tr("Bios Load Error"),
			("Gambatte-Speedrun requires a " + info.name + " bios for the selected platform.\n\n" +
			"The bios (or boot ROM) is a .bin file you'll need to acquire before continuing.\n\n" +
			"Gambatte-Speedrun does not distribute this file, but you may be able to acquire it " +
			"through similar methods used for other ROMs.\n\n" +
			"Please press Open to specify the location of this file."),
			QMessageBox::Open | QMessageBox::Cancel);
		if (button == QMessageBox::Open)
			openBios(info);
		return;
	}
	
	std::cout << "Loading rom..." << std::endl;

	if (gambatte::LoadRes const error = source_.load(fileName.toLocal8Bit().constData(), flags)) {
		mw_.stop();
		emit dmgRomLoaded(false);
		emit romLoaded(false);
		QMessageBox::critical(
			&mw_,
			tr("File Load Error"),
			(tr("Failed to load file\n")
			 + fileName + ".\n\n"
			 + gambatte::to_string(error).c_str() + '.'));
		return;
	}

	QString const &romTitle = QString::fromStdString(source_.romTitle()).trimmed();
	cheatDialog_->setGameName(romTitle.isEmpty() ? QFileInfo(fileName).completeBaseName() : romTitle);
	cheatDialogChange();

	if (!source_.isCgb()) {
		romPaletteDialog_->setSettingsFile(
				QFileInfo(fileName).completeBaseName() + ".pal",
				romTitle);
		//setDmgPaletteColors();
	}

	source_.setTimeMode(cycleBasedAction_->isChecked());
	source_.setTrueColors(trueColorsAction_->isChecked());
	source_.setAttemptMode(attemptModeAction_->isChecked());

	gambatte::PakInfo const &pak = source_.pakInfo();
	std::cout << romTitle.toStdString() << '\n'
	          << "GamePak type: " << pak.mbc()
	          << " rambanks: " << pak.rambanks()
	          << " rombanks: " << pak.rombanks() << '\n'
	          << "header checksum: " << (pak.headerChecksumOk() ? "ok" : "bad") << '\n'
	          << "cgb: " << source_.isCgb() << std::endl;

	// Basic good rom testing for PSR only. Fail doesn't mean it's a bad ROM for anything except English Gen1-2 games!!!
	QString label;
	for (GambatteGoodromInfo good : gambatte_goodroms) {
		if (romTitle.toStdString() == good.title && pak.crc() == good.crc && platformId == DEFAULT_GAMBATTE_PLATFORM) {
			if (!good.label.empty())
				label = " " + QString::fromStdString(good.label);
			source_.setBreakpoint(good.savBreakpoint);
			break;
		}
	}

	setWindowPrefix(strippedName(fileName) + label + (attemptModeAction_->isChecked() ? " AM" : ""));
	setCurrentFile(fileName);

	emit romLoaded(true);
	emit dmgRomLoaded(!source_.isCgb());

	mw_.resetAudio();
	mw_.run();
	
	if (attemptModeAction_->isChecked())
		reset(); // Force reset fadeout for loading ROMs; prevents avoiding the fadeout from closing and re-opening the ROM
}

void GambatteMenuHandler::open() {
	if (isResetting_)
		return;
	TmpPauser tmpPauser(mw_, 4);
	mw_.waitUntilPaused();

	QString const &fileName = QFileDialog::getOpenFileName(
		&mw_, tr("Open"), recentFileActs_[0]->data().toString(),
		tr("Game Boy ROM Images (*.dmg *.gb *.gbc *.sgb *.zip *.gz);;All Files (*)"));
	if (!fileName.isEmpty())
		loadFile(fileName);

	// giving back focus after getOpenFileName seems to fail at times, which
	// can be problematic with current exclusive mode handling.
	mw_.setFocus();
}

void GambatteMenuHandler::close() {
	if (isResetting_)
		return;
	TmpPauser tmpPauser(mw_, 4);
	mw_.waitUntilPaused();

	source_.load("", 0);
	mw_.stop();

	setWindowPrefix("");

	emit dmgRomLoaded(false);
	emit romLoaded(false);
}

void GambatteMenuHandler::openBios(GambatteBiosInfo const &info) {
	if (isResetting_)
		return;
	TmpPauser tmpPauser(mw_, 4);
	mw_.waitUntilPaused();

	QString const &fileName = QFileDialog::getOpenFileName(
		&mw_, tr("Open"), "",
		(info.name + tr(" Bios Images (*.bin ") + info.filter + tr(");;All Files (*)")));
	if (!fileName.isEmpty()) {
		mw_.stop();
		emit dmgRomLoaded(false);
		emit romLoaded(false);
		int result = source_.loadBios(fileName.toLocal8Bit().constData(), info.size, info.crc);
		if(result != 0) {
			QMessageBox::critical(
				&mw_,
				tr("Bios Load Error"),
				(tr("Could not load ") + info.name + tr(" bios.\n") +
				"Please check that the file is a valid " + info.name + " bios file."));
			return;
		}
		else {
			// Store bios path
			QSettings settings;
			settings.setValue(info.key, fileName);
			QMessageBox::information(
				&mw_,
				tr("Loaded Bios Successfully"),
				(tr("Loaded the ") + info.name + tr(" bios file successfully.\n") +
				"Its path has been saved for future use."));
			return;
		}
	}

	// giving back focus after getOpenFileName seems to fail at times, which
	// can be problematic with current exclusive mode handling.
	mw_.setFocus();
}

void GambatteMenuHandler::openRecentFile() {
	if (isResetting_)
		return;
	if (QAction const *action = qobject_cast<QAction *>(sender()))
		loadFile(action->data().toString());
}

void GambatteMenuHandler::about() {
	TmpPauser tmpPauser(mw_, pauseInc_);
	QMessageBox::about(
		&mw_,
		"About Gambatte-Speedrun",
		"<h3>Gambatte-Speedrun"
		" (" GSR_VERSION_STR ")"
		"</h3>"
		"<p>"
			"<b>Homepage:</b> "
			"<a href=\"https://github.com/pokemon-speedrunning/gambatte-speedrun\">"
				"https://github.com/pokemon-speedrunning/gambatte-speedrun"
			"</a>"
		"</p>"
		"<p>"
			"<b>Forked from:</b> "
			"<a href=\"https://github.com/sinamas/gambatte\">"
				"https://github.com/sinamas/gambatte"
			"</a>"
		"</p>"
		"<p>"
			"A portable, open-source Game Boy Color emulator with speedrunning-related fixes/optimizations."
		"</p>"
	);
}

void GambatteMenuHandler::globalPaletteChange() {
	romPaletteDialog_->externalChange();
	setDmgPaletteColors();
}

void GambatteMenuHandler::romPaletteChange() {
	globalPaletteDialog_->externalChange();
	setDmgPaletteColors();
}

namespace {

struct SetDmgPaletteColorFun {
	GambatteSource &source; int palnum; int colornum; unsigned long rgb32;
	void operator()() const { source.setDmgPaletteColor(palnum, colornum, rgb32); }
};

struct SetVideoSourceFun {
	GambatteSource &source; std::size_t sourceIndex;
	void operator()() const { source.setVideoSource(sourceIndex); }
};

struct SetSaveDirFun {
	GambatteSource &source; QString path;
	void operator()() const { source.setSavedir(path.toLocal8Bit().constData()); }
};

} // anon ns

void GambatteMenuHandler::setDmgPaletteColors() {
	for (int palnum = 0; palnum < 3; ++palnum)
	for (int colornum = 0; colornum < 4; ++colornum) {
		SetDmgPaletteColorFun fun = { source_, palnum, colornum,
		                              romPaletteDialog_->color(palnum, colornum) };
		mw_.callInWorkerThread(fun);
	}
}

void GambatteMenuHandler::videoDialogChange() {
	{
		SetVideoSourceFun fun = { source_, videoDialog_->sourceIndex() };
		mw_.callInWorkerThread(fun);
	}

	applySettings(mw_, *videoDialog_);
	windowSizeMenu_.videoDialogChange(*videoDialog_);
	reconsiderSyncFrameRateActionEnable();
}

void GambatteMenuHandler::soundDialogChange() {
	applySettings(mw_, *soundDialog_);
}

void GambatteMenuHandler::miscDialogChange() {
	SetSaveDirFun const setSaveDirFun = { source_, miscDialog_->savePath() };
	mw_.callInWorkerThread(setSaveDirFun);
	mw_.setDwmTripleBuffer(miscDialog_->dwmTripleBuf());
	mw_.setFastForwardSpeed(miscDialog_->turboSpeed());
	mw_.setJoystickThreshold(miscDialog_->threshold());
	source_.inputDialog()->setJoystickThreshold(miscDialog_->threshold());
	mw_.setPauseOnFocusOut(miscDialog_->pauseOnFocusOut() ? 2 : 0);
	pauseInc_ = miscDialog_->pauseOnDialogs() ? 4 : 0;
}

void GambatteMenuHandler::cheatDialogChange() {
	std::string gameGenieCodes;
	std::string gameSharkCodes;

	foreach (QString const &s, cheatDialog_->cheats().split(';', QString::SkipEmptyParts)) {
		if (s.contains('-')) {
			gameGenieCodes += s.toStdString() + ';';
		} else
			gameSharkCodes += s.toStdString() + ';';
	}

	//source_.setGameGenie(gameGenieCodes);
	//source_.setGameShark(gameSharkCodes);
}

void GambatteMenuHandler::reconsiderSyncFrameRateActionEnable() {
#ifdef ENABLE_FRAMERATE_BUTTONS
	if (mw_.blitterConf(videoDialog_->blitterNo()).maxSwapInterval()
			&& !MainWindow::isDwmCompositionEnabled()) {
		syncFrameRateAction_->setEnabled(true);
	} else {
		if (syncFrameRateAction_->isChecked())
			syncFrameRateAction_->trigger();

		syncFrameRateAction_->setEnabled(false);
	}
#endif
}

void GambatteMenuHandler::execGlobalPaletteDialog() {
	TmpPauser tmpPauser(mw_, pauseInc_);
	globalPaletteDialog_->exec();
}

void GambatteMenuHandler::execRomPaletteDialog() {
	TmpPauser tmpPauser(mw_, pauseInc_);
	romPaletteDialog_->exec();
}

void GambatteMenuHandler::execInputDialog() {
	TmpPauser tmpPauser(mw_, pauseInc_);
	source_.inputDialog()->exec();
}

void GambatteMenuHandler::execSoundDialog() {
	TmpPauser tmpPauser(mw_, pauseInc_);
	soundDialog_->exec();
}

void GambatteMenuHandler::execVideoDialog() {
	TmpPauser tmpPauser(mw_, pauseInc_);
	videoDialog_->exec();
}

void GambatteMenuHandler::execMiscDialog() {
	TmpPauser tmpPauser(mw_, pauseInc_);
	miscDialog_->exec();
}

void GambatteMenuHandler::prevStateSlot() {
	if (isResetting_)
		return;
	stateSlotGroup_->actions().at(source_.currentState() < 2
	                            ? source_.currentState() + 8
	                            : source_.currentState() - 2)->trigger();
}

void GambatteMenuHandler::nextStateSlot() {
	if (isResetting_)
		return;
	stateSlotGroup_->actions().at(source_.currentState())->trigger();
}

namespace {

struct SelectStateFun {
	GambatteSource &source; int i;
	void operator()() const { source.selectState(i); }
};

struct SaveStateFun {
	GambatteSource &source;
	MainWindow::FrameBuffer fb;
	void operator()() const {
		source.saveState(MainWindow::FrameBuffer::Locked(fb).get());
	}
};

struct LoadStateFun {
	GambatteSource &source;
	void operator()() const { source.loadState(); }
};

struct SaveStateAsFun {
	GambatteSource &source;
	MainWindow::FrameBuffer fb;
	QString fileName;
	void operator()() const {
		source.saveState(MainWindow::FrameBuffer::Locked(fb).get(),
		                 fileName.toLocal8Bit().constData());
	}
};

struct LoadStateFromFun {
	GambatteSource &source;
	QString fileName;
	void operator()() const {
		source.loadState(fileName.toLocal8Bit().constData());
	}
};

struct SaveInputLogAsFun {
	GambatteSource &source;
	QString fileName;
	void operator()() const {
		std::ofstream file(fileName.toLocal8Bit().constData(),
			std::ios::out | std::ios::binary);

		file.put(0xFE);
		file.put(0x01); // MOVIE_VERSION

		std::vector<char> state = source.inputLogState();
		file.put(state.size() >> 16);
		file.put(state.size() >> 8);
		file.put(state.size());
		file.write(state.data(), state.size());

		for (std::pair<std::uint32_t, std::uint8_t> i : source.inputLog()) {
			file.put(i.first >> 24);
			file.put(i.first >> 16);
			file.put(i.first >> 8);
			file.put(i.first);
			file.put(i.second);
		}

		file.close();
	}
};

struct ResetFun {
	GambatteSource &source;
	void operator()() const { source.tryReset(); }
};

} // anon ns

void GambatteMenuHandler::selectStateSlot() {
	if (isResetting_) {
		stateSlotGroup_->actions().at(source_.currentState() - 1)->setChecked(true);
		return;
	}
	if (QAction *action = stateSlotGroup_->checkedAction()) {
		SelectStateFun fun = { source_, action->data().toInt() };
		mw_.callInWorkerThread(fun);
	}
}

void GambatteMenuHandler::saveState() {
	if (isResetting_)
		return;
	SaveStateFun fun = { source_, MainWindow::FrameBuffer(mw_) };
	mw_.callInWorkerThread(fun);
}

void GambatteMenuHandler::loadState() {
	if (attemptModeAction_->isChecked() || isResetting_)
		return;
	LoadStateFun fun = { source_ };
	mw_.callInWorkerThread(fun);
}

void GambatteMenuHandler::saveStateAs() {
	if (isResetting_)
		return;
	TmpPauser tmpPauser(mw_, 4);
	mw_.waitUntilPaused();

	QString const &fileName = QFileDialog::getSaveFileName(
		&mw_, tr("Save State"), QString(),
		tr("Gambatte Quick Save Files (*.gqs);;All Files (*)"));
	if (!fileName.isEmpty()) {
		SaveStateAsFun fun = { source_, MainWindow::FrameBuffer(mw_), fileName };
		mw_.callInWorkerThread(fun);
	}
}

void GambatteMenuHandler::loadStateFrom() {
	if (attemptModeAction_->isChecked() || isResetting_)
		return;
	TmpPauser tmpPauser(mw_, 4);
	mw_.waitUntilPaused();

	QString const &fileName = QFileDialog::getOpenFileName(
		&mw_, tr("Load State"), QString(),
		tr("Gambatte Quick Save Files (*.gqs);;All Files (*)"));
	if (!fileName.isEmpty()) {
		LoadStateFromFun fun = { source_, fileName };
		mw_.callInWorkerThread(fun);
	}
}

void GambatteMenuHandler::saveInputLogAs() {
	if (isResetting_)
		return;
	TmpPauser tmpPauser(mw_, 4);
	mw_.waitUntilPaused();

	QString const &fileName = QFileDialog::getSaveFileName(
		&mw_, tr("Save Input Log"), QString(),
		tr("Gambatte Movie Files (*.gm);;All Files(*)"));
	if (!fileName.isEmpty()) {
		SaveInputLogAsFun fun = { source_, fileName };
		mw_.callInWorkerThread(fun);
	}
}

void GambatteMenuHandler::openSaveFolder() {
	// ref https://stackoverflow.com/questions/3569749/qt-open-default-file-explorer-on-nix
	QString path = QDir::toNativeSeparators(miscDialog_->savePath());
	QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void GambatteMenuHandler::reset() {
	ResetFun fun = { source_ };
	mw_.callInWorkerThread(fun);
}

void GambatteMenuHandler::setResetting(bool state) {
	isResetting_ = state;
}

void GambatteMenuHandler::pauseChange() {
	if (attemptModeAction_->isChecked() || isResetting_) {
		pauseAction_->setChecked(false);
		return;
	}
	if (pauseAction_->isChecked())
		mw_.pause();
	else
		mw_.unpause();
}

void GambatteMenuHandler::frameStep() {
	if (attemptModeAction_->isChecked() || isResetting_)
		return;
	if (pauseAction_->isChecked()) {
		mw_.frameStep();

		if (attemptModeAction_->isChecked() || isResetting_) {
			pauseAction_->setChecked(false);
			mw_.unpause();
		}
	}
	else
		pauseAction_->trigger();
}

void GambatteMenuHandler::escPressed() {
#ifdef Q_OS_MAC
	if (fsAct_->isChecked())
		fsAct_->trigger();
#else
	mw_.menuBar()->setVisible(!mw_.menuBar()->isVisible());
	if (!mw_.menuBar()->isVisible())
		mw_.hideCursor();
#endif
}

void GambatteMenuHandler::videoBlitterFailure() {
	TmpPauser tmpPauser(mw_, pauseInc_);
	QMessageBox::critical(&mw_, tr("Video engine failure"),
			tr("Failed to update video output. This may be fixed by changing the video engine settings."));
	videoDialog_->exec();
}

void GambatteMenuHandler::audioEngineFailure() {
	TmpPauser tmpPauser(mw_, pauseInc_);
	QMessageBox::critical(&mw_, tr("Sound engine failure"),
			tr("Failed to output audio. This may be fixed by changing the sound settings."));
	soundDialog_->exec();
}

void GambatteMenuHandler::setRtcMode() {
	source_.setTimeMode(cycleBasedAction_->isChecked());
}

void GambatteMenuHandler::toggleFullScreen() {
	saveWindowSizeIfNotFullScreen();
	mw_.toggleFullScreen();
}

void GambatteMenuHandler::saveWindowSizeIfNotFullScreen() {
	if (!mw_.isFullScreen()) {
		QSettings settings;
		settings.setValue("mainwindow/size", mw_.size());
	}
}
