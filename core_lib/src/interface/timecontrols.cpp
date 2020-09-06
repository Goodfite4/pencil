/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "timecontrols.h"

#include <QLabel>
#include <QSettings>
#include <QMenu>

#include "editor.h"
#include "playbackmanager.h"
#include "layermanager.h"
#include "pencildef.h"
#include "util.h"
#include "preferencemanager.h"
#include "timeline.h"
#include "pencildef.h"
#include <QDebug>

TimeControls::TimeControls(TimeLine* parent) : QToolBar(parent)
{
    mTimeline = parent;
}

void TimeControls::initUI()
{
    QSettings settings(PENCIL2D, PENCIL2D);

    mFpsBox = new QSpinBox(this);
    mFpsBox->setFixedHeight(24);
    mFpsBox->setValue(settings.value("Fps").toInt());
    mFpsBox->setMinimum(1);
    mFpsBox->setMaximum(90);
    mFpsBox->setSuffix(tr(" fps"));
    mFpsBox->setToolTip(tr("Frames per second"));
    mFpsBox->setFocusPolicy(Qt::WheelFocus);

    mFps = mFpsBox->value();
    mTimecodeSelect = new QToolButton(this);
    mTimecodeSelect->setIcon(QIcon(":app/icons/new/svg/more_options.svg"));
    mTimecodeSelect->setPopupMode(QToolButton::InstantPopup);
    mTimecodeLabelEnum = mEditor->preference()->getInt(SETTING::TIMECODE_TEXT);
    mTimecodeLabel = new QLabel(this);
    mTimecodeLabel->setContentsMargins(5, 0, 5, 0);
    mTimecodeLabel->setText("");

    switch (mTimecodeLabelEnum)
    {
    case NOTEXT:
        mTimecodeLabel->setToolTip("");
        break;
    case FRAMES:
        mTimecodeLabel->setToolTip(tr("Actual frame number"));
        break;
    case SMPTE:
        mTimecodeLabel->setToolTip(tr("Timecode format MM:SS:FF"));
        break;
    case SFF:
        mTimecodeLabel->setToolTip(tr("Timecode format S:FF"));
        break;
    default:
        mTimecodeLabel->setToolTip("");
    }

    mLoopStartSpinBox = new QSpinBox(this);
    mLoopStartSpinBox->setFixedHeight(24);
    mLoopStartSpinBox->setValue(settings.value("loopStart").toInt());
    mLoopStartSpinBox->setMinimum(1);
    mLoopStartSpinBox->setEnabled(false);
    mLoopStartSpinBox->setToolTip(tr("Start of playback loop"));
    mLoopStartSpinBox->setFocusPolicy(Qt::WheelFocus);

    mLoopEndSpinBox = new QSpinBox(this);
    mLoopEndSpinBox->setFixedHeight(24);
    mLoopEndSpinBox->setValue(settings.value("loopEnd").toInt());
    mLoopEndSpinBox->setMinimum(2);
    mLoopEndSpinBox->setEnabled(false);
    mLoopEndSpinBox->setToolTip(tr("End of playback loop"));
    mLoopEndSpinBox->setFocusPolicy(Qt::WheelFocus);

    mPlaybackRangeCheckBox = new QCheckBox(tr("Range"));
    mPlaybackRangeCheckBox->setFixedHeight(24);
    mPlaybackRangeCheckBox->setToolTip(tr("Playback range"));

    mPlayButton = new QPushButton(this);
    mLoopButton = new QPushButton(this);
    mSoundButton = new QPushButton(this);
    mSoundScrubButton = new QPushButton(this);
    mJumpToEndButton = new QPushButton(this);
    mJumpToStartButton = new QPushButton(this);

    mLoopIcon = QIcon(":icons/controls/loop.png");
    mSoundIcon = QIcon(":icons/controls/sound.png");
    if (mEditor->preference()->isOn(SETTING::SOUND_SCRUB_ACTIVE))
        mSoundScrubIcon = QIcon(":icons/controls/soundscrub.png");
    else
        mSoundScrubIcon = QIcon(":icons/controls/soundscrub-disabled.png");
    mJumpToEndIcon = QIcon(":icons/controls/endplay.png");
    mJumpToStartIcon = QIcon(":icons/controls/startplay.png");
    mStartIcon = QIcon(":icons/controls/play.png");
    mStopIcon = QIcon(":icons/controls/stop.png");
    mPlayButton->setIcon(mStartIcon);
    mLoopButton->setIcon(mLoopIcon);
    mSoundButton->setIcon(mSoundIcon);
    mSoundScrubButton->setIcon(mSoundScrubIcon);
    mJumpToEndButton->setIcon(mJumpToEndIcon);
    mJumpToStartButton->setIcon(mJumpToStartIcon);

    mPlayButton->setToolTip(tr("Play"));
    mLoopButton->setToolTip(tr("Loop"));
    mSoundButton->setToolTip(tr("Sound on/off"));
    mSoundScrubButton->setToolTip(tr("Sound scrub on/off"));
    mJumpToEndButton->setToolTip(tr("Jump to the End", "Tooltip of the jump to end button"));
    mJumpToStartButton->setToolTip(tr("Jump to the Start", "Tooltip of the jump to start button"));

    mLoopButton->setCheckable(true);
    mSoundButton->setCheckable(true);
    mSoundButton->setChecked(true);
    mSoundScrubButton->setCheckable(true);
    mSoundScrubButton->setChecked(mEditor->preference()->isOn(SETTING::SOUND_SCRUB_ACTIVE));


    addWidget(mJumpToStartButton);
    addWidget(mPlayButton);
    addWidget(mJumpToEndButton);
    addWidget(mLoopButton);
    addWidget(mPlaybackRangeCheckBox);
    addWidget(mLoopStartSpinBox);
    addWidget(mLoopEndSpinBox);
    addWidget(mSoundButton);
    addWidget(mSoundScrubButton);
    addWidget(mFpsBox);
    addWidget(mTimecodeSelect);
    addWidget(mTimecodeLabel);

    makeConnections();

    updateUI();
}

