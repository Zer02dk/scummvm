/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// The factory (location 6), written from notes/factory.md. Part 1: the
// outside (rooms 13..15) and what lies there, the trees and the saw, the
// hall light switch and the ceiling light, the peels' sorting. Part 2: the
// line in halls 5..8 (glass machine, rinser, potato machine, lid machine).
// Part 3: halls 9..12 (autoclave + close-up 16, hose, hole, labels, trays,
// shrink plastic, cardboard machine, the finished pack).

#include "common/events.h"
#include "common/system.h"
#include "graphics/framelimiter.h"

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/response_data.h"

namespace Miljostrup {

enum {
	kFactory = 6,
	kUsedPaper = 101,
	kColouredGlass = 102,
	kClearGlass = 103,
	kNewPaper = 104,
	kSaw = 105,
	kPePlastic = 106,
	kPvcPlastic = 107,
	kPeels = 109,
	kAluminium = 108,
	kGlassMachine = 113,   // room 5: +0x2f clear glass in, +0x30 coloured, +0x31 run
	kRinser = 116,         // room 6: +0x2f pending, +0x30 automatic
	kRinserSwitch = 117,
	kPotatoMachine = 120,  // room 7: +0x2f pending
	kPeelsProxy = 121,
	kLidMachine = 124,     // room 8: +0x2f pending, +0x30 aluminium
	kAutoclave = 127,      // room 9: +0x2f pending, +0x30 temperature (0 88°C, 1 100°C,
	                       // 2 118°C), +0x31 pressure, +0x32 set, +0x33 jars in, +0x34 to award
	kTrays = 110,
	kTape = 111,
	kHose = 128,           // +0x2f to the radiator
	kSewer = 129,
	kRadiator = 130,
	kHole = 131,           // +0x2f patched
	kLabelMachine = 135,   // room 10: +0x2f pending
	kTrayMachine = 138,    // room 11: +0x2f pending, +0x30 has trays, +0x31 jars waiting
	kShrinkMachine = 139,  // room 11: +0x2f has plastic, +0x30 PE
	kCardboardMachine = 144, // room 12: +0x2f paper, +0x30 new paper
	kTraysProxy = 145,
	kPack = 146,           // room 12: +0x2f pending, +0x30 arrived
	kLightSwitch = 142,    // room 12, +0x2f on (start 1)
	kCeilingLight = 143,   // a region in every hall
	kTrees = 148,          // room 13, +0x2f the count (start 3)
	kBin = 156,
	kDeadAnimals = 157,
	kFeedBarrel = 158,
	kField = 159,
	kOrganicField = 160,
	// engine IDs of the line's figures (rooms.EXTRA_FIGURES / FIGURE_IDS)
	kJars5 = 951,          // 951..953
	kJars6 = 954,          // 954..956
	kPotatoPartA = 957,    // hall 7 +0x726 (hidden until the machine runs)
	kPotatoPartB = 958,    // hall 7 +0x72a
	kJars7 = 959,          // 959..961, hidden at the start
	kLidPart = 962,        // hall 8 +0x726
	kJars8 = 963,          // 963..965
	kAutoclaveFigure = 966,
	kJars9 = 967,          // 967..969
	kLabelPart = 970,
	kJars10 = 971,         // 971..973
	kPackFigure = 974,     // hall 11 +0x726
	kJars11 = 975,         // 975..977
	kShrinkPart = 978,     // hall 11 +0x736
	kLineFirst = 951,
	kLineLast = 978
};

// An engine-only field (not in the original object): the cardboard machine's
// figure is shown from the paper (44f3) until its rule hides it (0x20)
static const int kWorking = 0x1ff;

// The halls' messages (event type 3, notes/factory.md "The line")
enum {
	kMsgDone = 0x1f,
	kMsgRun = 0x20,
	kMsgStop = 0x21,
	kMsg22 = 0x22,
	kMsg23 = 0x23,
	kMsg24 = 0x24,
	kMsg25 = 0x25
};

void MiljostrupEngine::initFactory() {
	// 6000:145e / 1693
	setField(kTrees, 0x2f, 3);
	setField(kLightSwitch, 0x2f, 1);
	_lineWaits.clear();
	for (int i = 0; i < ARRAYSIZE(_lineShown); ++i)
		_lineShown[i] = false;
}

int MiljostrupEngine::getFactoryLineState(int object, bool &visible) const {
	// the line's figures at the room's build: halls 5 / 6 shown in state 1;
	// hall 7's made hidden (5b25:0380), the jars in state 4; hall 8's shown
	// in their last state (5ab2:03b7)
	visible = true;
	if (object >= kPotatoPartA && object < kJars7 + 3) {
		visible = _lineShown[object - kLineFirst];
		return object >= kJars7 ? 4 : 1;
	}
	switch (object) {
	case kLidPart:
	case kAutoclaveFigure:
	case kJars9: case kJars9 + 1: case kJars9 + 2:
	case kLabelPart:
	case kJars11: case kJars11 + 1: case kJars11 + 2:
		return 2;
	case kJars8: case kJars8 + 1: case kJars8 + 2:
	case kJars10: case kJars10 + 1: case kJars10 + 2:
	case kShrinkPart:
		return 3;
	case kPackFigure:
		return 5;
	default:
		return 1;
	}
}

void MiljostrupEngine::waitForLine(int figure, int message) {
	// 2381:0065: the message comes when the figure's walks have ended
	LineWait w = { figure, message };
	_lineWaits.push_back(w);
}

void MiljostrupEngine::showLineFigure(int figure) {
	_lineShown[figure - kLineFirst] = true;
	_scene->setFigureVisible(figure, true);
}

void MiljostrupEngine::factoryLeavingRoom() {
	_lineWaits.clear();
	for (int i = 0; i < ARRAYSIZE(_lineShown); ++i)
		_lineShown[i] = false;
}

void MiljostrupEngine::factoryRoomEntered() {
	// The stations' rules when the hall is built (also again after an
	// award): 1c28 runs a pending station
	if (_location != kFactory)
		return;
	switch (_room) {
	case 6:
		runStationIfPending(kRinser);
		break;
	case 7:
		runStationIfPending(kPotatoMachine);
		break;
	case 8:
		runStationIfPending(kLidMachine);
		break;
	case 9:
		autoclaveRule();
		runStationIfPending(kAutoclave);
		break;
	case 10:
		runStationIfPending(kLabelMachine);
		break;
	case 11:
		shrinkMachineRule(); // 139 is bound before 138
		if (!getField(kTrayMachine, 0x2f) && getField(kTrayMachine, 0x31))
			factoryMessage(kMsg22); // 3a19: the jars wait at the machine
		runStationIfPending(kTrayMachine);
		break;
	case 12:
		runStationIfPending(kPack);
		break;
	default:
		break;
	}
}

void MiljostrupEngine::autoclaveRule() {
	// 6000:2c15: set and the jars in -> boil (0x22); a new setting -> its
	// award (VMT 0x5c = 304f)
	if (!getField(kAutoclave, 0x34)) {
		if (getField(kAutoclave, 0x33) && getField(kAutoclave, 0x32)) {
			setField(kAutoclave, 0x33, 0);
			factoryMessage(kMsg22);
		}
	} else {
		autoclaveAward();
	}
}

void MiljostrupEngine::autoclaveAward() {
	// 6000:304f -> 2ce7(value): Award(value, value, 0, 0)
	setField(kAutoclave, 0x34, 0);
	switch (getField(kAutoclave, 0x30)) {
	case 0:
		award(-10, -10, 0, 0, 0x60002d43, 0);
		break;
	case 1:
		award(-5, -5, 0, 0, 0x60002dd9, 0);
		break;
	default:
		if (getField(kAutoclave, 0x31))
			award(10, 10, 0, 0, 0x60002eb6, 0);
		else
			award(-5, -5, 0, 0, 0x60002fa1, 0);
		break;
	}
}

void MiljostrupEngine::shrinkMachineRule() {
	// 6000:3cc5: its figure while it has plastic, and then 0x20
	refreshObject(kShrinkMachine);
	if (getField(kShrinkMachine, 0x2f))
		factoryMessage(kMsgRun);
}

void MiljostrupEngine::runAutoclaveScreen() {
	// Close-up 16 (5712): picture 1371 (0x55b), palette A; the pressure's
	// gauge 991 / 992 at (192, 73); radio groups (1ccf): pressure on / off
	// (1db8:1b72 / 1df6, commands 0x40 / 0x41) at (170, 93) / (170, 113),
	// temperature 118 / 100 / 88°C (1372..1374, commands 0x42..0x44) at (82,
	// 72 / 92 / 112), light 16; OK 0x46 (132, 184), Cancel 0x45 (259, 184);
	// Enter / Space = OK, Esc = Cancel; each choice sound 1d26:0155. OK:
	// 6000:2cb6(pressure, temperature)
	Graphics::ManagedSurface *background = _archive.loadImage(1371);
	Graphics::ManagedSurface *gauge[2] = { _archive.loadImage(992), _archive.loadImage(991) };
	Graphics::ManagedSurface *temp[3] = { _archive.loadImage(1374), _archive.loadImage(1373), _archive.loadImage(1372) };
	Graphics::ManagedSurface *on = loadImageObject(_exe, kButtonOnImage);
	Graphics::ManagedSurface *off = loadImageObject(_exe, kButtonOffImage);
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *cancel = loadImageObject(_exe, kButtonCancelImage);
	static const int kTempY[3] = { 112, 92, 72 }; // by temperature 0..2
	int temperature = getField(kAutoclave, 0x30);
	bool pressure = getField(kAutoclave, 0x31) != 0;

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);
	int result = 0;
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!result && !shouldQuit()) {
		if (redraw) {
			if (background)
				_screen->blitFrom(*background, Common::Point(0, 0));
			if (gauge[pressure])
				_screen->transBlitFrom(*gauge[pressure], Common::Point(192, 73), gauge[pressure]->getTransparentColor());
			if (on && off) {
				drawBevelButton(_screen, *on, 170, 93, pressure, 1, 1, 16);
				drawBevelButton(_screen, *off, 170, 113, !pressure, 1, 1, 16);
			}
			for (int t = 0; t < 3; ++t) {
				if (temp[t])
					drawBevelButton(_screen, *temp[t], 82, kTempY[t], t == temperature, 1, 1, 16);
			}
			if (ok)
				drawBevelButton(_screen, *ok, 132, 184, false);
			if (cancel)
				drawBevelButton(_screen, *cancel, 259, 184, false);
			redraw = false;
		}
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				if (isOkKey(e))
					result = 1;
				else if (e.kbd.keycode == Common::KEYCODE_ESCAPE)
					result = -1;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				const Common::Point &m = e.mouse;
				if (ok && buttonRect(*ok, 132, 184).contains(m)) {
					if (trackButton(*ok, 132, 184))
						result = 1;
				} else if (cancel && buttonRect(*cancel, 259, 184).contains(m)) {
					if (trackButton(*cancel, 259, 184))
						result = -1;
				} else if (on && buttonRect(*on, 170, 93).contains(m)) {
					pressure = true;
					redraw = true;
					playSound(kSoundClick);
				} else if (off && buttonRect(*off, 170, 113).contains(m)) {
					pressure = false;
					redraw = true;
					playSound(kSoundClick);
				} else {
					for (int t = 0; t < 3; ++t) {
						if (temp[t] && buttonRect(*temp[t], 82, kTempY[t]).contains(m)) {
							temperature = t;
							redraw = true;
							playSound(kSoundClick);
						}
					}
				}
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	delete background;
	for (int i = 0; i < 2; ++i)
		delete gauge[i];
	for (int t = 0; t < 3; ++t)
		delete temp[t];
	delete on;
	delete off;
	delete ok;
	delete cancel;
	if (result == 1) {
		setField(kAutoclave, 0x30, temperature);
		setField(kAutoclave, 0x31, pressure ? 1 : 0);
		setField(kAutoclave, 0x32, 1);
		setField(kAutoclave, 0x34, 1);
	}
}

