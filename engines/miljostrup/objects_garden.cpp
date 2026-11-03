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

// The garden (home, room 22): hoses, tap, rain barrel, sprinklers, plants,
// greenhouse, mealybugs and compost, written from notes/garden.md. Most of
// the classes are in unit 9909.

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"

namespace Miljostrup {

enum {
	kLawnSprinkler = 229,  // havevander, DS:cac2
	kRedHose = 230,        // DS:ca7a, feeds the lawn sprinkler
	kGreenHose = 231,      // DS:ca7e, feeds the greenhouse sprinklers
	kTap = 232,            // DS:ca82
	kBarrel = 233,         // DS:ca86, rain water
	kPlants = 234,         // DS:ca8a
	kFirstSprinkler = 235, // 235..238, DS:ca8e..ca9a
	kLastSprinkler = 238,
	kGreenhouse = 239,     // DS:ca9e
	kCompost = 240,        // DS:caa6
	kMealybugs = 243,      // DS:caa2
	kMites = 244,          // rovmider (shop)
	kPesticide = 245,      // sprøjtegift (proxy 246)
	kNothing = 100,        // a hose or a tap/barrel without partner

	kWaterTicks = 150,     // VMT 0x68(150): idle ticks of a tap or barrel run
	kGreenhouseTicks = 8   // the greenhouse figure's frames per step (8c31:0823)
};

// Hoses (+0x2f): what they hang on. Tap and barrel: +0x32 the hose, +0x34
// running, timer +0x2f / +0x31 (a000:0aff); barrel +0x35 empty, +0x36 used
// on the lawn. Plants +0x2f watered. Mealybugs +0x2f present, +0x30 shown.

bool MiljostrupEngine::isHoseRunning(int hose) const {
	// 9909:1f50: the tap's or the barrel's +0x34
	switch (getFlag(hose)) {
	case kTap:
		return getField(kTap, 0x34) != 0;
	case kBarrel:
		return getField(kBarrel, 0x34) != 0;
	default:
		return false;
	}
}

void MiljostrupEngine::refreshSprinklers() {
	refreshObject(kGreenHose);
	for (int i = kFirstSprinkler; i <= kLastSprinkler; ++i)
		refreshObject(i);
}

void MiljostrupEngine::setMealybugsShown(bool shown) {
	// 9909:3264 (VMT 0x60) / 9909:327c (VMT 0x64)
	setField(kMealybugs, 0x30, shown ? 1 : 0);
	refreshObject(kMealybugs);
}

void MiljostrupEngine::setInputOff(bool off) {
	// Room VMT 0x24 = 2395:02ec: input off shows the hourglass
	_inputOff = off;
	_interface->setCursor(off ? Interface::kCursorWait : Interface::kCursorArrow);
}

void MiljostrupEngine::startWater(int source) {
	// 9909:2397 / 2990: the water runs for 150 idle ticks (VMT 0x68 = a000:0b71)
	setField(source, 0x34, 1);
	setInputOff(true);
	setFlag(source, kWaterTicks);
	setField(source, 0x31, 1);
}

void MiljostrupEngine::tickGarden() {
	if (_location != 1)
		return;
	// 8c31:0bd1, the garden's idle tick: tap and barrel count down while
	// running (VMT 0x60 = a000:0b87); at 0 VMT 0x64 ends the water
	static const int kSources[] = { kTap, kBarrel };
	for (int k = 0; k < 2; ++k) {
		int source = kSources[k];
		if (!getField(source, 0x31))
			continue;
		setFlag(source, getFlag(source) - 1);
		if (getFlag(source) == 0) {
			setField(source, 0x31, 0);
			waterEnded(source);
		}
	}

	// The greenhouse's look (9909:2fc5, 301b, 3070), steps of 8 frames
	if (_greenhouseStage && --_greenhouseTimer <= 0) {
		if (_greenhouseStage == 1) {
			// message 0x23 -> VMT 0x64: the magnifier for 34 steps
			_greenhouseStage = 2;
			_greenhouseTimer = 34 * kGreenhouseTicks;
		} else if (_greenhouseStage == 2) {
			// then state 2 for one step, message 0x24
			_greenhouseStage = 3;
			_greenhouseTimer = kGreenhouseTicks;
		} else {
			// VMT 0x68: the mealybugs again, input on
			_greenhouseStage = 0;
			setMealybugsShown(true);
			setInputOff(false);
		}
		refreshObject(kGreenhouse);
	}
}

void MiljostrupEngine::waterEnded(int source) {
	// 9909:2269 (tap) / 9909:280a (barrel)
	setInputOff(false);
	setField(source, 0x34, 0);
	bool greenhouse = getField(source, 0x32) == kGreenHose;
	if (source == kTap) {
		if (!greenhouse)
			return;
		refreshSprinklers();
		setMealybugsShown(true);
		award(0, 0, 0, 0, 0x990921e5, 0);
	} else if (greenhouse) {
		refreshSprinklers();
		setMealybugsShown(true);
		award(10, 0, 0, 10, 0x990926ec, 0);
	} else {
		// red hose: the barrel is empty, the lawn did not get enough
		setField(kBarrel, 0x35, 1);
		refreshObject(kRedHose);
		refreshObject(kLawnSprinkler);
		award(0, 0, 0, 0, 0x9909274d, 0);
	}
}

int MiljostrupEngine::getGardenFigureState(uint32 rule, int object, bool &visible) const {
	switch (rule) {
	case 0x99091cf4: // lawn sprinkler: spraying while the red hose has water
		return isHoseRunning(kRedHose) ? 2 : 1;
	case 0x99092038: // hose: loose 1, on the tap 2, on the barrel 3
		return getFlag(object) == kTap ? 2 : (getFlag(object) == kBarrel ? 3 : 1);
	case 0x99092dd4: // plants: healthy (watered, no mealybugs) 1, else 2
		return getFlag(kPlants) && !getFlag(kMealybugs) ? 1 : 2;
	case 0x99092e8b: // greenhouse sprinklers: spraying while the green hose has water
		return isHoseRunning(kGreenHose) ? 2 : 1;
	case 0x990930a2: // greenhouse: the magnifier (state 1) only while looking
		return _greenhouseStage == 2 ? 1 : 2;
	case 0x990932ac: // mealybugs: present and shown 1, else 2 (empty image)
		return getFlag(object) && getField(object, 0x30) ? 1 : 2;
	case 0x9909318f: // compost: the heap is shown once garden waste is in it
		visible = getFlag(object) != 0;
		return 1;
	default:
		return 0;
	}
}

bool MiljostrupEngine::runGardenHandler(uint32 handler, int verb, int object, int second) {
	switch (handler) {
	case 0x99091c6d: // lawn sprinkler Se på
		say(handlerText(isHoseRunning(kRedHose) ? 0x99091c46 : 0x99091c5d), 14, true);
		break;

	// Hoses
	case 0x99091e04: // Se på
		if (getFlag(object) == kNothing)
			say(handlerText(0x99091dad), 14, true);
		else if (getFlag(object) == kTap)
			say(handlerText(0x99091dc0), 14, true);
		else if (getFlag(object) == kBarrel)
			say(handlerText(0x99091ddd), 14, true);
		break;
	case 0x99091f3d: // Træk: the hose's Tag (VMT 0x34)
	case 0x99091eb0: // Tag: take it off the tap or the barrel
		if (getFlag(object) != kTap && getFlag(object) != kBarrel) {
			sayRootMessage(kVerbTake);
		} else if (isHoseRunning(object)) {
			say(handlerText(0x99091e99), 14, true);
		} else {
			setField(getFlag(object), 0x32, kNothing); // VMT 0x74 = 9909:2164
			setFlag(object, kNothing);
			refreshObject(object);
		}
		break;
	case 0x99091fbc: // Benyt X med tap/barrel: put it on, if that one is free (VMT 0x6c)
		if (getFlag(object) == kNothing && (second == kTap || second == kBarrel) &&
				getField(second, 0x32) == kNothing) {
			setFlag(object, second);
			refreshObject(object);
			setField(second, 0x32, object); // VMT 0x70 = 9909:212d
		} else {
			sayRootMessage(kVerbUse);
		}
		break;

	// Tap and barrel: Åbn = Tænd (VMT 0x4c), Luk = Sluk (VMT 0x50)
	case 0x9909213e:
		return runClassHandler(object == kTap ? 0x99092397 : 0x99092990, kVerbTurnOn, object, second);
	case 0x99092151:
		return runClassHandler(object == kTap ? 0x99092556 : 0x99092b3f, kVerbTurnOff, object, second);
	case 0x99092397: // tap Tænd
		if (getField(kTap, 0x34)) {
			say(handlerText(0x99092327), 14, true);
		} else if (getField(kTap, 0x32) == kNothing) {
			say(handlerText(0x9909233e), 14, true);
		} else if (getField(kTap, 0x32) == kRedHose) {
			say(handlerText(0x99092363), 14, true);
		} else if (getFlag(kPlants)) {
			say(handlerText(0x99092379), 14, true);
		} else {
			startWater(kTap);
			setFlag(kPlants, 1); // VMT 0x64 = 9909:2c6b
			refreshSprinklers();
			setMealybugsShown(false);
		}
		break;
	case 0x99092556: // tap Sluk
		if (!getField(kTap, 0x34)) {
			say(handlerText(0x9909253d), 14, true);
			break;
		}
		setField(kTap, 0x34, 0);
		if (getField(kTap, 0x32) == kRedHose) {
			refreshObject(kRedHose);
			refreshObject(kLawnSprinkler);
			award(10, 0, 0, 10, 0x990924f2, 0);
		}
		break;
	case 0x99092615: // tap Se på
		say(handlerText(getField(kTap, 0x34) ? 0x990925f9 : 0x99092606), 14, true);
		break;
	case 0x99092990: // barrel Tænd
		if (getField(kBarrel, 0x32) == kNothing) {
			say(handlerText(0x9909292b), 14, true);
		} else if (getField(kBarrel, 0x35)) {
			say(handlerText(0x99092950), 14, true);
		} else if (getField(kBarrel, 0x32) == kRedHose && getField(kBarrel, 0x36)) {
			say(handlerText(0x9909295c), 14, true);
		} else if (getField(kBarrel, 0x32) == kGreenHose && getFlag(kPlants)) {
			say(handlerText(0x99092972), 14, true);
		} else {
			startWater(kBarrel);
			if (getField(kBarrel, 0x32) == kRedHose) {
				setField(kBarrel, 0x36, 1);
				refreshObject(kRedHose);
				refreshObject(kLawnSprinkler);
			} else {
				setFlag(kPlants, 1);
				refreshSprinklers();
				setMealybugsShown(false);
			}
		}
		break;
	case 0x99092b9d: // barrel Se på
		say(handlerText(getField(kBarrel, 0x35) ? 0x99092b70 : 0x99092b7c), 14, true);
		break;

	case 0x99092d08: // plants Se på
		if (getFlag(kPlants) && !getFlag(kMealybugs))
			say(handlerText(0x99092ca7), 14, true);
		else if (getFlag(kMealybugs))
			say(handlerText(getFlag(kPlants) ? 0x99092cdb : 0x99092cb8), 14, true);
		else
			say(handlerText(0x99092cf5), 14, true);
		break;
	case 0x99092f5f: // greenhouse Se på: the mealybugs under the magnifier
		if (!getFlag(kMealybugs)) {
			sayRootMessage(kVerbLook);
			break;
		}
		say(handlerText(0x99092f3e), 10, true);
		setInputOff(true);
		setMealybugsShown(false);
		// 9909:2fc5: Walk(2, 0, 0, 3), then message 0x23
		_greenhouseStage = 1;
		_greenhouseTimer = 3 * kGreenhouseTicks;
		break;

	case 0x99093378: // predatory mites Benyt med the greenhouse (0xef) or the plants (0xea)
	case 0x99091b6d: // pesticide: the same, but other second objects -> the waste handler
		if (second != kGreenhouse && second != kPlants) {
			if (handler == 0x99091b6d)
				return runClassHandler(0xa00004ee, verb, object, second);
			sayRootMessage(kVerbUse);
		} else if (!getFlag(kMealybugs)) {
			say(handlerText(handler == 0x99091b6d ? 0x99091b51 : 0x9909335c), 14, true);
		} else {
			// The mealybugs' VMT 0x68 (9909:3294): +0x2f := 0, their figure;
			// the room's VMT 0x54 takes the item. Mites Award(10, 0, 0, 10);
			// pesticide Award(-10, 0, 0, -10) with page "31". The award
			// rebuilds the room, so the plants' figure follows too.
			setFlag(kMealybugs, 0);
			refreshObject(kMealybugs);
			refreshObject(kPlants);
			removeObject(object);
			if (handler == 0x99091b6d)
				award(-10, 0, 0, -10, 0x99091ab4, 0x99091b4e);
			else
				award(10, 0, 0, 10, 0x990932f5, 0);
		}
		break;
	case 0x9909359f: // garden waste Benyt: into the compost shows the heap
		if (second == kCompost) {
			setFlag(kCompost, 1); // VMT 0x60 = 9909:3177
			refreshObject(kCompost);
		}
		return runClassHandler(0xa00004ee, verb, object, second);

	default:
		return false;
	}
	return true;
}

} // End of namespace Miljostrup