void TimeControls::updateUI()
{
    PlaybackManager* playback = mEditor->playback();

    mPlaybackRangeCheckBox->setChecked(playback->isRangedPlaybackOn()); // don't block this signal since it enables start/end range spinboxes.

    QSignalBlocker b1(mLoopStartSpinBox);
    mLoopStartSpinBox->setValue(playback->markInFrame());

    QSignalBlocker b2(mLoopEndSpinBox);
    mLoopEndSpinBox->setValue(playback->markOutFrame());

    QSignalBlocker b3(mFpsBox);
    mFpsBox->setValue(playback->fps());

    QSignalBlocker b4(mLoopButton);
    mLoopButton->setChecked(playback->isLooping());
}

void TimeControls::setEditor(Editor* editor)
{
    Q_ASSERT(editor != nullptr);
    mEditor = editor;
}

void TimeControls::setFps(int value)
{
    QSignalBlocker blocker(mFpsBox);
    mFpsBox->setValue(value);
    mFps = value;
    updateTimecodeLabel(mEditor->currentFrame());
}

void TimeControls::setLoop(bool checked)
{
    mLoopButton->setChecked(checked);
}

void TimeControls::setRangeState(bool checked)
{
    mPlaybackRangeCheckBox->setChecked(checked);
    mTimeline->updateLength();
}