void MiljostrupEngine::tickFactory() {
	if (_location != kFactory)
		return;
	for (uint i = 0; i < _lineWaits.size(); ++i) {
		if (!_scene->isFigureBusy(_lineWaits[i].figure)) {
			int message = _lineWaits[i].message;
			_lineWaits.remove_at(i);
			factoryMessage(message);
			return;
		}
	}
}

void MiljostrupEngine::runStationIfPending(int station) {
	// 6000:1c28: VMT 0x64 while +0x2f is set
	if (!getField(station, 0x2f))
		return;
	switch (station) {
	case kRinser: // 2278
		setField(kRinser, 0x2f, 0);
		if (!getField(kRinser, 0x30)) {
			_scene->queueFigureWalk(kRinser, 3, 0, 0, 48);
		} else {
			_scene->queueFigureWalk(kRinser, 1, 0, 0, 11);
			_scene->queueFigureWalk(kRinser, 2, 0, 0, 36);
			_scene->queueFigureWalk(kRinser, 1, 0, 0, 1);
		}
		waitForLine(kRinser, kMsgDone);
		factoryMessage(kMsgRun);
		break;
	case kPotatoMachine: // 2722
		setField(kPotatoMachine, 0x2f, 0);
		factoryMessage(kMsgRun);
		break;
	case kLidMachine: // 293f: without aluminium the jars stop (still pending)
		if (!getField(kLidMachine, 0x30)) {
			factoryMessage(kMsgStop);
		} else {
			setField(kLidMachine, 0x2f, 0);
			factoryMessage(kMsgRun);
		}
		break;
	case kAutoclave:     // 2c64
	case kLabelMachine:  // 3833
		setField(station, 0x2f, 0);
		factoryMessage(kMsgDone);
		break;
	case kTrayMachine:   // 39f4
		setField(kTrayMachine, 0x2f, 0);
		setField(kTrayMachine, 0x31, 1);
		factoryMessage(kMsg24);
		break;
	case kPack:          // 46a8: the finished pack slides in
		setField(kPack, 0x2f, 0);
		setField(kPack, 0x30, 1);
		refreshObject(kPack);
		_scene->setFigurePos(kPack, -21, 82);
		_scene->queueFigureWalk(kPack, 1, 68, 0, 17);
		_scene->queueFigureWalk(kPack, 1, 0, 0, 1);
		waitForLine(kPack, kMsgStop);
		break;
	default:
		break;
	}
}

