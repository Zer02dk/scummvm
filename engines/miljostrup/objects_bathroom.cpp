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

// The bathroom (home, room 9), written from notes/bathroom.md: its objects,
// the little brother's visit and the washing machine with its close-ups
// (closeup.cpp).

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/handlers_data.h"

namespace Miljostrup {

enum {
	kBathroomDoor = 130,
	kChain = 131,          // WC-snor, DS:cb26
	kWashingMachine = 132, // DS:cac6
	kHotTap = 141,         // DS:cb4e
	kColdTap = 142,        // DS:cb52
	kSinkTap = 145,        // DS:cb56, drips
	kPaperHolder = 149,
	kKitchenRoll = 150,
	kGasket = 153,         // pakning (shop)
	kLittleBrother = 154,  // DS:cb82
	kShower = 209,         // DS:cc56
	kSaverShower = 210,    // sparebruser (shop)
	kToilet = 219,         // DS:cc66
	kToiletPaper = 208,    // of recycled paper
	kPlainToiletPaper = 284,
	kLaundry1 = 143, kLaundry2 = 144, kTowels = 187,
	kLaundryProxy = 211,   // DS:cc5a, -> 144, hidden at the start
	kPowder = 250,         // vaskepulver, DS:c9d6: +0x2f..+0x34 the towns' water hardness
	kSchedule = 251,
	kEmpty = 100,
	kBathroomRoom = 9,

