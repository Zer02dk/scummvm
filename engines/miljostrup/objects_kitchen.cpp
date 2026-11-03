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

// The kitchen (home, room 8), written from notes/kitchen.md, with its
// close-ups 13/14 (thermostats), 15 (potatoes) and 25 (water for the eggs).

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/rooms.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/response_data.h"

namespace Miljostrup {

enum {
	kKitchenDoor = 126,
	kFridge = 127,          // DS:cb16
	kFreezer = 128,         // DS:cb1a
	kOvenCupboard = 129,    // DS:cb1e, below the oven
	kFridgeThermostat = 138,  // DS:cb3e
	kFreezerThermostat = 139, // DS:cb42
	kPotatoes = 140,
	kKitchenHolder = 151,
	kKitchenRoll = 150,
	kEggTray = 155,         // DS:cb86
	kEgg = 156,
	kSinkCupboard = 157,    // DS:cb8e
	kWasteBag = 170,        // DS:cb92
	kFlute = 173,           // DS:cbd2, +0x2f toasted
	kFluteInFreezer = 174,  // proxy, DS:cbd6
	kChicken = 176,
	kChickenInOven = 177,   // proxy, DS:cbe6
	kChickenInFridge = 178, // proxy, DS:cbea
	kOven = 175,            // DS:cbde
	kSaucepan = 184,        // DS:cc02
	kLid = 185,
	kLidInCupboard = 186,   // proxy, DS:cc0a
	kFluteInOven = 203,     // proxy, DS:cbda
	kToaster = 204,         // DS:cc4a
	kLidOnSaucepan = 205,   // proxy, DS:cc0e
	kKitchenTap = 206,
	kNothingInside = 100,

