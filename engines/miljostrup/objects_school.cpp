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

// The school (location 2), written from notes/school.md: classroom 1 (the
// window, radiator, thermometer, notice and the bins with the bat) and its
// two choice close-ups. The waste sorting is in objects_home.cpp (useWaste).

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"

namespace Miljostrup {

enum {
	kSchool = 2,
	kClassroom1 = 6,
	kFirstLamp = 122,       // 122..124 (VMT 5f70), DS:c866..c86e
	kLastLamp = 124,
	kSwitch1 = 125,         // DS:c85e, +0x2f on (start), +0x30 = 126
	kSwitch2 = 126,         // DS:c862, +0x30 = 125
	kLamp2a = 138,          // classroom 2's lamps (VMT 6330)
	kLamp2b = 139,
	kSwitches2 = 142,       // classroom 2's switches, DS:c896, +0x2f on
	kSchoolWindow = 127,    // DS:c87e, +0x2f open
	kSchoolRadiator = 128,  // DS:c87a, +0x2f level (6), +0x31 set, +0x32 award pending
	kFullBin = 130,         // DS:c872, +0x2f stamped
	kSpot1 = 131,           // DS:c882, the bin's empty place in classroom 1
	kBat = 132,
	kNotice = 136,          // DS:c886, +0x2f chosen, +0x30 award pending, choice
	kSpareBin = 140,
	kSpot2 = 141,           // DS:c8a2, in classroom 2, holds the bin at the start
	kBoy1 = 163,
	kBoy2 = 164,

	kBatState = 12,         // the player swinging the bat (images 621, 622)