void MiljostrupEngine::factoryMessage(int message) {
	// The halls' message handlers (5c09:014a, 5b9a:010d, 5b25:01f0, 5ab2:021b)
	switch (_room) {
	case 5:
		if (message == kMsgRun) {
			// 5c09:0011: the jars come out of the machine, one after another
			for (int k = 0; k < 3; ++k) {
				int jar = kJars5 + k;
				_scene->setFigurePos(jar, 147, 80);
				_scene->queueFigureWalk(jar, 1, 0, 0, 12 * k + 9);
				_scene->queueFigureWalk(jar, 2, 0, 0, 2);
				_scene->queueFigureWalk(jar, 3, 24, 0, 6);
				_scene->queueFigureWalk(jar, 4, 144, 0, 36);
				_scene->queueFigureWalk(jar, 1, 0, 0, 1);
			}
			setInputOff(true);
		} else if (message == kMsgDone) {
			// 5c09:0110: the machine's rule, its VMT 0x5c (20ea), sound
			_scene->setFigureState(kGlassMachine, 1);
			setInputOff(false);
			glassMachineAward();
			playSound(kSoundClick);
		}
		break;
	case 6:
		if (message == kMsgRun) {
			// 5b9a:002e: the jars through the rinser; the potato machine next
			for (int k = 0; k < 3; ++k) {
				int jar = kJars6 + k;
				_scene->setFigurePos(jar, 48 * k - 5, 82);
				_scene->queueFigureWalk(jar, 2, 324 - 48 * k, 0, 81 - 12 * k);
				_scene->queueFigureWalk(jar, 1, 0, 0, 1);
			}
			setField(kPotatoMachine, 0x2f, 1);
			setInputOff(true);
		} else if (message == kMsgDone) {
			setInputOff(false);
		}
		break;
	case 7:
		if (message == kMsgRun) {
			// 5b25:0077: the machine's parts and the jars shown; the lid
			// machine next, the peels appear (HYPOTHESIS: the proxy's VMT
			// 0x60(1) shows it)
			showLineFigure(kPotatoPartA);
			_scene->queueFigureWalk(kPotatoPartA, 1, 0, 0, 82);
			waitForLine(kPotatoPartA, kMsgDone);
			showLineFigure(kPotatoPartB);
			for (int k = 0; k < 3; ++k) {
				int jar = kJars7 + k;
				showLineFigure(jar);
				_scene->setFigurePos(jar, 48 * k - 8, 82);
				_scene->queueFigureWalk(jar, 1, 164 - 48 * k, 0, 41 - 12 * k);
				_scene->queueFigureWalk(jar, 2, 4, 0, 1);
				_scene->queueFigureWalk(jar, 3, 156, 0, 39);
				_scene->queueFigureWalk(jar, 4, 0, 0, 1);
			}
			setField(kLidMachine, 0x2f, 1);
			setFlag(kPeelsProxy, 1);
		} else if (message == kMsgDone) {
			// 5b25:0000: the parts hidden, the peels' rule, input on
			_lineShown[kPotatoPartA - kLineFirst] = _lineShown[kPotatoPartB - kLineFirst] = false;
			_scene->setFigureVisible(kPotatoPartB, false);
			_scene->setFigureVisible(kPotatoPartA, false);
			refreshObject(kPeelsProxy);
			setInputOff(false);
		}
		break;
	case 8:
		if (message == kMsgStop) {
			// 5ab2:01a2: the jars wait before the machine
			for (int k = 0; k < 3; ++k) {
				_scene->setFigurePos(kJars8 + k, 48 * k - 3, 82);
				_scene->setFigureState(kJars8 + k, 1);
			}
		} else if (message == kMsgRun) {
			// 5ab2:003d: the machine works, the jars go on; the autoclave next
			_scene->queueFigureWalk(kLidPart, 2, 0, 0, 1);
			_scene->queueFigureWalk(kLidPart, 1, 0, 0, 36);
			_scene->queueFigureWalk(kLidPart, 2, 0, 0, 1);
			waitForLine(kLidPart, kMsgDone);
			for (int k = 0; k < 3; ++k) {
				int jar = kJars8 + k;
				_scene->setFigurePos(jar, 48 * k - 3, 82);
				_scene->queueFigureWalk(jar, 1, 136 - 48 * k, 0, 34 - 12 * k);
				_scene->queueFigureWalk(jar, 2, 184, 0, 46);
				_scene->queueFigureWalk(jar, 3, 0, 0, 1);
			}
			setField(kAutoclave, 0x2f, 1);
			setInputOff(true);
		} else if (message == kMsgDone) {
			// 5ab2:0000: the lid machine's VMT 0x5c = 2a58, command 0x40
			setInputOff(false);
			award(0, 0, 0, 0, 0x6000298b, 0);
		}
		break;
	case 9:
		if (message == kMsgDone) {
			// 59fb:0000: the jars to the autoclave
			for (int k = 0; k < 3; ++k) {
				int jar = kJars9 + k;
				_scene->setFigurePos(jar, 48 * k - 1, 82);
				_scene->queueFigureWalk(jar, 1, 160 - 48 * k, 0, 40 - 12 * k);
				_scene->queueFigureWalk(jar, 2, 0, 0, 1);
			}
			waitForLine(kJars9, kMsgRun);
			setInputOff(true);
		} else if (message == kMsgRun) {
			// 59fb:00e9: the jars are in (2cd8), the autoclave's rule
			setInputOff(false);
			setField(kAutoclave, 0x33, 1);
			autoclaveRule();
			playSound(kSoundClose);
		} else if (message == kMsg22) {
			// 59fb:0176: boiling
			setInputOff(true);
			for (int k = 0; k < 5; ++k) {
				_scene->queueFigureWalk(kAutoclaveFigure, 2, 0, 0, 1);
				_scene->queueFigureWalk(kAutoclaveFigure, 1, 0, 0, 1);
			}
			_scene->queueFigureWalk(kAutoclaveFigure, 2, 0, 0, 1);
			_scene->queueFigureWalk(kAutoclaveFigure, 1, 0, 0, 30);
			_scene->queueFigureWalk(kAutoclaveFigure, 2, 0, 0, 1);
			waitForLine(kAutoclaveFigure, kMsgStop);
		} else if (message == kMsgStop) {
			// 59fb:0134: the label machine next, the door's sound
			setField(kLabelMachine, 0x2f, 1);
			playSound(kSoundOpen);
			factoryMessage(kMsg23);
		} else if (message == kMsg23) {
			// 59fb:0318: the jars out
			for (int k = 0; k < 3; ++k) {
				int jar = kJars9 + k;
				_scene->setFigurePos(jar, 183, 82);
				_scene->queueFigureWalk(jar, 2, 0, 0, 12 * k + 1);
				_scene->queueFigureWalk(jar, 1, 136, 0, 34);
				_scene->queueFigureWalk(jar, 2, 0, 0, 1);
			}
			waitForLine(kJars9, kMsg24);
		} else if (message == kMsg24) {
			// 59fb:03e1: input on, the hose's VMT 0x5c = 3532
			setInputOff(false);
			if (getField(kHose, 0x2f))
				award(10, 10, 0, 0, 0x60003473, 0);
		}
		break;
	case 10:
		if (message == kMsgDone) {
			// 5999:0000: labels on, the tray machine next
			_scene->queueFigureWalk(kLabelPart, 2, 0, 0, 9);
			_scene->queueFigureWalk(kLabelPart, 1, 0, 0, 36);
			_scene->queueFigureWalk(kLabelPart, 2, 0, 0, 1);
			waitForLine(kLabelPart, kMsgRun);
			for (int k = 0; k < 3; ++k) {
				int jar = kJars10 + k;
				_scene->setFigurePos(jar, 48 * k - 2, 82);
				_scene->queueFigureWalk(jar, 1, 4 * (36 - 12 * k), 0, 36 - 12 * k);
				_scene->queueFigureWalk(jar, 2, 172, 0, 43);
				_scene->queueFigureWalk(jar, 3, 0, 0, 1);
			}
			setField(kTrayMachine, 0x2f, 1);
		} else if (message == kMsgRun) {
			setInputOff(false);
		}
		break;
	case 11:
		if (message == kMsg24) {
			// 58ff:0000: the jars to the tray machine
			for (int k = 0; k < 3; ++k) {
				int jar = kJars11 + k, steps = 31 - 10 * k;
				_scene->setFigurePos(jar, 48 * k, 82);
				_scene->queueFigureWalk(jar, 1, 4 * steps, 0, steps);
				_scene->queueFigureWalk(jar, 1, 0, 0, 1);
			}
			waitForLine(kJars11, kMsg22);
			setInputOff(true);
		} else if (message == kMsg22) {
			// 58ff:00f6: they wait; on with trays and plastic
			for (int k = 0; k < 3; ++k) {
				_scene->setFigurePos(kJars11 + k, 124 + 8 * k, 82);
				_scene->setFigureState(kJars11 + k, 1);
			}
			if (getField(kTrayMachine, 0x30) && getField(kShrinkMachine, 0x2f))
				factoryMessage(kMsgDone);
			else
				setInputOff(false);
		} else if (message == kMsgDone) {
			// 58ff:01b2: into the tray, through the shrink plastic; the pack
			// in hall 12 next
			setField(kTrayMachine, 0x31, 0);
			for (int k = 0; k < 3; ++k) {
				_scene->queueFigureWalk(kJars11 + k, 1, 0, 0, 4);
				_scene->queueFigureWalk(kJars11 + k, 2, 0, 0, 1);
			}
			_scene->setFigurePos(kPackFigure, 123, 82);
			_scene->queueFigureWalk(kPackFigure, 1, 0, 0, 3);
			_scene->queueFigureWalk(kPackFigure, 2, 0, 0, 1);
			_scene->queueFigureWalk(kPackFigure, 2, 48, 0, 12);
			_scene->queueFigureWalk(kPackFigure, 3, 0, 0, 4);
			_scene->queueFigureWalk(kPackFigure, 4, 148, 0, 37);
			_scene->queueFigureWalk(kPackFigure, 5, 0, 0, 1);
			_scene->queueFigureWalk(kShrinkPart, 1, 0, 0, 12);
			_scene->queueFigureWalk(kShrinkPart, 2, 0, 0, 8);
			_scene->queueFigureWalk(kShrinkPart, 1, 0, 0, 1);
			_scene->queueFigureWalk(kShrinkPart, 1, 0, 0, 12);
			waitForLine(kShrinkPart, kMsg25);
			setField(kPack, 0x2f, 1);
			setInputOff(true);
		} else if (message == kMsg25) {
			// 58ff:03a2: the shrink machine's VMT 0x5c = 3fb4
			setInputOff(false);
			if (!getField(kShrinkMachine, 0x2f))
				break;
			if (getField(kShrinkMachine, 0x30)) {
				award(10, 0, 10, 0, 0x60003d9b, 0x60003e68);
			} else {
				award(-10, 0, -10, 0, 0x60003e6b, 0x60003fad);
				setQuiz(0x60003f46, 0x60003f8f, 0x60003fa1, 1, 2);
			}
		} else if (message == kMsgRun) {
			// 58ff:03ca
			_scene->setFigureState(kShrinkPart, 1);
		}
		break;
	case 12:
		if (message == kMsgDone) {
			// 588d:0000 -> 6000:457e: the trays come out of the machine
			// the state first: a hidden figure takes its new state when it is
			// shown (Scene::showFigure), so state 1 (the full stack) is not seen
			_scene->setFigureState(kTraysProxy, 3);
			_scene->setFigureVisible(kTraysProxy, true);
			_scene->queueFigureWalk(kTraysProxy, 2, 0, 0, 28);
			_scene->queueFigureWalk(kTraysProxy, 1, 0, 0, 1);
			waitForLine(kTraysProxy, kMsgRun);
			setInputOff(true);
		} else if (message == kMsgRun) {
			// 588d:0039: the trays shown (HYPOTHESIS: their VMT 0x64(1)), the
			// machine's rule and its VMT 0x5c = 443a
			setFlag(kTraysProxy, 1);
			refreshObject(kTraysProxy);
			setField(kCardboardMachine, kWorking, 0);
			refreshObject(kCardboardMachine);
			setInputOff(false);
			if (!getField(kCardboardMachine, 0x30)) {
				award(9, 3, 3, 3, 0x6000435e, 0);
			} else {
				award(-10, -5, 0, -5, 0x600042a4, 0);
				setQuiz(0x60004311, 0x60004357, 0x6000435a, 1, 1);
			}
		} else if (message == kMsgStop) {
			setInputOff(false);
		}
		break;
	default:
		break;
	}
}