	kFlushTicks = 50       // a000:3858: +0x30 := 50 idle ticks
};

// The bath close-up's choice (+0x32): 1 the tub, 2..6 showers; anything else
// counts as the tub too (a000:6890: < 2 or > 6)
static bool isBathTub(int choice) {
	return choice < 2 || choice > 6;
}

void MiljostrupEngine::initWashingPowder() {
	// 9909:0b84: Randomize, then +0x2f..+0x34 := Random(3) each: the water
	// hardness of the six towns on the box (0 hard, 1 medium, 2 soft); the
	// fourth (+0x32) is Miljøstrup's
	for (int k = 0; k < 6; ++k)
		setField(kPowder, 0x2f + k, _randomSource.getRandomNumber(2));
}

void MiljostrupEngine::startWashProgram() {
	// GoTo(30, 0): close-up 30; 8a01:088c stores the temperature (+0x36) and
	// the program (+0x35) and sets +0x31 unless cancelled; back in the
	// bathroom the machine's figure rule (9909:0a5b) starts the wash
	int program, temperature;
	bool started = runWashProgram(program, temperature);
	setField(kWashingMachine, 0x36, temperature);
	setField(kWashingMachine, 0x35, program);
	setField(kWashingMachine, 0x31, started ? 1 : 0);
	redrawRoom();
}

static Common::String joinAnd(const Common::String &a, const Common::String &b) {
	// 9909:0091: a + " og " + b, or the one that is not empty
	if (a.empty())
		return b;
	if (b.empty())
		return a;
	return a + " og " + b;
}

void MiljostrupEngine::washingAward() {
	// 9909:0544 (VMT 0x68): +0x2f := 0 (it washes), then VMT 0x60 = 9909:0184
	const int m = kWashingMachine;
	setFlag(m, 0);
	refreshObject(m);

	// One record per aspect (kWashingRecords, in handlers.WASHING_RECORDS order)
	enum { kLoadFull, kLoadNotFull, kDoseRight, kDoseLittle, kDoseMuch, kTempOk, kTempHigh, kPrewash, kNoPrewash };
	// The dose the chart asks for: the powder's VMT 0x60 (9909:0c72) by the
	// program and Miljøstrup's water hardness (+0x32: 0 hard, 1 medium, 2 soft)
	static const int kNeed[2][3] = { { 180, 120, 100 }, { 220, 170, 150 } };
	int program = getField(m, 0x35);
	int need = kNeed[program == 0 ? 0 : 1][CLIP(getField(kPowder, 0x32), 0, 2)];
	int dose = getField(m, 0x33);
	const WashingRecord *rec[4] = {
		&kWashingRecords[program == 0 ? kNoPrewash : kPrewash],
		&kWashingRecords[getField(m, 0x36) > 40 ? kTempHigh : kTempOk],
		&kWashingRecords[dose < need ? kDoseLittle : dose > need ? kDoseMuch : kDoseRight],
		&kWashingRecords[getField(m, 0x32) == 3 ? kLoadFull : kLoadNotFull]
	};
	Common::String good, bad;
	int points = 0, energy = 0, water = 0;
	for (int k = 0; k < 4; ++k) {
		good = joinAnd(good, readPascalString(_exe, rec[k]->good));
		bad = joinAnd(bad, readPascalString(_exe, rec[k]->bad));
		points += rec[k]->points;
		energy += rec[k]->energy;
		water += rec[k]->water;
	}
	// "Godt, at du har <good>. \r" + "Derimod har du <bad>.", or "Du har
	// <bad>.\r" + "Det var ikke så godt!"
	Common::String text;
	if (!good.empty()) {
		text = handlerText(0x9909010c) + good + handlerText(0x9909011d);
		if (!bad.empty())
			text += handlerText(0x99090121) + bad + handlerText(0x99090131);
	} else {
		text = handlerText(0x99090133) + bad + handlerText(0x9909013b) + handlerText(0x9909013e);
	}
	// ac76:0051 = Max: no value below -10; no waste points, no page
	awardText(MAX(points, -10), MAX(energy, -10), 0, MAX(water, -10), text, Common::String());
	// b06f:1185: correct = 1 ("Ja", the first answer), category 3 = water
	setQuiz(0x99090154, 0x9909017d, 0x99090180, 1, 3);
}

void MiljostrupEngine::bathroomBuilt() {
	if (_location != 1 || _room != kBathroomRoom)
		return;
	// The washing machine's figure rule (9909:0a5b): +0x31 (set by close-up
	// 30) -> clear it, VMT 0x68: the wash starts, the award
	if (getField(kWashingMachine, 0x31)) {
		setField(kWashingMachine, 0x31, 0);
		washingAward();
	}
	// The binding calls the figure rule (22a9:03af -> VMT 0x58); the run then
	// starts with VMT 0x38 (90f3:0ef4): in his bath (3) he gets out next time
	_brotherWalking = false;
	littleBrotherFigure();
	if (getField(kLittleBrother, 0x30) == 3)
		littleBrotherNext();
}

void MiljostrupEngine::littleBrotherNext() {
	// a000:682e: +0x2f := 1, +0x31 := state + 1 (states 0..5)
	const int b = kLittleBrother;
	setFlag(b, 1);
	if (getField(b, 0x30) <= 5)
		setField(b, 0x31, getField(b, 0x30) + 1);
}

bool MiljostrupEngine::isLittleBrotherRegionOn() const {
	// The region (+8, VMT 0x24) is switched on in states 2 and 5 and off in
	// 0, 1, 4 and 6; in state 3 the room is new, so it is on (widgets start
	// enabled) at its place from the room data
	int state = getField(kLittleBrother, 0x30);
	return state == 2 || state == 3 || state == 5;
}

void MiljostrupEngine::littleBrotherFigure() {
	// a000:6890 (VMT 0x58). +0x2f set: on to the state in +0x31 first.
	// Positions are room coordinates of the figure (VMT 0x40); the region
	// is moved in adjustRegion (a000:67d8, screen coordinates)
	const int b = kLittleBrother;
	bool advancing = getFlag(b) != 0;
	if (advancing)
		setField(b, 0x30, getField(b, 0x31));
	bool tub = isBathTub(getField(b, 0x32));

	switch (getField(b, 0x30)) {
	case 0:
		// Not there; he comes once the sink tap is fixed (DS:cb56 +0x2f = 0)
		// and the shower has the water saver (DS:cc56 +0x2f)
		_scene->setFigureVisible(b, false);
		if (!getFlag(kSinkTap) && getFlag(kShower)) {
			littleBrotherNext();
			littleBrotherFigure();
		}
		break;
	case 1:
		// He walks in from the door: input off, message 0x1f at the end
		if (advancing) {
			setInputOff(true);
			_scene->setFigurePos(b, 20, 77);
			_scene->setFigureVisible(b, true);
			_scene->queueFigureWalk(b, 2, 140, 0, 35);
			_scene->queueFigureWalk(b, 3, 0, 0, 1);
			_brotherWalking = true;
		}
		break;
	case 2:
		// He waits for his bath (Se på, Hjælp)
		setInputOff(false);
		_scene->setFigurePos(b, 160, 77);
		_scene->setFigureVisible(b, true);
		_scene->setFigureState(b, 3);
		break;
	case 3:
		// In the tub (state 5) or under the shower (state 4)
		_scene->setFigureVisible(b, true);
		if (tub) {
			_scene->setFigurePos(b, 125, 82);
			_scene->setFigureState(b, 5);
		} else {
			_scene->setFigurePos(b, 276, 34);
			_scene->setFigureState(b, 4);
		}
		break;
	case 4:
		// He walks to the sink: input off, message 0x1f at the end
		if (advancing) {
			setInputOff(true);
			_scene->setFigureVisible(b, true);
			if (tub) {
				_scene->setFigurePos(b, 160, 77);
				_scene->queueFigureWalk(b, 6, -85, 0, 21);
			} else {
				_scene->setFigurePos(b, 290, 77);
				_scene->queueFigureWalk(b, 6, -215, 0, 54);
			}
			_scene->queueFigureWalk(b, 7, 0, 0, 1);
			_brotherWalking = true;
		}
		break;
	case 5:
		// He brushes his teeth; arriving, he opens the cold tap (DS:cb52
		// VMT 0x60 = a000:4c8d)
		if (advancing) {
			setFlag(kColdTap, 0);
			refreshObject(kColdTap);
		}
		setInputOff(false);
		_scene->setFigurePos(b, 75, 77);
		_scene->setFigureVisible(b, true);
		_scene->setFigureState(b, 8);
		break;
	default:
		// 6: gone (the player left the bathroom while he brushed his teeth)
		_scene->setFigureVisible(b, false);
		break;
	}

	setFlag(b, 0);
	if (!getField(b, 0x2c))
		return;
	// +0x2c (set by close-up 26): VMT 0x60 = a000:654d, the award for the
	// choice, with a quiz (correct = 1: the first answer, category 3 water)
	// and no page; command 0x40 lets the player talk first
	setField(b, 0x2c, 0);
	static const struct {
		int values[4];
		uint32 text;
	} kBathAwards[6] = {
		{ { -10, -5, 0, -5 }, 0xa00060e0 }, // the tub
		{ {  10,  5, 0,  5 }, 0xa0006159 }, // shower, at most 4 minutes
		{ {   6,  3, 0,  3 }, 0xa000621f }, // 4-6
		{ {   0,  0, 0,  0 }, 0xa00062ce }, // 6-8
		{ {  -6, -3, 0, -3 }, 0xa0006393 }, // 8-10
		{ { -10, -5, 0, -5 }, 0xa000644d }  // 10-12
	};
	int choice = getField(b, 0x32);
	if (choice < 1 || choice > 6)
		return;
	const int *v = kBathAwards[choice - 1].values;
	award(v[0], v[1], v[2], v[3], kBathAwards[choice - 1].text, 0);
	setQuiz(0xa000650b, 0xa0006539, 0xa0006543, 1, 3);
}

void MiljostrupEngine::tickBathroom() {
	if (_location != 1)
		return;
	// The little brother's walk has ended: message 0x1f (90f3:0f3e) -> his
	// VMT 0x68 and the figure rule
	if (_brotherWalking && !_scene->isFigureBusy(kLittleBrother)) {
		_brotherWalking = false;
		littleBrotherNext();
		littleBrotherFigure();
	}
	// 90f3:0f14: while the chain is pulled, its VMT 0x60 (a000:39bb) counts down
	if (getFlag(kChain) || !getField(kChain, 0x30))
		return;
	setField(kChain, 0x30, getField(kChain, 0x30) - 1);
	if (getField(kChain, 0x30))
		return;
	setFlag(kChain, 1);
	refreshObject(kChain);
	setInputOff(false);
	award(-10, 0, 0, -10, 0xa00038c1, 0xa00039b8);
	// b06f:1185: correct = 0 (the second answer), category 3 = water
	setQuiz(0xa0003969, 0xa00039a6, 0xa00039af, 0, 3);
}

bool MiljostrupEngine::canLeaveBathroom(int door) {
	// a000:bc98: the bathroom door's go-through (VMT 0x60) asks the cold tap
	// (VMT 0x64 = a000:4fb5) first when the door is open
	if (_location != 1 || door != kBathroomDoor || !_doors.contains(door) || !_doors[door])
		return true;
	if (getFlag(kColdTap)) {
		// closed: brushing his teeth (5), the little brother leaves too (VMT 0x68)
		if (getField(kLittleBrother, 0x30) == 5)
			littleBrotherNext();
		return true;
	}
	if (!getField(kColdTap, 0x30)) {
		setField(kColdTap, 0x30, 1);
		award(-5, 0, 0, -5, 0xa0004ec8, 0xa0004f95);
		setQuiz(0xa0004f4d, 0xa0004f81, 0xa0004f8b, 0, 3);
	} else {
		say(handlerText(0xa0004f98), 14, true);
	}
	return false;
}

int MiljostrupEngine::getBathroomFigureState(uint32 rule, int object, bool &visible) const {
	switch (rule) {
	case 0xa0003ab1: // toilet chain: ready 1, pulled 2
		return getFlag(object) ? 1 : 2;
	case 0xa00050b2: // hot/cold tap: closed 1, running 2
		return getFlag(object) ? 1 : 2;
	case 0xa0005468: // sink tap: dripping 2, fixed 1
		return getFlag(object) ? 2 : 1;
	case 0xa0005d24: // paper holder: empty 1, kitchen roll 2, toilet paper 3, newspaper 4
		switch (getFlag(object)) {
		case kKitchenRoll:
			return 2;
		case kToiletPaper:
		case kPlainToiletPaper:
			return 3;
		case 152:
			return 4;
		default:
			return 1;
		}
	case 0xa000bc56: // shower: water saver 2, old 1
		return getFlag(object) ? 2 : 1;
	case 0xa0006890: { // little brother: the state the figure rule gives (littleBrotherFigure)
		int state = getField(object, 0x30);
		visible = state >= 1 && state <= 5;
		switch (state) {
		case 1:
			return 2;
		case 2:
			return 3;
		case 3:
			return isBathTub(getField(object, 0x32)) ? 5 : 4;
		case 4:
			return 6;
		case 5:
			return 8;
		default:
			return 1;
		}
	}
	case 0x99090a5b: { // washing machine: closed 1..4 / open 5..8 by its load, washing 9
		if (!getFlag(object))
			return 9;
		int load = CLIP(getField(object, 0x32), 0, 3);
		return (getField(object, 0x30) ? 1 : 5) + load;
	}
	default:
		return 0;
	}
}

bool MiljostrupEngine::runBathroomHandler(uint32 handler, int verb, int object, int second) {
	Common::String pronoun;
	if (const ObjectInfo *info = findObject(_location, object))
		pronoun = capitalize(readPascalString(_ovr, info->pronoun));

	switch (handler) {
	// Toilet chain 131 (+0x2f ready, +0x30 countdown) and toilet 219 (+0x2f paper in it)
	case 0xa0003785: // Se på
		say(handlerText(getFlag(kChain) ? 0xa000375c : 0xa000376f), 14, true);
		break;
	case 0xa0003819: // Træk
		if (!getFlag(kChain)) {
			say(handlerText(0xa00037fa), 14, true);
		} else if (!getFlag(kToilet)) {
			say(handlerText(0xa00037e7), 14, true);
		} else {
			setFlag(kChain, 0);
			setFlag(kToilet, 0); // VMT 0x64 = a000:bda6
			refreshObject(kChain);
			setField(kChain, 0x30, kFlushTicks);
			setInputOff(true);
		}
		break;
	case 0xa000be2d: // toilet Se på
		say(handlerText(getFlag(kToilet) ? 0xa000be02 : 0xa000be20), 14, true);
		break;
	case 0xa000bcef: // used paper 212 Benyt: into the toilet, else as waste
		if (second == kToilet) {
			setFlag(kToilet, 1); // VMT 0x60 = a000:bd97
			removeObject(object);
			return true;
		}
		return runClassHandler(0xa00004ee, verb, object, second);

	// Hot and cold tap 141/142 (+0x2f closed, +0x30 award given); Tænd/Sluk call Åbn/Luk
	case 0xa0004ccf: // Se på
		say(handlerText(getFlag(object) ? 0xa0004ca5 : 0xa0004cbb), 14, true);
		break;
	case 0xa0004d31:
		return runClassHandler(0xa0004d73, kVerbOpen, object, second);
	case 0xa0004d44:
	case 0xa0004e15: // Luk
		if (getFlag(object)) {
			say(handlerText(0xa0004df7), 14, true);
			break;
		}
		setFlag(object, 1);
		refreshObject(object);
		if (!getField(object, 0x30)) {
			setField(object, 0x30, 1);
			award(10, 0, 0, 10, 0xa0004da4, 0xa0004df4);
		}
		break;

	// Sink tap 145 (+0x2f dripping); Tænd/Sluk call Åbn/Luk
	case 0xa000518b: // Se på
		say(handlerText(getFlag(kSinkTap) ? 0xa0005160 : 0xa0005173), 14, true);
		break;
	case 0xa00051ed:
		return runClassHandler(0xa000522f, kVerbOpen, object, second);
	case 0xa0005200:
	case 0xa00052a7: // Luk
		say(handlerText(getFlag(kSinkTap) ? 0xa0005260 : 0xa0005289), 14, true);
		break;
	case 0xa00053b0: // Benyt med the gasket (shop)
		if (second != kGasket) {
			sayRootMessage(kVerbUse);
		} else if (!getFlag(kSinkTap)) {
			say(handlerText(0xa0005397), 14, true);
		} else {
			removeObject(kGasket);
			setFlag(kSinkTap, 0);
			refreshObject(kSinkTap);
			award(10, 0, 0, 10, 0xa0005309, 0);
		}
		break;
	case 0xa0006041: // gasket 153 Benyt med the sink tap: the tap's Benyt (VMT 0x30)
		if (second == kSinkTap)
			executeVerb(kVerbUse, kSinkTap, kGasket);
		else
			sayRootMessage(kVerbUse);
		break;

	// Paper holder 149 (+0x2f what hangs on it, 100 nothing) and the kitchen roll
	case 0xa0005858: // Benyt med a roll
		if (second != kKitchenRoll && second != kToiletPaper && second != kPlainToiletPaper) {
			sayRootMessage(kVerbUse);
		} else if (getFlag(kPaperHolder) != kEmpty) {
			say(handlerText(0xa0005845), 14, true);
		} else {
			setFlag(kPaperHolder, second);
			removeObject(second);
			refreshObject(kPaperHolder);
			updateHolderName(kPaperHolder);
			// VMT 0x60 = a000:5af7
			if (second == kKitchenRoll) {
				award(-10, 0, -10, 0, 0xa00058f9, 0);
			} else if (second == kToiletPaper) {
				award(10, 0, 10, 0, 0xa000598d, 0);
			} else {
				award(0, 0, 0, 0, 0xa0005a1c, 0);
				setQuiz(0xa0005ab3, 0xa0005adb, 0xa0005ae7, 0, 2);
			}
		}
		break;
	case 0xa0005c0b: // Tag
		if (getFlag(kPaperHolder) != kEmpty)
			say(handlerText(0xa0005bf3), 14, true);
		else
			sayRootMessage(kVerbTake);
		break;
	case 0x99091961: // toilet paper 208 / 284 Benyt med holder 149 (0x95): its Benyt (VMT 0x30)
		if (second == kPaperHolder)
			executeVerb(kVerbUse, kPaperHolder, object);
		else
			sayRootMessage(kVerbUse);
		break;
	case 0x99091a31: // kitchen roll Benyt med a paper holder: the holder's Benyt (VMT 0x30)
		if (second == kPaperHolder || second == 151)
			executeVerb(kVerbUse, second, object);
		else
			sayRootMessage(kVerbUse);
		break;

	// Shower 209 (+0x2f water saver)
	case 0xa000bbf4: // Se på
		say(handlerText(getFlag(kShower) ? 0xa000bbbe : 0xa000bbd5), 14, true);
		break;
	case 0xa000baa5: // Benyt med the water saver (shop)
		if (second != kSaverShower) {
			sayRootMessage(kVerbUse);
			break;
		}
		setFlag(kShower, 1);
		removeObject(kSaverShower);
		refreshObject(kShower);
		award(10, 0, 0, 10, 0xa000ba44, 0);
		break;
	case 0x99091a76: // water saver 210 Benyt med the shower: the shower's Benyt (VMT 0x30)
		if (second == kShower)
			executeVerb(kVerbUse, kShower, object);
		else
			sayRootMessage(kVerbUse);
		break;

	// Little brother 154 (+0x30 his state, littleBrotherFigure)
	case 0xa0006720: // Se på: waiting for his bath, or brushing his teeth
		if (getField(object, 0x30) == 2)
			say(handlerText(0xa00066e9), 14, true);
		else if (getField(object, 0x30) == 5)
			say(handlerText(0xa000670c), 14, true);
		break;
	case 0xa0006782: { // Hjælp while he waits: close-up 26
		if (getField(object, 0x30) != 2) {
			sayRootMessage(kVerbHelp);
			break;
		}
		// GoTo(26, 0), the room ends; his laundry (proxy 211, DS:cc5a, VMT
		// 0x64 = ac7f:044f) is shown
		setFlag(kLaundryProxy, 1);
		refreshObject(kLaundryProxy);
		int choice = runBathChoice();
		if (choice > 0) {
			// 8a94:05bf: +0x32 := the choice, VMT 0x68 (next state), VMT 0x5c
			// (+0x2c := 1: the award when the room is built again)
			setField(object, 0x32, choice);
			littleBrotherNext();
			setField(object, 0x2c, 1);
		}
		// GoTo(9, 0): the bathroom again, the player stays
		redrawRoom();
		break;
	}

	// Washing machine 132: +0x2f idle (0 = washing), +0x30 door closed,
	// +0x32 load (0..3), +0x33 powder
	case 0x990905ba: { // Se på
		static const uint32 kLoad[] = { 0x9909055c, 0x99090568, 0x99090582, 0x9909059b };
		if (!getFlag(object))
			say(handlerText(0x990905ae), 14, true);
		else
			say(handlerText(kLoad[CLIP(getField(object, 0x32), 0, 3)]), 14, true);
		break;
	}
	case 0xa00054aa: // laundry 143/144, towels 187 Benyt med the machine: its Benyt (VMT 0x30)
		if (second == kWashingMachine)
			executeVerb(kVerbUse, kWashingMachine, object);
		else
			sayRootMessage(kVerbUse);
		break;
	case 0x990908d4: // Åbn
		if (!getFlag(object)) {
			say(handlerText(0x990908a9), 14, true);
		} else if (!getField(object, 0x30)) {
			say(handlerText(0x990908be), 14, true);
		} else {
			setField(object, 0x30, 0);
			refreshObject(object);
			playSound(kSoundOpen);
		}
		break;
	case 0x99090970: // Luk
		if (getField(object, 0x30)) {
			say(handlerText(0x99090958), 14, true);
		} else {
			setField(object, 0x30, 1);
			refreshObject(object);
			playSound(kSoundClose);
		}
		break;
	case 0x990909d2: // Benyt med laundry or towels: in it
		if (getField(object, 0x30)) {
			say(handlerText(0x990909c3), 14, true);
		} else if (second == kLaundry1 || second == kLaundry2 || second == kTowels) {
			setField(object, 0x32, getField(object, 0x32) + 1);
			removeObject(second);
			refreshObject(object);
		} else {
			sayRootMessage(kVerbUse);
		}
		break;
	case 0x990906d9: // Tænd
		if (!getField(object, 0x30))
			say(handlerText(0x9909066d), 14, true);
		else if (!getFlag(object))
			say(pronoun + handlerText(0x990906c5), 14, true);
		else if (!getField(object, 0x32))
			say(handlerText(0x9909067e), 14, true);
		else if (!getField(object, 0x33))
			say(handlerText(0x990906a0), 14, true);
		else
			startWashProgram();
		break;
	case 0x9909082e: // Sluk
		if (!getFlag(object))
			say(handlerText(0x990907f7), 14, true);
		else
			say(pronoun + handlerText(0x99090818), 14, true);
		break;
	case 0x99090c48: // washing powder Benyt med the machine: its VMT 0x64 (9909:04de)
		if (second != kWashingMachine) {
			sayRootMessage(kVerbUse);
		} else if (!getFlag(kWashingMachine)) {
			say(handlerText(0x990904d2), 14, true);
		} else {
			// GoTo(29, 0): the dosing scale; 11f8:0699 stores the dose in the
			// machine's +0x33, then GoTo(9, 0)
			setField(kWashingMachine, 0x33, runDosing());
			redrawRoom();
		}
		break;
	case 0x99090c18: { // washing powder Se på: close-up 27 (GoTo(27, 0))
		int hardness[6];
		for (int k = 0; k < 6; ++k)
			hardness[k] = getField(kPowder, 0x2f + k);
		runPowderBox(hardness);
		redrawRoom();
		break;
	}
	case 0x99090cd5: // schedule Se på: close-up 28 (GoTo(28, 0))
		runSchedule();
		redrawRoom();
		break;

	default:
		return false;
	}
	return true;
}

} // End of namespace Miljostrup