void TimeControls::makeConnections()
{
    connect(mPlayButton, &QPushButton::clicked, this, &TimeControls::playButtonClicked);
    connect(mJumpToEndButton, &QPushButton::clicked, this, &TimeControls::jumpToEndButtonClicked);
    connect(mJumpToStartButton, &QPushButton::clicked, this, &TimeControls::jumpToStartButtonClicked);
    connect(mLoopButton, &QPushButton::clicked, this, &TimeControls::loopButtonClicked);
    connect(mPlaybackRangeCheckBox, &QCheckBox::clicked, this, &TimeControls::playbackRangeClicked);

    auto spinBoxValueChanged = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);
    connect(mLoopStartSpinBox, spinBoxValueChanged, this, &TimeControls::loopStartValueChanged);
    clearFocusOnFinished(mLoopStartSpinBox);
    connect(mLoopEndSpinBox, spinBoxValueChanged, this, &TimeControls::loopEndValueChanged);
    clearFocusOnFinished(mLoopEndSpinBox);

    connect(mPlaybackRangeCheckBox, &QCheckBox::toggled, mLoopStartSpinBox, &QSpinBox::setEnabled);
    connect(mPlaybackRangeCheckBox, &QCheckBox::toggled, mLoopEndSpinBox, &QSpinBox::setEnabled);

    connect(mSoundButton, &QPushButton::clicked, this, &TimeControls::soundToggled);
    connect(mSoundButton, &QPushButton::clicked, this, &TimeControls::updateSoundIcon);

    connect(mSoundScrubButton, &QPushButton::clicked, this, &TimeControls::soundScrubToggled);
    connect(mSoundScrubButton, &QPushButton::clicked, this, &TimeControls::updateSoundScrubIcon);

    connect(mFpsBox, spinBoxValueChanged, this, &TimeControls::fpsChanged);
    connect(mFpsBox, &QSpinBox::editingFinished, this, &TimeControls::onFpsEditingFinished);

    connect(mFpsBox, spinBoxValueChanged, this, &TimeControls::setFps);
    connect(mEditor, &Editor::fpsChanged, this, &TimeControls::setFps);
    connect(mTimecodeSelect, &QToolButton::clicked, this, &TimeControls::showTimecodeLabelMenu);
}

void TimeControls::playButtonClicked()
{
    emit playButtonTriggered();
}

void TimeControls::updatePlayState()
{
    if (mEditor->playback()->isPlaying())
    {
        mPlayButton->setIcon(mStopIcon);
        mPlayButton->setToolTip(tr("Stop"));
    }
    else
    {
        mPlayButton->setIcon(mStartIcon);
        mPlayButton->setToolTip(tr("Play"));
    }
}

void TimeControls::showTimecodeLabelMenu()
{
    QPoint globalPos = mTimecodeSelect->cursor().pos();

    QMenu* menu = new QMenu();
    menu->addAction(tr("No text"), this, &TimeControls::noTimecodeText, 0);
    menu->addAction(tr("Frames"),  this, &TimeControls::onlyFramesText, 0);
    menu->addAction(tr("SMPTE Timecode"), this, &TimeControls::SMPTE_text, 0);
    menu->addAction(tr("SFF Timecode"), this, &TimeControls::SFF_text, 0);

    menu->exec(globalPos);}

void TimeControls::jumpToStartButtonClicked()
{
    if (mPlaybackRangeCheckBox->isChecked())
    {
        mEditor->scrubTo(mLoopStartSpinBox->value());
        mEditor->playback()->setCheckForSoundsHalfway(true);
    }
    else
    {
        mEditor->scrubTo(mEditor->layers()->firstKeyFrameIndex());
    }
    mEditor->playback()->stopSounds();
}

void TimeControls::jumpToEndButtonClicked()
{
    if (mPlaybackRangeCheckBox->isChecked())
    {
        mEditor->scrubTo(mLoopEndSpinBox->value());
    }
    else
    {
        mEditor->scrubTo(mEditor->layers()->lastKeyFrameIndex());
    }
}

void TimeControls::loopButtonClicked(bool bChecked)
{
    mEditor->playback()->setLooping(bChecked);
}

void TimeControls::playbackRangeClicked(bool bChecked)
{
    mEditor->playback()->enableRangedPlayback(bChecked);
}