void MiljostrupEngine::glassMachineAward() {
	// 6000:20ea -> 1e6c(value): Award(value, 0, value, 0), page "20"
	if (getField(kGlassMachine, 0x2f))
		award(10, 0, 10, 0, 0x60001ec8, 0x600020e7);
	else if (getField(kGlassMachine, 0x30))
		award(-5, 0, -5, 0, 0x60001f6e, 0x600020e7);
	else
		award(0, 0, 0, 0, 0x60002034, 0x600020e7);
}

int MiljostrupEngine::getObjectNode(int object, int regionNode) const {
	// VMT 0x24 (ae72:0e21 walks there): the trees' 6000:14fa returns their
	// count, so each tree has its own place (nodes 3, 2, 1 for 3, 2, 1 trees)
	if (_location == kFactory && object == kTrees && getField(kTrees, 0x2f) > 0)
		return getField(kTrees, 0x2f);
	return regionNode;
}

bool MiljostrupEngine::isFactoryObjectEnabled(int object, bool &enabled) const {
	// 6000:15c3: the trees' second part (+8, HYPOTHESIS: the region) while
	// any tree is left
	if (object == kTrees) {
		enabled = getField(kTrees, 0x2f) > 0;
		return true;
	}
	return false;
}

int MiljostrupEngine::getFactoryFigureState(uint32 rule, int object, bool &visible) const {
	switch (rule) {
	case 0x600015c3: { // the trees: state = the count, hidden at 0
		int count = getField(kTrees, 0x2f);
		visible = count > 0;
		return count > 0 ? count : 1;
	}
	case 0x60001b5e: // the ceiling light: shown while the switch is off
		visible = getField(kLightSwitch, 0x2f) == 0;
		return 1;
	case 0x60001dfb: // the glass machine
		return 1;
	case 0x60002220: // the rinser: automatic 1, spraying all the time 3
		return getField(kRinser, 0x30) ? 1 : 3;
	case 0x6000261c: // its switch: automatic 1, off 2
		return getField(kRinser, 0x30) ? 1 : 2;
	case 0x600028ee: // the lid machine: shown while it has no aluminium
		visible = getField(kLidMachine, 0x30) == 0;
		return 1;
	case 0x60003424: // the hose: into the sewer 1, to the radiator 2
		return getField(kHose, 0x2f) ? 2 : 1;
	case 0x60003672: // the hole: spraying 1, patched 2
		return getField(kHole, 0x2f) ? 2 : 1;
	case 0x60003cc5: // the shrink-plastic machine: shown while it has plastic
		visible = getField(kShrinkMachine, 0x2f) != 0;
		return 1;
	case 0x6000420b: // the cardboard machine: hidden; 44f3 shows it while it works
		visible = getField(kCardboardMachine, kWorking) != 0;
		return 1;
	case 0x60003a19: // the tray machine (no figure): its rule is run by factoryRoomEntered
		return 0;
	case 0x6000465d: // the finished pack: there 1, not yet 2
		return getField(kPack, 0x30) ? 1 : 2;
	default:
		return 0;
	}
}