	// the gym (room 8, unit 81da)
	kGym = 8,
	kBatProxy = 145,        // the bat (ac7f:000b(145, 132, 1)); clickable only with light
	kCurtainCord = 146,     // DS:c8ae, +0x2f curtains open (start closed)
	kFirstCurtain = 147,    // 147..149 "gardiner"
	kLastCurtain = 149,
	kFirstGymSwitch = 150,  // 150..152, DS:c8be..c8c6, +0x2f on (start off)
	kLastGymSwitch = 152,
	kFirstTube = 153,       // 153..161 "røret", each on one switch
	kLastTube = 161,
	kTubes = 162,           // lysstofrør, DS:c8ee: its figure rule sets the light
	kRunner = 163,          // the boy who runs in (DS:c8f2): +0x2f away, +0x30 there
	kDribbler = 164,        // the boy who dribbles (DS:c8f6): +0x2f away, +0x31, +0x30
	kPanel = 165            // thermostat (DS:c8fa): +0x2f degrees, +0x31 award pending, +0x32 set
};

// 83de:066f: the switch of each tube section 153..161
static const int kTubeSwitch[9] = { 150, 151, 152, 152, 150, 151, 151, 152, 150 };

int MiljostrupEngine::gymSwitchesOn() const {
	// 8557:2d17
	int n = 0;
	for (int s = kFirstGymSwitch; s <= kLastGymSwitch; ++s) {
		if (getFlag(s))
			++n;
	}
	return n;
}

bool MiljostrupEngine::hasGymLight() const {
	// 8557:2d64: a switch on or the curtains open
	return gymSwitchesOn() > 0 || getFlag(kCurtainCord);
}

int MiljostrupEngine::getSchoolDimFactor() const {
	// 8557:2bd7 (the tubes' figure rule): dim while the curtains are closed
	// (2472:057e), factor 1 / 6 / 9 / 12 by the switches on (2472:0598); the
	// gym inserts the interval (16, 240, 0, 3, 3) (81da:0839). The other
	// school rooms never dim.
	if (_location != kSchool || _room != kGym || getFlag(kCurtainCord))
		return 0;
	static const int kFactors[4] = { 1, 6, 9, 12 };
	return kFactors[CLIP(gymSwitchesOn(), 0, 3)];
}

void MiljostrupEngine::refreshGymLight() {
	// 8557:2bd7: the tube sections' figures and the palette; the regions
	// that need light follow from isSchoolObjectEnabled
	if (_location != kSchool || _room != kGym)
		return;
	for (int t = kFirstTube; t <= kLastTube; ++t)
		refreshObject(t);
	setRoomPalette(kPaletteA);
}

bool MiljostrupEngine::isSchoolObjectEnabled(int object, bool &enabled) const {
	switch (object) {
	case kBatProxy: // 8557:247e: the proxy's rule, and only with light
		enabled = getFlag(object) != 0 && hasGymLight();
		return true;
	case kCurtainCord:
	case kFirstCurtain:
	case kFirstCurtain + 1:
	case kLastCurtain:
	case kTubes:
	case kPanel: // 8557:2758, 2954, 2bd7, 383c: region on with light
		enabled = hasGymLight();
		return true;
	case kRunner:
	case kDribbler: // 8557:2ea8, 30b5: region on while the boy is there
		enabled = getFlag(object) == 0;
		return true;
	default:
		return false;
	}
}

void MiljostrupEngine::adjustSchoolRegion(int object, Common::Rect &r) const {
	// 8557:2758: the curtain cord's region (screen coordinates) moves down
	// when it is pulled: closed (11, 80)-(16, 92), open (11, 99)-(16, 111)
	if (object == kCurtainCord && _room == kGym) {
		const RoomInfo *room = findRoom(kSchool, kGym);
		int y = getFlag(kCurtainCord) ? 99 : 80;
		r = Common::Rect(11, y, 16, y + 12);
		if (room)
			r.translate(-room->x, -room->y);
	}
}

void MiljostrupEngine::usePanel() {
	// 8557:3c8b: once; +0x31 := 1, +0x32 := 1, GoTo(14, 0); the dispatcher
	// (7fa1) runs 8000:07bc(panel +0x2f, back to 8, value, max 25, min 10)
	if (getField(kPanel, 0x32)) {
		say(handlerText(0x85573c6b), 14, true);
		return;
	}
	setField(kPanel, 0x31, 1);
	setField(kPanel, 0x32, 1);
	setFlag(kPanel, runTemperatureScreen(getFlag(kPanel), 10, 25));
	redrawRoom();
}

void MiljostrupEngine::gymBuilt() {
	if (_location != kSchool || _room != kGym)
		return;
	refreshGymLight();
	// The panel's figure rule (8557:383c): +0x31 -> 8557:3a00, the award
	if (getField(kPanel, 0x31)) {
		setField(kPanel, 0x31, 0);
		int t = getFlag(kPanel);
		int points;
		if (t <= 10)
			points = 1;
		else if (t == 11)
			points = 4;
		else if (t == 12)
			points = 7;
		else if (t <= 17)
			points = 10;
		else if (t == 18)
			points = 6;
		else if (t == 19)
			points = 2;
		else if (t == 20)
			points = -2;
		else if (t == 21)
			points = -6;
		else
			points = -10;
		Common::String n = Common::String::format("%d", t);
		Common::String text = handlerText(0x8557387b);
		if (t < 13)
			text += n + handlerText(0x85573925);
		else if (t < 18)
			text += handlerText(0x85573943) + n + handlerText(0x8557396e);
		else if (t < 21)
			text += handlerText(0x85573943) + n + handlerText(0x85573960);
		else
			text += handlerText(0x85573943) + n + handlerText(0x8557394e);
		awardText(points, points, 0, 0, text, handlerText(0x855739fa));
		if (points < 0) {
			// correct 0 ("Tager trøje på"), category 1
			setQuiz(0x85573976, 0x855739d6, 0x855739eb, 0, 1);
		}
	}
	// The boys' figure rules (8557:2ea8, 30b5)
	if (!getFlag(kDribbler) && !getField(kDribbler, 0x31) && !_gymStage) {
		// he has just come (a switch went on): input off, he runs in from
		// the left, Walk(1, 65, 0, 13), message 0x1f
		setInputOff(true);
		_scene->setFigurePos(kDribbler, -30, 75);
		_scene->setFigureVisible(kDribbler, true);
		_scene->stopFigure(kDribbler);
		_scene->queueFigureWalk(kDribbler, 1, 65, 0, 13);
		_gymStage = 1;
	} else if (!getFlag(kDribbler) && getField(kDribbler, 0x30)) {
		_scene->setFigurePos(kDribbler, 265, 75);
		_scene->setFigureState(kDribbler, 2);
	}
	if (!getFlag(kRunner) && getField(kRunner, 0x30)) {
		_scene->setFigurePos(kRunner, 170, 76);
		_scene->setFigureState(kRunner, 2);
	}
	// The room's run start (81da:00ec): in the dark
	if (!hasGymLight())
		say(handlerText(0x81da00d4), 14, true);
}

void MiljostrupEngine::tickGym() {
	if (_location != kSchool || !_gymStage)
		return;
	if (_gymStage == 1) {
		if (_scene->isFigureBusy(kDribbler))
			return;
		// message 0x1f (81da:0133): the dribbler (8557:3224: +0x31 := 1)
		// dribbles across, Walk(1, 230, 0, 46), message 0x20; the runner
		// (8557:2f8d: +0x30 := was there, +0x2f := 0) runs in, Walk(1,
		// 200, 0, 50), message 0x21
		setField(kDribbler, 0x31, 1);
		_scene->queueFigureWalk(kDribbler, 1, 230, 0, 46);
		setField(kRunner, 0x30, getFlag(kRunner) == 0 ? 1 : 0);
		setFlag(kRunner, 0);
		if (!getField(kRunner, 0x30)) {
			_scene->setFigureVisible(kRunner, true);
			_scene->queueFigureWalk(kRunner, 1, 200, 0, 50);
		}
		_gymStage = 2;
		return;
	}
	// message 0x20: the dribbler stands at (265, 75) (state 2)
	if (!getField(kDribbler, 0x30) && !_scene->isFigureBusy(kDribbler)) {
		setField(kDribbler, 0x30, 1);
		_scene->setFigurePos(kDribbler, 265, 75);
		_scene->setFigureState(kDribbler, 2);
	}
	// message 0x21: the runner stands at (170, 76), input on again
	if (!getField(kRunner, 0x30) && !_scene->isFigureBusy(kRunner)) {
		setField(kRunner, 0x30, 1);
		_scene->setFigurePos(kRunner, 170, 76);
		_scene->setFigureState(kRunner, 2);
		setInputOff(false);
	}
	if (getField(kDribbler, 0x30) && getField(kRunner, 0x30))
		_gymStage = 0;
}

void MiljostrupEngine::updateSpotName(int spot) {
	// 8557:2286 (GetName): an empty spot is "tom plads", one with the bin has
	// the bin's name
	if (getFlag(spot))
		_interface->setNameOverride(spot, _interface->getObjectName(kSpareBin));
	else
		_interface->setNameOverride(spot, handlerText(0x85572202));
}

void MiljostrupEngine::initSchool() {
	updateSpotName(kSpot1);
	updateSpotName(kSpot2);
	_batStage = 0;
	_gymStage = 0;
}

static int radiatorLevelToChoice(int level) {
	// 8093:00fa: the button pressed at the start
	switch (level) {
	case 1:
		return 1;
	case 2:
		return 2;
	case 4:
		return 3;
	default:
		return 4;
	}
}

void MiljostrupEngine::useSchoolRadiator() {
	// 8557:0dcf: set once (+0x31) -> "Den har du allerede indstillet!"; else
	// GoTo(12, 0): the choice screen; 8093:0073 gives level 0 / 2 / 4 / 6 to
	// the radiator's 8557:0d94 (+0x2f, +0x31 := 1, +0x32 := 1) and GoTo(6, 0)
	if (getField(kSchoolRadiator, 0x31)) {
		say(handlerText(0x85570daf), 14, true);
		return;
	}
	const RoomInfo *room = findRoom(kSchool, 12);
	int choice = runChoiceScreen(room ? room->image : 629, room ? room->x : 0, room ? room->y : 23, 630,
		handlerText(0x809300d8), true, radiatorLevelToChoice(getFlag(kSchoolRadiator)));
	if (choice > 0) {
		static const int kLevels[4] = { 0, 2, 4, 6 };
		setFlag(kSchoolRadiator, kLevels[choice - 1]);
		setField(kSchoolRadiator, 0x31, 1);
		setField(kSchoolRadiator, 0x32, 1);
	}
	redrawRoom();
}

void MiljostrupEngine::useNotice() {
	// 8557:16a6: GoTo(13, 0), the choice screen without OK (a click chooses);
	// 8082:0073 -> 8557:194d(choice): +0x2f := 1, +0x30 := 1, the award's
	// text and points (8557:16d6)
	const RoomInfo *room = findRoom(kSchool, 13);
	int choice = runChoiceScreen(room ? room->image : 634, room ? room->x : 0, room ? room->y : 23, 635,
		handlerText(0x808200a2), false, 0);
	if (choice > 0) {
		setFlag(kNotice, 1);
		setField(kNotice, 0x30, 1);
		setField(kNotice, 0x31, choice);
	}
	redrawRoom();
}

void MiljostrupEngine::schoolBuilt() {
	if (_location == kSchool && _room == kGym)
		gymBuilt();
	if (_location != kSchool || _room != kClassroom1)
		return;
	// The radiator's figure rule (8557:0d73): +0x32 -> 8557:0f8e, the award
	if (getField(kSchoolRadiator, 0x32)) {
		setField(kSchoolRadiator, 0x32, 0);
		if (getFlag(kSchoolRadiator) == 0)
			award(10, 10, 0, 0, 0x85570e35, 0x85570f88);
		else if (getFlag(kSchoolWindow))
			award(-10, -10, 0, 0, 0x85570e86, 0x85570f88);
		else
			award(0, 0, 0, 0, 0x85570ede, 0x85570f88);
		// b06f:1185: correct 0 ("Klassen"), category 1 energy
		setQuiz(0x85570f56, 0x85570f71, 0x85570f80, 0, 1);
		return;
	}
	// The notice's figure rule (8557:1a30): +0x30 -> Award(points, points,
	// 0, 0, text), page "16"; a negative one with the quiz
	if (getField(kNotice, 0x30)) {
		setField(kNotice, 0x30, 0);
		static const uint32 kTexts[4] = { 0x8557172b, 0x855717c9, 0x85571844, 0x855718da };
		static const int kPoints[4] = { 5, 10, -10, 0 };
		int choice = CLIP(getField(kNotice, 0x31), 1, 4) - 1;
		award(kPoints[choice], kPoints[choice], 0, 0, kTexts[choice], 0x855719fc);
		if (kPoints[choice] < 0) {
			// correct 1 ("Dieseltog"), category 1
			setQuiz(0x855719ff, 0x85571a18, 0x85571a22, 1, 1);
		}
	}
}

void MiljostrupEngine::tickSchool() {
	tickGym();
	if (_location != kSchool || !_batStage || _scene->isWalking())
		return;
	if (_batStage == 1) {
		// message 0x20 (82d8:00b7): the bin is stamped (8557:0669), and the
		// player swings on for 29 steps; then message 0x21
		setFlag(kFullBin, 1);
		refreshObject(kFullBin);
		_scene->playPlayer(kBatState, 29);
		_batStage = 2;
		return;
	}
	// message 0x21 (82d8:01ab): Award(10, 10, 0, 0), no page
	_batStage = 0;
	setInputOff(false);
	award(10, 10, 0, 0, 0x82d80121, 0);
}

int MiljostrupEngine::getSchoolFigureState(uint32 rule, int object, bool &visible) const {
	switch (rule) {
	case 0x85570627: // bin 130: overfull 1, stamped 2
		return getFlag(object) ? 2 : 1;
	case 0x85571510: // window 127: open 1, closed 2
		return getFlag(object) ? 1 : 2;
	case 0x855723f6: // spots 131 / 141: the bin is shown while it stands there
		visible = getFlag(object) != 0;
		return 1;
	case 0x855704bd: // lamps 122..124: lit (image 618) while both switches are on (8557:0501)
		visible = getFlag(kSwitch1) && getFlag(kSwitch2);
		return 1;
	case 0x85571f42: // classroom 2's lamps 138 / 139: lit while the switches 142 are on
		visible = getFlag(kSwitches2) != 0;
		return 1;
	case 0x85572758: // the curtain cord: closed 1, pulled 2
		return getFlag(object) ? 2 : 1;
	case 0x85572954: // curtains 147..149: open 1, closed 2
		return getFlag(kCurtainCord) ? 1 : 2;
	case 0x855736bb: // tube sections: lit while their switch is on
		if (object >= kFirstTube && object <= kLastTube)
			visible = getFlag(kTubeSwitch[object - kFirstTube]) != 0;
		return 1;
	case 0x85572ea8: // the runner: away (hidden), running 1, there 2
		visible = getFlag(object) == 0;
		return getField(object, 0x30) ? 2 : 1;
	case 0x855730b5: // the dribbler: away (hidden, 2), dribbling 1, there 2
		visible = getFlag(object) == 0;
		if (getFlag(object))
			return 2;
		return getField(object, 0x31) && getField(object, 0x30) ? 2 : 1;
	default:
		return 0;
	}
}

bool MiljostrupEngine::runSchoolHandler(uint32 handler, int verb, int object, int second) {
	switch (handler) {
	// The dribbler 164: Tal til, his own words (colour 2, the player does
	// not talk)
	case 0x85573084:
		say(handlerText(0x8557306d), 2, false);
		break;
	// Light switches 125 / 126 (VMT 5f10, +0x2f on, +0x30 the other switch)
	case 0x855700ce: // Se på
		say(handlerText(getFlag(object) ? 0x855700b0 : 0x855700be), 14, true);
		break;
	case 0x8557015f: // Tænd: never on again
		say(handlerText(getFlag(object) ? 0x85570130 : 0x85570147), 14, true);
		break;
	case 0x8557024b: { // Sluk: only one of the two
		int other = object == kSwitch1 ? kSwitch2 : kSwitch1;
		if (!getFlag(object)) {
			say(handlerText(0x85570232), 14, true);
		} else if (!getFlag(other)) {
			say(handlerText(0x8557021a), 14, true);
		} else {
			// off, Award(10, 10, 0, 0), page "14", command 0x40, the three
			// lamps' figures (VMT 0x58), sound 1d26:0155
			setFlag(object, 0);
			award(10, 10, 0, 0, 0x855701c1, 0x85570217);
			for (int lamp = kFirstLamp; lamp <= kLastLamp; ++lamp)
				refreshObject(lamp);
			playSound(kSoundClick);
		}
		break;
	}
	// Lamps 122..124 (VMT 5f70)
	case 0x85570403: // Se på
		say(handlerText(getFlag(kSwitch1) && getFlag(kSwitch2) ? 0x855703c6 : 0x855703ec), 14, true);
		break;
	case 0x85570479: // Tænd
	case 0x855704aa: // Sluk calls Tænd (VMT 0x4c)
		say(handlerText(0x85570468), 14, true);
		break;

	// Classroom 2: switches 142 (VMT 6270.., +0x2f on) and lamps 138 / 139
	case 0x85571b76: // Se på
		say(handlerText(getFlag(object) ? 0x85571b59 : 0x85571b66), 14, true);
		break;
	case 0x85571c0f: // Tænd: never on again
		say(handlerText(getFlag(object) ? 0x85571bd8 : 0x85571bee), 14, true);
		break;
	case 0x85571ce8: // Sluk
		if (!getFlag(object)) {
			say(handlerText(0x85571ccf), 14, true);
			break;
		}
		// off, the lamps' figures, Award(10, 10, 0, 0), page "14", command
		// 0x40, sound 1d26:0155
		setFlag(object, 0);
		refreshObject(kLamp2a);
		refreshObject(kLamp2b);
		award(10, 10, 0, 0, 0x85571c71, 0x85571ccc);
		playSound(kSoundClick);
		break;
	case 0x85571e50: // lamps Se på (by the switches, DS:c896)
		say(handlerText(getFlag(kSwitches2) ? 0x85571e1b : 0x85571e3b), 14, true);
		break;

	// The gym: curtain cord 146 and curtains 147..149
	case 0x85572570: // cord Se på
		say(handlerText(getFlag(kCurtainCord) ? 0x8557253a : 0x85572555), 14, true);
		break;
	case 0x85572677: // cord Træk: open the curtains
		if (getFlag(kCurtainCord)) {
			say(handlerText(0x855725d2), 14, true);
			break;
		}
		// +0x2f := 1, figures (the cord, the curtains, the light), Award(10,
		// 10, 0, 0), page "14", command 0x40, sound 1d26:0132
		setFlag(kCurtainCord, 1);
		refreshObject(kCurtainCord);
		for (int c = kFirstCurtain; c <= kLastCurtain; ++c)
			refreshObject(c);
		refreshGymLight();
		award(10, 10, 0, 0, 0x855725f6, 0x85572674);
		playSound(kSoundClose);
		break;
	case 0x855728a1: // curtains Se på
		say(handlerText(getFlag(kCurtainCord) ? 0x8557287b : 0x8557288e), 14, true);
		break;

	// Tubes 162 (the light) and the switches 150..152
	case 0x85572a8d: { // Se på: by the switches on
		static const uint32 kTexts[4] = { 0x85572a26, 0x85572a3f, 0x85572a5a, 0x85572a77 };
		say(handlerText(kTexts[CLIP(gymSwitchesOn(), 0, 3)]), 14, true);
		break;
	}
	case 0x855732ea: // switch Se på
		say(handlerText(getFlag(object) ? 0x855732cc : 0x855732da), 14, true);
		break;
	case 0x855733bb: // switch Tænd
		if (getFlag(object)) {
			say(handlerText(0x8557334c), 14, true);
			break;
		}
		// on, the light; the dribbler comes (8557:325a: +0x2f := 0, he
		// runs in when the room is built again); Award(-5, -5, 0, 0), page
		// "14", command 0x40, sound 1d26:0155
		setFlag(object, 1);
		refreshGymLight();
		setFlag(kDribbler, 0);
		award(-5, -5, 0, 0, 0x85573363, 0x855733b8);
		playSound(kSoundClick);
		break;
	case 0x85573524: // switch Sluk
		if (!getFlag(object)) {
			say(handlerText(0x8557350b), 14, true);
		} else if (gymSwitchesOn() < 2 && !getFlag(kCurtainCord)) {
			say(handlerText(0x855734e2), 14, true);
		} else {
			setFlag(object, 0);
			refreshGymLight();
			award(5, 5, 0, 0, 0x8557347f, 0x855734df);
			playSound(kSoundClick);
		}
		break;

	// Panel 165: the thermostat
	case 0x855737a8: // Se på: "Termostaten er sat til " + t + "°C."
		say(handlerText(0x8557378c) + Common::String::format("%d", getFlag(kPanel)) + handlerText(0x855737a4), 14, true);
		break;
	case 0x8557380c: // Benyt
	case 0x8557381c: // Tænd
	case 0x8557382c: // Sluk
		usePanel();
		break;

	// Bin 130 and the bat 132
	case 0x855705c5: // Se på
		say(handlerText(getFlag(kFullBin) ? 0x8557059e : 0x855705b3), 14, true);
		break;
	case 0x85570790: // bat Benyt med the bin (0x82) or a boy (0xa3, 0xa4)
		if (second == kFullBin) {
			if (getFlag(kFullBin)) {
				say(handlerText(0x8557073b), 14, true);
			} else {
				// message 0x1f (82d8:001c): the player stops, input off, he
				// swings the bat (state 12) for 4 steps; then message 0x20
				_scene->stopWalking();
				setInputOff(true);
				_scene->playPlayer(kBatState, 4);
				_batStage = 1;
			}
		} else if (second == kBoy1) {
			say(handlerText(0x8557075f), 14, true);
		} else if (second == kBoy2) {
			say(handlerText(0x85570780), 14, true);
		} else {
			sayRootMessage(kVerbUse);
		}
		break;

	// Thermometer 129 (8557:0928: 19 degrees with the radiator off and the window open, else 25)
	case 0x855708c2: {
		int t = getFlag(kSchoolRadiator) == 0 && getFlag(kSchoolWindow) ? 19 : 25;
		say(handlerText(0x855708b3) + Common::String::format("%d", t) + handlerText(0x855708be), 14, true);
		break;
	}

	// Radiator 128
	case 0x85570a4d: { // Se på
		int level = getFlag(kSchoolRadiator);
		Common::String l = Common::String::format("%d", level);
		if (!getFlag(kSchoolWindow)) {
			if (level < 3)
				say(handlerText(0x85570a13) + l + handlerText(0x85570a28), 14, true);
			else
				say(handlerText(0x85570a2a) + l + handlerText(0x85570a4b), 14, true);
		} else {
			say(handlerText(level == 0 ? 0x855709d0 : 0x855709ef), 14, true);
		}
		break;
	}
	case 0x85570b92: // Benyt
	case 0x85570bfa: // Tænd
	case 0x85570c60: // Sluk
	case 0x85570cc8: // Åbn
	case 0x85570d2d: { // Luk: at level 0 a text, else the close-up
		if (getFlag(kSchoolRadiator) == 0) {
			static const struct { uint32 handler, text; } kOff[] = {
				{ 0x85570b92, 0x85570b70 }, { 0x85570bfa, 0x85570bd8 }, { 0x85570c60, 0x85570c40 },
				{ 0x85570cc8, 0x85570ca6 }, { 0x85570d2d, 0x85570d0e }
			};
			for (uint i = 0; i < ARRAYSIZE(kOff); ++i) {
				if (kOff[i].handler == handler)
					say(handlerText(kOff[i].text), 14, true);
			}
		} else {
			useSchoolRadiator();
		}
		break;
	}

	// Window 127 (it never closes again)
	case 0x85571140: // Se på
		say(handlerText(getFlag(kSchoolWindow) ? 0x85571123 : 0x85571131), 14, true);
		break;
	case 0x85571302: // Åbn
		if (getFlag(kSchoolWindow)) {
			say(handlerText(0x855711a2), 14, true);
			break;
		}
		setFlag(kSchoolWindow, 1);
		refreshObject(kSchoolWindow);
		playSound(kSoundOpen);
		if (getFlag(kSchoolRadiator) == 0) {
			award(10, 10, 0, 0, 0x85571286, 0x855712fc);
		} else {
			award(-10, -10, 0, 0, 0x855711b9, 0x855712fc);
			// correct 1 ("20°C"), category 1
			setQuiz(0x8557124e, 0x8557127c, 0x85571281, 1, 1);
		}
		break;
	case 0x85571479: // Luk
		if (!getFlag(kSchoolWindow))
			say(handlerText(0x85571461), 14, true);
		else
			say(handlerText(getFlag(kSchoolRadiator) == 0 ? 0x8557142b : 0x85571448), 14, true);
		break;

	// Notice 136: the trip to Paris
	case 0x855715f9: // Se på
	case 0x85571660: // Benyt
		if (!getFlag(kNotice))
			useNotice();
		else
			say(handlerText(handler == 0x855715f9 ? 0x855715d3 : 0x8557163f), 14, true);
		break;

	// The spots 131 / 141 and the spare bin 140
	case 0x85572356: // Se på
		say(handlerText(getFlag(object) ? 0x85572315 : 0x85572331), 14, true);
		break;
	case 0x855723b8: // Tag: the bin (room VMT 0x50 with 0x8c)
		if (!getFlag(object)) {
			sayRootMessage(kVerbTake);
			break;
		}
		setFlag(object, 0);
		refreshObject(object);
		updateSpotName(object);
		takeObject(kSpareBin);
		break;
	case 0x8557210f: // bin 140 Benyt med a spot (0x83, 0x8d)
		if (second != kSpot1 && second != kSpot2) {
			sayRootMessage(kVerbUse);
			break;
		}
		removeObject(kSpareBin);
		setFlag(second, 1); // 8557:2438
		refreshObject(second);
		updateSpotName(second);
		if (second == kSpot1 && !getFlag(kFullBin))
			award(0, 0, 0, 0, 0x8557203b, 0);
		break;

	default:
		return false;
	}
	return true;
}

} // End of namespace Miljostrup