void TimeControls::loopStartValueChanged(int i)
{
    if (i >= mLoopEndSpinBox->value())
    {
        mLoopEndSpinBox->setValue(i + 1);
    }
    mLoopEndSpinBox->setMinimum(i + 1);

    mEditor->playback()->setRangedStartFrame(i);
    mTimeline->updateLength();
}

void TimeControls::loopEndValueChanged(int i)
{
    mEditor->playback()->setRangedEndFrame(i);
    mTimeline->updateLength();
}

void TimeControls::updateSoundIcon(bool soundEnabled)
{
    if (soundEnabled)
    {
        mSoundButton->setIcon(QIcon(":icons/controls/sound.png"));
    }
    else
    {
        mSoundButton->setIcon(QIcon(":icons/controls/sound-disabled.png"));
    }
}

void TimeControls::updateSoundScrubIcon(bool soundScrubEnabled)
{
    if (soundScrubEnabled)
    {
        mSoundScrubButton->setIcon(QIcon(":icons/controls/soundscrub.png"));
        mEditor->playback()->setSoundScrubActive(true);
        mEditor->preference()->set(SETTING::SOUND_SCRUB_ACTIVE, true);
    }
    else
    {
        mSoundScrubButton->setIcon(QIcon(":icons/controls/soundscrub-disabled.png"));
        mEditor->playback()->setSoundScrubActive(false);
        mEditor->preference()->set(SETTING::SOUND_SCRUB_ACTIVE, false);
    }
}

void TimeControls::noTimecodeText()
{
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue(SETTING_TIMECODE_TEXT, NOTEXT);
    mTimecodeLabelEnum = NOTEXT;
    mTimecodeLabel->setToolTip(tr(""));
    updateTimecodeLabel(mEditor->currentFrame());
}

void TimeControls::onlyFramesText()
{
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue(SETTING_TIMECODE_TEXT, FRAMES);
    mTimecodeLabelEnum = FRAMES;
    mTimecodeLabel->setToolTip(tr("Actual frame number"));
    updateTimecodeLabel(mEditor->currentFrame());
}

void TimeControls::SFF_text()
{
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue(SETTING_TIMECODE_TEXT, SFF);
    mTimecodeLabelEnum = SFF;
    mTimecodeLabel->setToolTip(tr("Timecode format S:FF"));
    updateTimecodeLabel(mEditor->currentFrame());
}

void TimeControls::SMPTE_text()
{
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue(SETTING_TIMECODE_TEXT, SMPTE);
    mTimecodeLabelEnum = SMPTE;
    mTimecodeLabel->setToolTip(tr("Timecode format MM:SS:FF"));
    updateTimecodeLabel(mEditor->currentFrame());
}

void TimeControls::onFpsEditingFinished()
{
    mFpsBox->clearFocus();
    emit fpsChanged(mFpsBox->value());
    mFps = mFpsBox->value();
}

void TimeControls::updateTimecodeLabel(int frame)
{
    switch (mTimecodeLabelEnum)
    {
    case TimecodeTextLevel::SMPTE:
        mTimecodeLabel->setText(QString("%1:%2:%3")
                                .arg(QString::number(frame / (60 * mFps) % 60).rightJustified(2,'0'))
                                .arg(QString::number(frame / mFps % 60).rightJustified(2, '0'))
                                .arg(QString::number(frame % mFps).rightJustified(2, '0')));
        break;
    case TimecodeTextLevel::SFF:
        mTimecodeLabel->setText(QString("%1:%2")
                                .arg(QString::number(frame / mFps))
                                .arg(QString::number(frame % mFps).rightJustified(2, '0')));
        break;
    case TimecodeTextLevel::FRAMES:
        mTimecodeLabel->setText(tr("%1").arg(QString::number(frame).rightJustified(4, '0')));
        break;
    case TimecodeTextLevel::NOTEXT:
    default:
        mTimecodeLabel->setText("");
        break;
    }


}

void TimeControls::updateLength(int frameLength)
{
    mLoopStartSpinBox->setMaximum(frameLength - 1);
    mLoopEndSpinBox->setMaximum(frameLength);
}