	// The fridge's "closed it" award is given once: DS:b974, cleared by
	// the fridge/freezer constructor a000:2977. Kept as a field of the fridge.
	kFridgeAwardField = 0xf0,
	kOvenTicks = 100,       // a000:8897: +0x33 := 100 idle ticks
	kToasterFrames = 15 * 3 // a000:b9a4: Walk(2, 0, 0, 15), 3 frames per step
};

bool MiljostrupEngine::isKitchenProxyOpen(int proxy) const {
	// The proxies' subclasses (a000:8319, 839c, 8c22, 8cce, a198) give them a
	// region only while their place is open
	switch (proxy) {
	case kFluteInFreezer:
		return getFlag(kFreezer) != 0;
	case kFluteInOven:
	case kChickenInOven:
		return getFlag(kOven) == 0;
	case kChickenInFridge:
		return getFlag(kFridge) != 0;
	case kLidInCupboard:
		return getFlag(kOvenCupboard) != 0;
	default:
		return true;
	}
}

bool MiljostrupEngine::isKitchenObjectEnabled(int object, bool &enabled) const {
	switch (object) {
	case kFridgeThermostat: // a000:4313: region while the fridge is open
	case kEggTray:          // a000:6fad
		enabled = getFlag(kFridge) != 0;
		return true;
	case kFreezerThermostat: // a000:47c7
		enabled = getFlag(kFreezer) != 0;
		return true;
	case kWasteBag: // a000:7531: while the cupboard is open
		enabled = getFlag(kSinkCupboard) != 0;
		return true;
	case kPotatoes: // a000:4b9b: gone once peeled
		enabled = getField(kPotatoes, 0x30) == 0;
		return true;
	default:
		return false;
	}
}

bool MiljostrupEngine::isKitchenTagProxy(int proxy) const {
	return proxy == kChickenInOven || proxy == kFluteInOven || proxy == kChickenInFridge ||
		proxy == kLidOnSaucepan;
}

bool MiljostrupEngine::runKitchenProxyTag(int proxy) {
	switch (proxy) {
	case kChickenInOven: // a000:8c04 / 837e: the oven is empty, then the proxy's Tag
	case kFluteInOven:
		setField(kOven, 0x31, kNothingInside); // VMT 0x64 = a000:84a6
		refreshObject(kOven);
		return false;
	case kChickenInFridge: // a000:8c9d
		say(handlerText(0xa0008c87), 14, true);
		return true;
	case kLidOnSaucepan: // a000:a21a: not while it is hot, else the lid is off
		if (getField(kSaucepan, 0x31)) {
			say(handlerText(0xa000a1fd), 14, true);
			return true;
		}
		setFlag(kSaucepan, 0); // VMT 0x64 = a000:9e16
		return false;
	default:
		return false;
	}
}

void MiljostrupEngine::refreshEggTray() {
	refreshObject(kEggTray);
	// a000:6eef (GetName): open with the egg in it, the tray is called after
	// the egg (DS:cb8a = 156)
	if (!getFlag(kEggTray) && getField(kEggTray, 0x30))
		_interface->setNameOverride(kEggTray, _interface->getObjectName(kEgg));
	else
		_interface->removeNameOverride(kEggTray);
}

void MiljostrupEngine::thermostatAward(int object) {
	// a000:3fe3 (fridge) / a000:4508 (freezer): "Du har sat termostaten på "
	// + n + "øC" (ac76:0003) + a text by the temperature; Award(points,
	// energy, waste, 0) and a quiz (first answer, category 1)
	int t = getFlag(object);
	Common::String n = Common::String::format("%d", t) + readPascalString(_ovr, kTextDegrees);
	int p, e, w;
	Common::String text;
	if (object == kFridgeThermostat) {
		text = handlerText(0xa0003d6f) + n;
		if (t < 0) {
			p = -10; e = -10; w = 0;
			text += handlerText(0xa0003d8a) + handlerText(0xa0003d9b) + handlerText(0xa0003dd5);
		} else if (t <= 1) {
			p = e = t == 0 ? -5 : -1; w = 0;
			text += handlerText(0xa0003dff) + handlerText(0xa0003e18) + handlerText(0xa0003e54);
		} else if (t <= 4) {
			static const int kGood[3] = { 2, 4, 7 };
			p = e = kGood[t - 2]; w = 0;
			text += handlerText(0xa0003e77) + handlerText(0xa0003e91) + handlerText(0xa0003ece);
		} else if (t == 5) {
			p = e = 10; w = 0;
			text += handlerText(0xa0003ede) + handlerText(0xa0003efd);
		} else {
			if (t == 6) {
				p = -5; e = -3; w = -2;
			} else {
				p = -10; e = -6; w = -4;
			}
			text += handlerText(0xa0003f32) + handlerText(0xa0003f50) + handlerText(0xa0003f8e);
		}
		awardText(p, e, w, 0, text, Common::String());
		_award.question = handlerText(0xa0003fa6);
		_award.answer1 = handlerText(0xa0003fcd);
		_award.answer2 = handlerText(0xa0003fd7);
	} else {
		text = handlerText(0xa0004349) + n + handlerText(0xa0004364);
		if (t <= -24) {
			p = e = -5; w = 0;
		} else if (t <= -19) {
			static const int kBetter[5] = { 0, 2, 4, 6, 8 }; // -23..-19
			p = e = kBetter[t + 23]; w = 0;
		} else if (t == -18) {
			p = e = 10; w = 0;
		} else if (t == -17) {
			p = -5; e = -3; w = -2;
		} else {
			p = -10; e = -6; w = -4;
		}
		if (t <= -23)
			text += handlerText(0xa0004367) + handlerText(0xa0004391);
		else if (t <= -19)
			text += handlerText(0xa00043b2) + handlerText(0xa00043e3);
		else if (t == -18)
			text += handlerText(0xa000441b) + handlerText(0xa000444b);
		else // a000:4728 starts a new text here, without the "Du har sat ..." part
			text = handlerText(0xa000445b) + n + handlerText(0xa0004472) + handlerText(0xa00044ab);
		awardText(p, e, w, 0, text, Common::String());
		_award.question = handlerText(0xa00044c8);
		_award.answer1 = handlerText(0xa00044f3);
		_award.answer2 = handlerText(0xa00044fe);
	}
	_award.quiz = true;
	_award.correct = 1;
	_award.category = 1;
}

void MiljostrupEngine::cookEggs() {
	// a000:9ff2: on, then VMT 0x60 = a000:9b51, the award by the water and the lid
	setField(kSaucepan, 0x31, 1);
	int water = CLIP(getField(kSaucepan, 0x32), 1, 4);
	bool lid = getFlag(kSaucepan) != 0;
	static const int kNoLid[4][3] = { { 0, 0, 0 }, { -3, -2, -1 }, { -6, -4, -2 }, { -10, -7, -3 } };
	static const int kLid[4][3] = { { 10, 6, 4 }, { -2, -1, -1 }, { -5, -3, -2 }, { -8, -5, -3 } };
	const int *v = lid ? kLid[water - 1] : kNoLid[water - 1];
	Common::String text = handlerText(0xa000994c) + Common::String::format("%d", water) + handlerText(0xa000996d);
	if (!lid && water == 1)
		text += handlerText(0xa0009976) + handlerText(0xa0009979) + handlerText(0xa00099a2);
	else if (!lid)
		text += handlerText(0xa0009976) + handlerText(0xa00099cb) + handlerText(0xa00099f3) + handlerText(0xa0009a29);
	else if (water == 1)
		text += handlerText(0xa0009a3d) + handlerText(0xa0009a67) + handlerText(0xa0009a95);
	else
		text += handlerText(0xa0009ab2) + handlerText(0xa0009ae0);
	// Award(points, energy, 0, water)
	awardText(v[0], v[1], 0, v[2], text, Common::String());
	setQuiz(0xa0009b12, 0xa0009b3e, 0xa0009b48, 1, 1);
}

void MiljostrupEngine::refreshFridge() {
	refreshObject(kFridge);
	refreshObject(kFridgeThermostat);
	refreshEggTray();
	refreshObject(kChickenInFridge);
}

void MiljostrupEngine::tickKitchen() {
	if (_location != 1)
		return;
	// 91f4:1265: the oven's VMT 0x68 (a000:89b6) while it is on
	if (getField(kOven, 0x30)) {
		setField(kOven, 0x33, getField(kOven, 0x33) - 1);
		if (getField(kOven, 0x33) == 0) {
			setField(kOven, 0x30, 0);
			refreshObject(kOven);
			setInputOff(false);
			setFlag(kFlute, 1); // VMT 0x60: toasted
			award(-10, -10, 0, 0, 0xa00088d9, 0);
			setQuiz(0xa0008980, 0xa000899e, 0xa00089ab, 1, 1);
		}
	}
	// The toaster's walk ends with message 0x22 -> VMT 0x60 (a000:b85c)
	if (_toasterTimer && --_toasterTimer == 0) {
		setInputOff(false);
		setFlag(kToaster, 0);
		setField(kToaster, 0x31, 0);
		setField(kToaster, 0x30, 1);
		refreshObject(kToaster);
		award(10, 10, 0, 0, 0xa000b7d9, 0);
	}
}

bool MiljostrupEngine::canLeaveKitchen(int door) {
	// a000:a2c0: the open door asks the fridge and the freezer (VMT 0x60 = a000:2b05)
	if (_location != 1 || door != kKitchenDoor || !_doors.contains(door) || !_doors[door])
		return true;
	if (!getFlag(kFridge) && !getFlag(kFreezer))
		return true;
	if (getField(kFridge, kFridgeAwardField)) {
		say(handlerText(0xa0002ae3), 14, true);
		return false;
	}
	Common::String what, them;
	if (getFlag(kFridge) && getFlag(kFreezer)) {
		what = handlerText(0xa00029e8);
		them = handlerText(0xa00029ff);
	} else if (getFlag(kFridge)) {
		what = handlerText(0xa0002a02);
		them = handlerText(0xa0002a0d);
	} else {
		what = handlerText(0xa0002a11);
		them = handlerText(0xa0002a1a);
	}
	awardText(-10, -10, 0, 0, handlerText(0xa0002a1e) + what + handlerText(0xa0002a32) +
		handlerText(0xa0002a40) + them + handlerText(0xa0002a4c) + handlerText(0xa0002a6a),
		handlerText(0xa0002adc));
	setQuiz(0xa0002aa8, 0xa0002ac7, 0xa0002ad6, 1, 1);
	setField(kFridge, kFridgeAwardField, 1);
	return false;
}

int MiljostrupEngine::getKitchenFigureState(uint32 rule, int object, bool &visible) const {
	switch (rule) {
	case 0xa0003195: // fridge: open 1, closed 2
	case 0xa000340c: // freezer
	case 0xa00036ae: // cupboard below the oven
		return getFlag(object) ? 1 : 2;
	case 0xa00072ec: // cupboard 157: open 3, closed 1
		return getFlag(object) ? 3 : 1;
	case 0xa0004b9b: // potatoes: gone once peeled
		visible = getField(object, 0x30) == 0;
		return 1;
	case 0xa0005fb1: // paper holder: empty 1, kitchen roll 2
		return getFlag(object) == kKitchenRoll ? 2 : 1;
	case 0xa0006fad: // egg tray: closed 1, open with the egg 2, open and empty 3
		if (getFlag(object))
			return 1;
		return getField(object, 0x30) ? 2 : 3;
	case 0xa0008b76: // oven (+0x2f = 1 closed): open 1, closed 2, closed and on 3
		if (!getFlag(object))
			return 1;
		return getField(object, 0x30) ? 3 : 2;
	case 0xa000b95d: // toaster: done 3, toasting 2, else 1
		if (getField(object, 0x30))
			return 3;
		return getFlag(object) ? 2 : 1;
	case 0xa000a135: // saucepan
		return 1;
	default:
		return 0;
	}
}

bool MiljostrupEngine::runKitchenHandler(uint32 handler, int verb, int object, int second) {
	switch (handler) {
	// The flute 173 (+0x2f toasted): Se på
	case 0xa00082b7:
		say(handlerText(getFlag(object) ? 0xa0008297 : 0xa00082a6), 14, true);
		break;
	// Fridge 127 (+0x2f open, +0x30 the chicken in it)
	case 0xa0002de5: // Benyt med the chicken
		if (!getFlag(kFridge)) {
			say(handlerText(0xa0002d4f), 14, true);
		} else if (second == kChicken) {
			setField(kFridge, 0x30, 1);
			removeObject(kChicken);
			setFlag(kChickenInFridge, 1); // VMT 0x64(1) = ac7f:044f
			refreshFridge();
			award(10, 0, 10, 0, 0xa0002d65, 0);
		} else {
			sayRootMessage(kVerbUse);
		}
		break;
	case 0xa0002ef6: // Se på
		if (getFlag(kFridge))
			say(handlerText(0xa0002ec5) + Common::String::format("%d", getFlag(kFridgeThermostat)) +
				readPascalString(_ovr, kTextDegrees) + handlerText(0xa0002ee5), 14, true);
		else
			say(handlerText(0xa0002ee7), 14, true);
		break;
	case 0xa0002f93: // Åbn
		if (getFlag(kFridge)) {
			say(handlerText(0xa0002f7c), 14, true);
		} else {
			setFlag(kFridge, 1);
			refreshFridge();
			playSound(kSoundOpen);
		}
		break;
	case 0xa00030b3: // Luk: the first time an award (DS:b974)
		if (!getFlag(kFridge)) {
			say(handlerText(0xa000300d), 14, true);
			break;
		}
		setFlag(kFridge, 0);
		refreshFridge();
		if (!getField(kFridge, kFridgeAwardField)) {
			setField(kFridge, kFridgeAwardField, 1);
			award(10, 10, 0, 0, 0xa0003025, 0xa00030ac);
		}
		playSound(kSoundOpen); // 1d26:010f, as when opening
		break;

	// Freezer 128 (+0x2f open)
	case 0xa0003273: // Se på
		if (getFlag(kFreezer))
			say(handlerText(0xa000323a) + Common::String::format("%d", getFlag(kFreezerThermostat)) +
				readPascalString(_ovr, kTextDegrees) + handlerText(0xa000325d), 14, true);
		else
			say(handlerText(0xa000325f), 14, true);
		break;
	case 0xa0003315: // Åbn
	case 0xa000339f: // Luk
		if ((getFlag(kFreezer) != 0) == (handler == 0xa0003315)) {
			say(handlerText(handler == 0xa0003315 ? 0xa00032f9 : 0xa0003382), 14, true);
		} else {
			setFlag(kFreezer, handler == 0xa0003315 ? 1 : 0);
			refreshObject(kFreezer);
			refreshObject(kFreezerThermostat);
			refreshObject(kFluteInFreezer);
			playSound(kSoundOpen);
		}
		break;

	// The two cupboards (+0x2f open); both open only while the oven is closed (VMT 0x60)
	case 0xa0003511: // 129 Se på
		say(handlerText(getFlag(object) ? 0xa00034d4 : 0xa00034fa), 14, true);
		break;
	case 0xa000717f: // 157 Se på
		say(handlerText(getFlag(object) ? 0xa0007142 : 0xa0007168), 14, true);
		break;
	case 0xa000359f: // 129 Åbn
	case 0xa00071f8: // 157 Åbn
		if (!getFlag(kOven)) {
			if (handler == 0xa000359f)
				say(handlerText(0xa0003573), 14, true);
			else
				sayRootMessage(kVerbOpen);
		} else if (getFlag(object)) {
			say(handlerText(handler == 0xa000359f ? 0xa0003588 : 0xa00071e1), 14, true);
		} else {
			setFlag(object, 1);
			refreshObject(object);
			refreshObject(object == kOvenCupboard ? kLidInCupboard : kWasteBag);
			playSound(kSoundOpen);
		}
		break;
	case 0xa000364e: // 129 Luk
	case 0xa000728c: // 157 Luk
		if (!getFlag(object)) {
			say(handlerText(handler == 0xa000364e ? 0xa0003636 : 0xa0007274), 14, true);
		} else {
			setFlag(object, 0);
			refreshObject(object);
			refreshObject(object == kOvenCupboard ? kLidInCupboard : kWasteBag);
			playSound(kSoundOpen);
		}
		break;

	// Thermostats 138/139: Se på; Benyt = a000:3cd1 (close-ups 13/14)
	case 0xa0003c61:
		say(handlerText(0xa0003c4a) + Common::String::format("%d", getFlag(object)) +
			readPascalString(_ovr, kTextDegrees) + handlerText(0xa0003c5f), 14, true);
		break;
	case 0xa0003cd1: {
		if (getField(object, 0x33)) {
			say(handlerText(0xa0003cb5), 14, true);
			break;
		}
		// GoTo(+0x31 = 13 / 14): 8f1b:05a9 / 0639 with picture 236 / 237;
		// unless cancelled a000:3d3a (+0x2f := it, +0x33 := 1) and VMT 0x5c,
		// so the kitchen's rebuild gives the award (VMT 0x60)
		int t = runThermostatDisplay(object == kFridgeThermostat ? 236 : 237, getFlag(object));
		if (t != -999) {
			setFlag(object, t);
			setField(object, 0x33, 1);
		}
		redrawRoom();
		if (t != -999)
			thermostatAward(object);
		break;
	}

	// Potatoes 140: Benyt -> close-up 15 (8ebc:056a)
	case 0xa0004b6b: {
		int choice = runPotatoPeeling();
		if (choice >= 0) {
			// VMT 0x60 = a000:485c: peeled, the choice, the peel 160 into the
			// inventory; VMT 0x5c makes the room's rebuild give the award
			setField(kPotatoes, 0x30, 1);
			setFlag(kPotatoes, choice);
			takeObject(160);
		}
		redrawRoom();
		if (choice < 0)
			break;
		// a000:4a31
		if (choice == 2)
			award(0, 0, 0, 0, 0xa0004882, 0);
		else if (choice == 1)
			award(10, 0, 0, 10, 0xa000490e, 0);
		else
			award(-10, 0, 0, -10, 0xa0004979, 0);
		setQuiz(0xa00049f1, 0xa0004a1e, 0xa0004a25, 0, 3);
		break;
	}

	// Kitchen paper holder 151 (+0x2f the roll on it, 100 none)
	case 0xa0005e28: // Benyt med the kitchen roll
		if (second != kKitchenRoll) {
			sayRootMessage(kVerbUse);
		} else if (getFlag(kKitchenHolder) != kNothingInside) {
			say(handlerText(0xa0005e15), 14, true);
		} else {
			setFlag(kKitchenHolder, kKitchenRoll);
			removeObject(kKitchenRoll);
			refreshObject(kKitchenHolder);
			updateHolderName(kKitchenHolder);
		}
		break;
	case 0xa0005ea0: // Tag: the roll
		if (getFlag(kKitchenHolder) == kNothingInside) {
			sayRootMessage(kVerbTake);
		} else {
			takeObject(getFlag(kKitchenHolder));
			setFlag(kKitchenHolder, kNothingInside);
			refreshObject(kKitchenHolder);
			updateHolderName(kKitchenHolder);
		}
		break;

	// Egg tray 155 (+0x2f closed, +0x30 the egg in it)
	case 0xa0006d38: // Benyt med the egg
		if (second != kEgg)
			sayRootMessage(kVerbUse);
		else if (getFlag(kEggTray))
			say(handlerText(0xa0006d0f), 14, true);
		else if (getField(kEggTray, 0x30))
			say(handlerText(0xa0006d25), 14, true);
		else {
			setField(kEggTray, 0x30, 1);
			removeObject(kEgg);
			refreshEggTray();
		}
		break;
	case 0xa0006ea7: // Tag: the egg
		if (getFlag(kEggTray) || !getField(kEggTray, 0x30)) {
			sayRootMessage(kVerbTake);
		} else {
			takeObject(kEgg);
			setField(kEggTray, 0x30, 0);
			refreshEggTray();
		}
		break;
	case 0xa0006df9: // Åbn
		if (!getFlag(kEggTray)) {
			say(handlerText(0xa0006de3), 14, true);
		} else {
			setFlag(kEggTray, 0);
			refreshEggTray();
		}
		break;
	case 0xa0006e5c: // Luk
		if (getFlag(kEggTray)) {
			say(handlerText(0xa0006e44), 14, true);
		} else {
			setFlag(kEggTray, 1);
			refreshEggTray();
		}
		break;

	// Waste bag 170 (+0x2f emptied): Tag gives the waste items
	case 0xa00073a6: {
		if (getFlag(kWasteBag)) {
			say(handlerText(0xa000739a), 14, true);
			break;
		}
		static const int kBagItems[] = { 159, 161, 162, 163, 164, 165, 166, 167, 168, 169 };
		for (uint i = 0; i < ARRAYSIZE(kBagItems); ++i)
			takeObject(kBagItems[i]);
		setFlag(kWasteBag, 1);
		break;
	}
	case 0xa00074cf: // Se på
		say(handlerText(getFlag(kWasteBag) ? 0xa00074aa : 0xa00074b6), 14, true);
		break;

	// Oven 175: +0x2f closed, +0x30 on, +0x31 what is in it (100 nothing), +0x33 timer
	case 0xa00084e3: // Benyt med the chicken or the flute
		if (getFlag(kOven)) {
			say(handlerText(0xa00084bf), 14, true);
		} else if (getField(kOven, 0x31) != kNothingInside) {
			say(handlerText(0xa00084d0), 14, true);
		} else if (second == kChicken || second == kFlute) {
			setField(kOven, 0x31, second);
			removeObject(second);
			setFlag(second == kChicken ? kChickenInOven : kFluteInOven, 1);
			refreshObject(second == kChicken ? kChickenInOven : kFluteInOven);
			refreshObject(kOven);
		} else {
			sayRootMessage(kVerbUse);
		}
		break;
	case 0xa000860e: // Åbn: not while a cupboard door is open (VMT 0x60 = a000:8478)
		if (getFlag(kSinkCupboard) || getFlag(kOvenCupboard)) {
			say(handlerText(0xa00085db), 14, true);
		} else if (!getFlag(kOven)) {
			say(handlerText(0xa00085f8), 14, true);
		} else {
			setFlag(kOven, 0);
			refreshObject(kOven);
			refreshObject(kChickenInOven);
			refreshObject(kFluteInOven);
			playSound(kSoundOpen);
		}
		break;
	case 0xa00086c7: // Luk
		if (getFlag(kOven)) {
			say(handlerText(0xa00086af), 14, true);
		} else {
			setFlag(kOven, 1);
			refreshObject(kOven);
			refreshObject(kChickenInOven);
			refreshObject(kFluteInOven);
			playSound(kSoundOpen);
		}
		break;
	case 0xa00087d2: // Tænd
		if (!getFlag(kOven))
			say(handlerText(0xa00087c3), 14, true);
		else if (getField(kOven, 0x31) == kNothingInside)
			say(handlerText(0xa000877e), 14, true);
		else if (getField(kOven, 0x31) == kChicken)
			say(handlerText(0xa000878c), 14, true);
		else if (getFlag(kFlute))
			say(handlerText(0xa00087a9), 14, true);
		else {
			setField(kOven, 0x30, 1);
			refreshObject(kOven);
			setField(kOven, 0x33, kOvenTicks);
			setInputOff(true);
		}
		break;
	case 0xa0008ad7: { // Se på
		uint32 text = 0xa0008a80;
		if (!getFlag(kOven)) {
			int inside = getField(kOven, 0x31);
			text = inside == kChicken ? 0xa0008a9b : (inside == kFlute ? 0xa0008aba : 0xa0008a8f);
		}
		say(handlerText(text), 14, true);
		break;
	}

	// Toaster 204: +0x2f toasting, +0x30 done, +0x31 the flute in it
	case 0xa000b6f5: // Benyt med the flute
		if (second != kFlute) {
			sayRootMessage(kVerbUse);
		} else if (getFlag(kFlute)) {
			say(handlerText(0xa000b6de), 14, true);
		} else {
			setField(kToaster, 0x31, 1);
			removeObject(kFlute);
			refreshObject(kToaster);
		}
		break;
	case 0xa000b779: // Tænd
		if (!getField(kToaster, 0x31)) {
			say(handlerText(0xa000b76d), 14, true);
		} else {
			setInputOff(true);
			setFlag(kToaster, 1);
			refreshObject(kToaster);
			_toasterTimer = kToasterFrames;
		}
		break;
	case 0xa000b8fb: // Se på
		say(handlerText(getField(kToaster, 0x31) ? 0xa000b8d6 : 0xa000b8ef), 14, true);
		break;

	// Stove 207 and saucepan 184: +0x2f lid on, +0x30 eggs, +0x31 on, +0x32 water (dl)
	case 0xa0009738: // stove Se på
		say(handlerText(getField(kSaucepan, 0x31) ? 0xa000971a : 0xa0009728), 14, true);
		break;
	case 0xa0009706: // stove Tænd -> saucepan VMT 0x6c (a000:9ff2)
		if (getField(kSaucepan, 0x31))
			say(handlerText(0xa0009fb2), 14, true);
		else if (!getField(kSaucepan, 0x30))
			say(handlerText(0xa0009fc9), 14, true);
		else if (!getField(kSaucepan, 0x32))
			say(handlerText(0xa0009fdd), 14, true);
		else
			cookEggs();
		break;
	case 0xa00096f2: // stove Sluk -> saucepan VMT 0x68 (a000:a0d3)
		say(handlerText(getField(kSaucepan, 0x31) ? 0xa000a0a2 : 0xa000a0ba), 14, true);
		break;
	case 0xa000983e: { // saucepan Se på: "Der er" [" 6 æg"] [" og"] [" vand"] " i."
		bool eggs = getField(kSaucepan, 0x30) != 0, water = getField(kSaucepan, 0x32) > 0;
		if (!eggs && !water) {
			say(handlerText(0xa0009817), 14, true);
			break;
		}
		Common::String text = handlerText(0xa0009833);
		if (eggs)
			text += handlerText(0xa0009823);
		if (eggs && water)
			text += handlerText(0xa000982f);
		if (water)
			text += handlerText(0xa0009829);
		say(text + handlerText(0xa000983a), 14, true);
		break;
	}
	case 0xa0009e73: // saucepan Benyt med the lid, the egg or the tap
		if (getField(kSaucepan, 0x31)) {
			say(handlerText(0xa0009e25), 14, true);
		} else if (second == kLid) {
			setFlag(kSaucepan, 1);
			removeObject(kLid);
			setFlag(kLidOnSaucepan, 1);
			refreshObject(kLidOnSaucepan);
			refreshObject(kSaucepan);
		} else if (getFlag(kSaucepan)) {
			say(handlerText(0xa0009e42), 14, true);
		} else if (second == kEgg) {
			setField(kSaucepan, 0x30, 1);
			removeObject(kEgg);
		} else if (second == kKitchenTap) {
			if (!getField(kSaucepan, 0x30)) {
				say(handlerText(0xa0009e60), 14, true);
			} else {
				// close-up 25 (8af9:06ac): the saucepan's +0x32 := the water
				int water = runEggWater();
				if (water > 0)
					setField(kSaucepan, 0x32, water);
				redrawRoom();
			}
		} else {
			sayRootMessage(kVerbUse);
		}
		break;

	// Items whose Benyt calls the container's Benyt (VMT 0x30) with themselves
	case 0xa0008bd5: // chicken: the fridge, else the waste handler (bin 215)
		if (second == kFridge) {
			executeVerb(kVerbUse, kFridge, object);
			return true;
		}
		return runClassHandler(0xa00004ee, verb, object, second);
	case 0xa0008243: // flute: the oven or the toaster
	case 0xa0007032: // egg: the egg tray or the saucepan
	case 0xa000a15a: // lid: the saucepan
		if ((handler == 0xa0008243 && (second == kOven || second == kToaster)) ||
				(handler == 0xa0007032 && (second == kEggTray || second == kSaucepan)) ||
				(handler == 0xa000a15a && second == kSaucepan))
			executeVerb(kVerbUse, second, object);
		else
			sayRootMessage(kVerbUse);
		break;

	default:
		return false;
	}
	return true;
}

} // End of namespace Miljostrup