void MiljostrupEngine::sortPeels(int target) {
	// 6000:0f72 -> 0b8c(points, energy, waste, water, text): Award, out of
	// the inventory, command 0x40; the quizzes b06f:1185
	switch (target) {
	case kOrganicField:
		award(10, 0, 10, 0, 0x60000bff, 0);
		break;
	case kField:
		award(10, 0, 10, 0, 0x60000c9a, 0);
		break;
	case kDeadAnimals: // "Hvad destruer man?" Komøg / Døde dyr
		award(-10, 0, -10, 0, 0x60000d0e, 0);
		setQuiz(0x60000dbe, 0x60000dd1, 0x60000dd7, 0, 2);
		break;
	case kFeedBarrel:
		award(0, 0, 0, 0, 0x60000de0, 0);
		break;
	case kBin: // "Kan skræller fra kartofler komme i konpostbeholderen?" Ja / Nej
		award(-10, 0, -10, 0, 0x60000e89, 0);
		setQuiz(0x60000f1f, 0x60000f55, 0x60000f58, 1, 2);
		break;
	default:
		say(handlerText(0x60000f5c), 14, true);
		return;
	}
	removeObject(kPeels);
	if (target != kOrganicField && target != kField)
		playSound(kSoundClose);
}

bool MiljostrupEngine::runFactoryHandler(uint32 handler, int verb, int object, int second) {
	if (_location != kFactory)
		return false;
	switch (handler) {
	// The items whose class serves two IDs (notes/factory.md "Carried items")
	case 0x600000dd: // glass: Se på
		say(handlerText(object == kClearGlass ? 0x600000a8 : 0x600000c2), 14, true);
		break;
	case 0x60000367: // paper: Se på
		say(handlerText(object == kUsedPaper ? 0x6000033b : 0x6000034f), 14, true);
		break;
	case 0x60000584: // plastic: Se på
		say(handlerText(object == kPePlastic ? 0x60000555 : 0x6000056c), 14, true);
		break;

	// The saw 105: Benyt with the trees 148 (6000:099f)
	case 0x6000099f:
		if (second != kTrees) {
			sayRootMessage(kVerbUse);
			break;
		}
		if (getField(kTrees, 0x2f) > 0)
			setField(kTrees, 0x2f, getField(kTrees, 0x2f) - 1);
		refreshObject(kTrees);
		if (getField(kTrees, 0x2f) == 0)
			award(0, 0, 0, 0, 0x60000929, 0);
		else
			say(handlerText(0x60000998), 14, true);
		break;

	// The peels 109 (6000:0f72)
	case 0x60000f72:
		sortPeels(second);
		break;

	// The trees 148: Se på (6000:1561)
	case 0x60001561:
		say(handlerText(getField(kTrees, 0x2f) == 1 ? 0x60001513 : 0x60001539), 14, true);
		break;

	// The light switch 142 (room 12)
	case 0x6000170d: // Se på
		say(handlerText(getField(kLightSwitch, 0x2f) ? 0x600016eb : 0x600016fb), 14, true);
		break;
	case 0x600017a5: // Tænd: it cannot be switched on again
		say(handlerText(getField(kLightSwitch, 0x2f) ? 0x6000176f : 0x60001788), 14, true);
		break;
	case 0x600018f5: // Sluk: only once the trees are gone
		if (!getField(kLightSwitch, 0x2f)) {
			say(handlerText(0x600018da), 14, true);
		} else if (getField(kTrees, 0x2f) >= 1) {
			say(handlerText(0x60001807), 14, true);
		} else {
			setField(kLightSwitch, 0x2f, 0);
			refreshObject(kCeilingLight);
			playSound(kSoundClick);
			award(10, 10, 0, 0, 0x60001823, 0);
		}
		break;

	// Used glass 102 / 103: Benyt with the glass machine (6000:01cc)
	case 0x600001cc:
		if (second != kGlassMachine) {
			say(handlerText(0x600001b1), 14, true);
		} else if (getField(kGlassMachine, 0x2f) || getField(kGlassMachine, 0x30)) {
			say(handlerText(0x60000185), 14, true);
		} else {
			removeObject(object);
			setField(kGlassMachine, object == kClearGlass ? 0x2f : 0x30, 1);
			say(handlerText(0x600001aa), 14, true);
			playSound(kSoundClose);
		}
		break;

	// Aluminium 108: Benyt with the lid machine (6000:080b)
	case 0x6000080b:
		if (second != kLidMachine) {
			sayRootMessage(kVerbUse);
			break;
		}
		removeObject(kAluminium);
		setField(kLidMachine, 0x30, 1);
		refreshObject(kLidMachine);
		say(handlerText(0x60000804), 14, true);
		playSound(kSoundClose);
		runStationIfPending(kLidMachine); // its rule 28ee -> 1c28
		break;

	// The glass machine 113: Tænd (6000:1d16)
	case 0x60001d16:
		if (getField(kGlassMachine, 0x31)) {
			say(handlerText(0x60001cf5), 14, true);
			break;
		}
		setField(kGlassMachine, 0x31, 1);
		setField(kRinser, 0x2f, 1);
		_scene->queueFigureWalk(kGlassMachine, 2, 0, 0, 36);
		waitForLine(kGlassMachine, kMsgDone);
		factoryMessage(kMsgRun);
		playSound(kSoundClick);
		break;

	// The rinser's switch 117 (6000:24c9 / 25b6)
	case 0x600024c9: // Tænd: the automatic control
		if (getField(kRinser, 0x30)) {
			say(handlerText(0x60002409), 14, true);
			break;
		}
		setField(kRinser, 0x30, 1);
		refreshObject(kRinser);
		refreshObject(kRinserSwitch);
		runStationIfPending(kRinser);
		award(10, 5, 0, 5, 0x6000242d, 0);
		playSound(kSoundClick);
		break;
	case 0x600025b6: // Sluk
		say(handlerText(getField(kRinser, 0x30) ? 0x60002574 : 0x60002592), 14, true);
		break;

	// The lid machine 124: Se på (6000:27e6)
	case 0x600027e6:
		say(handlerText(getField(kLidMachine, 0x30) ? 0x600027a8 : 0x600027c4), 14, true);
		break;

	// Used / new paper 101 / 104: Benyt with the cardboard machine (6000:042f)
	case 0x6000042f:
		if (second != kCardboardMachine) {
			sayRootMessage(kVerbUse);
		} else if (getField(kCardboardMachine, 0x2f)) {
			say(handlerText(0x6000040f), 14, true);
		} else {
			// 44f3: paper in, the machine shown, 0x1f
			removeObject(object);
			setField(kCardboardMachine, 0x2f, 1);
			setField(kCardboardMachine, 0x30, object == kNewPaper ? 1 : 0);
			setField(kCardboardMachine, kWorking, 1);
			refreshObject(kCardboardMachine);
			factoryMessage(kMsgDone);
		}
		break;

	// PE / PVC 106 / 107: Benyt with the shrink-plastic machine (6000:065c)
	case 0x6000065c:
		if (second != kShrinkMachine) {
			sayRootMessage(kVerbUse);
		} else if (getField(kShrinkMachine, 0x2f)) {
			say(handlerText(0x6000062c), 14, true);
		} else {
			removeObject(object);
			// 3d2f: the plastic in; the jars go on if the trays are there
			setField(kShrinkMachine, 0x30, object == kPePlastic ? 1 : 0);
			setField(kShrinkMachine, 0x2f, 1);
			bool go = getField(kTrayMachine, 0x31) && getField(kTrayMachine, 0x30);
			if (go)
				setField(kTrayMachine, 0x31, 0);
			say(handlerText(0x60000655), 14, true);
			if (go)
				factoryMessage(kMsgDone);
			shrinkMachineRule();
		}
		break;

	// Cardboard trays 110: Benyt with the tray machine (6000:1202 -> 3a4b)
	case 0x60001202:
		if (second != kTrayMachine) {
			sayRootMessage(kVerbUse);
			break;
		}
		removeObject(kTrays);
		say(handlerText(0x600011fb), 14, true);
		setField(kTrayMachine, 0x30, 1);
		if (getField(kTrayMachine, 0x31) && getField(kShrinkMachine, 0x2f)) {
			setField(kTrayMachine, 0x31, 0);
			factoryMessage(kMsgDone);
		}
		break;

	// The tape 111: Benyt with the hole (6000:1383)
	case 0x60001383:
		if (second != kHole) {
			sayRootMessage(kVerbUse);
			break;
		}
		removeObject(kTape);
		setField(kHole, 0x2f, 1);
		refreshObject(kHole);
		award(10, 5, 0, 5, 0x60001310, 0);
		break;

	// The autoclave 127: Tænd (Benyt and Sluk call it, VMT 0x4c)
	case 0x60002b89:
	case 0x60002c02:
	case 0x60002bef:
		if (getField(kAutoclave, 0x32)) {
			say(handlerText(0x60002b62), 14, true);
			break;
		}
		// manager VMT 0x0c(0, 16): the close-up, then back to room 9 node 0
		runAutoclaveScreen();
		redrawRoom();
		break;

	// The hose 128 (6000:3197 / 3320)
	case 0x60003197:
		say(handlerText(getField(kHose, 0x2f) ? 0x6000314b : 0x60003172), 14, true);
		break;
	case 0x60003320:
		if (second == kRadiator) {
			if (getField(kHose, 0x2f)) {
				say(handlerText(0x600031f9), 14, true);
			} else {
				setField(kHose, 0x2f, 1);
				refreshObject(kHose);
				award(0, 0, 0, 0, 0x6000321f, 0);
			}
		} else if (second == kSewer) {
			say(handlerText(getField(kHose, 0x2f) ? 0x600032d5 : 0x600032fd), 14, true);
		} else {
			sayRootMessage(kVerbUse);
		}
		break;

	// The hole 131: Se på (6000:3610)
	case 0x60003610:
		say(handlerText(getField(kHole, 0x2f) ? 0x600035e9 : 0x600035fb), 14, true);
		break;

	// The tray machine 138 / the shrink-plastic machine 139: Se på
	case 0x60003901:
		say(handlerText(getField(kTrayMachine, 0x30) ? 0x600038bb : 0x600038dd), 14, true);
		break;
	case 0x60003ba4:
		if (!getField(kShrinkMachine, 0x2f))
			say(handlerText(0x60003b7e), 14, true);
		else
			say(handlerText(getField(kShrinkMachine, 0x30) ? 0x60003b2f : 0x60003b56), 14, true);
		break;

	// The ceiling light 143: Se på by the switch (6000:1a5d)
	case 0x60001a5d:
		say(handlerText(getField(kLightSwitch, 0x2f) ? 0x60001a3b : 0x60001a4b), 14, true);
		break;

	default:
		return false;
	}
	return true;
}

} // End of namespace Miljostrup
