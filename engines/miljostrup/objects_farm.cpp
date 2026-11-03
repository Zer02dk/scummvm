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

// The farm (location 3), written from notes/farm.md. So far the shed (the
// piles of weedkiller cans and fertiliser sacks, the feed) and the potato
// cellar (the light, the potatoes that sprout, the seed potatoes).

#include "common/events.h"
#include "common/system.h"
#include "graphics/framelimiter.h"

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/handlers_data.h"

namespace Miljostrup {

enum {
	kFarm = 3,
	kCellar = 8,
	kCow = 109,
	kTrough = 111,
	kPlanter = 118,        // sætter, DS:c770: +0x34 filled, +0x35 organic
	kSpreader = 119,       // spreder, DS:c774: +0x34 kind (3 empty, 2 fertiliser), +0x35 amount
	kSprayer = 125,        // giftspreder, DS:c778: +0x34 hectares of weedkiller
	kContainer = 114,      // DS:c764, +0x2f dung in it
	kOrganicDung = 112,    // DS:c768, +0x2f there (start 0)
	kHarrow = 117,         // the machines (2b4f): +0x2f work pending, +0x30 the field
	kBurner = 122,
	kPlank = 123,
	kHarvester = 124,      // +0x34 / +0x35 the first harvest's note (start +0x35 = 1)
	kSprinklerA = 120,     // +0x34 set on a field
	kSprinklerB = 121,
	kSprayA = 126,         // the sprinklers' spray over field 115 / 116 (DS:c788 / c78c)
	kSprayB = 127,
	kYard = 5,
	kCellarSwitch = 143,   // DS:c7e0, +0x2f light on (start off)
	kOrganicPotatoes = 144, // DS:c7e8, the shelf's potatoes: +0x2f sprouted,
	kPotatoes = 145,       // +0x30 taken, +0x31 light seen, +0x32 the seed potatoes
	kOrganicFeed = 158,
	kGate = 110,           // staldport, DS:c74c, +0x2f open (start closed)
	kFieldA = 115,         // the fields, +0x2f stage (115: 1, 116: 0)
	kFieldB = 116,
	kWagon = 136,          // ladvogn, +0x2f its load (0 none, 1 boxes, 2 crates, 3 potatoes)
	kBagMachine = 130,     // +0x2f running (shown)
	kBagPotatoes = 131,    // potatoes for the bags, +0x2f there, +0x30 harvested once
	kLoosePotatoes = 133,  // for the factory
	kWastePotatoes = 134,
	kRoom9Container = 135,
	kVan = 137,
	kPlasticCrates = 138,
	kCardboardBoxes = 139,
	kRoom9Field = 140,
	kMeadow = 141,
	kBelt1 = 901,          // room 9's figures without an object (rooms.FIGURE_IDS)
	kBelt2 = 902,
	kBelt3 = 903,
	kFallingBag = 904,     // 7554:0b89, +0x732
	kBagHolder = 129,      // DS:c7a8, +0x2f the roll on it (0 none, 1 PE, 2 PVC, 3 paper)
	kMachineBags = 132,    // +0x2f (word) bags in the machine

	kFertiliserKind = 2,
	kSpreaderEmpty = 3
};

// 784c:0782: an item of a pile keeps its count in +0x35
static const int kItemCount = 0x35;
// 784c:0911: a pile keeps its count in +0x31
static const int kPileCount = 0x31;

static const FarmPile *findPile(int object) {
	for (uint i = 0; i < ARRAYSIZE(kFarmPiles); ++i) {
		if (kFarmPiles[i].id == object || kFarmPiles[i].item == object)
			return &kFarmPiles[i];
	}
	return nullptr;
}

static int pileForm(int count) {
	return count == 0 ? 0 : (count == 1 ? 1 : 2);
}

void MiljostrupEngine::initFarm() {
	for (uint i = 0; i < ARRAYSIZE(kFarmPiles); ++i)
		updatePileName(kFarmPiles[i].id);
	updateFieldName(kFieldA);
	updateFieldName(kFieldB);
}

void MiljostrupEngine::updateFieldName(int field) {
	// 784c:5751: GetName "eng" at stage 0, else the root's name ("mark")
	if (getFlag(field) == 0)
		_interface->setNameOverride(field, handlerText(0x784c574d));
	else
		_interface->removeNameOverride(field);
}

void MiljostrupEngine::updatePileName(int pile) {
	// 784c:0aa3: GetName by the count: the root's name (the floor, the
	// shelves), the singular or the plural
	const FarmPile *p = findPile(pile);
	if (p)
		_interface->setNameOverride(pile, readPascalString(_ovr, p->texts[3 * pileForm(getField(pile, kPileCount)) + 1]));
}

void MiljostrupEngine::setItemCount(int item, int count) {
	// 784c:08c2: the item leaves the inventory at 0 (room VMT 0x54) and comes
	// into it from 0 (room VMT 0x50)
	int old = getField(item, kItemCount);
	if (count == old)
		return;
	if (count == 0)
		removeObject(item);
	else if (old == 0)
		takeObject(item);
	setField(item, kItemCount, count);
}

void MiljostrupEngine::setPileCount(int pile, int count) {
	// 784c:0cc7: the count, then the figure rule
	setField(pile, kPileCount, count);
	updatePileName(pile);
	refreshObject(pile);
}

void MiljostrupEngine::updatePotatoes(int object) {
	// 784c:16f3, the potatoes' figure rule: they have sprouted once the light
	// was on at an earlier run (+0x31, the switch when the rule last ran)
	setFlag(object, getFlag(object) || getField(object, 0x31) ? 1 : 0);
	setField(object, 0x31, getFlag(kCellarSwitch) ? 1 : 0);
}

void MiljostrupEngine::farmBuilt() {
	// The room's bindings run their figure rules: the cellar's potatoes, and
	// in the yard the machines' 784c:2bbb: work pending -> cleared, VMT 0x5c
	// (the machine's award)
	if (_location != kFarm)
		return;
	if (_room == kCellar) {
		updatePotatoes(kOrganicPotatoes);
		updatePotatoes(kPotatoes);
	}
	if (_room == kYard) {
		static const int kMachines[7] = { kHarrow, kPlanter, kSpreader, kBurner, kPlank, kHarvester, kSprayer };
		for (int i = 0; i < 7; ++i) {
			if (getFlag(kMachines[i])) {
				setFlag(kMachines[i], 0);
				machineAward(kMachines[i]);
			}
		}
	}
}

bool MiljostrupEngine::isFarmObjectEnabled(int object, bool &enabled) const {
	// 784c:16f3: the potatoes' region is there until they are taken
	if (object == kOrganicPotatoes || object == kPotatoes) {
		enabled = getField(object, 0x30) == 0;
		return true;
	}
	// the yard: the cow at the gate while it is open (784c:25d8), the dung
	// heaps while there (298d), the sprinklers until set on a field (445d)
	if (object == kCow) {
		enabled = getFlag(kGate) != 0;
		return true;
	}
	if (object == 112 || object == 113) {
		enabled = getFlag(object) != 0;
		return true;
	}
	if (object == 120 || object == 121) {
		enabled = getField(object, 0x34) == 0;
		return true;
	}
	// room 9: the potatoes while they lie there (784c:6378 / 6fe9 / 74d1),
	// the bags while there are some (690c), the crates until loaded (60e4)
	if (object == 131 || object == 133 || object == 134 || object == kMachineBags) {
		enabled = getFlag(object) > 0;
		return true;
	}
	if (object == 138 || object == 139) {
		enabled = getFlag(object) == 0;
		return true;
	}
	return false;
}

int MiljostrupEngine::getFarmDimFactor() const {
	// 784c:15a9, the switch's figure rule: dim (2472:057e) while the light is
	// off; the cellar inserts the interval (16, 15, 0, 3, 3) with factor 10
	// (750d:0372 / 0384)
	if (_location != kFarm || _room != kCellar || getFlag(kCellarSwitch))
		return 0;
	return 10;
}

int MiljostrupEngine::getFarmFigureState(uint32 rule, int object, bool &visible) const {
	switch (rule) {
	case 0x784c0c42: { // the piles: hidden when empty, else the count
		int count = getField(object, kPileCount);
		visible = count != 0;
		return count ? count : 1;
	}
	case 0x784c16f3: // the cellar's potatoes: on the shelf 1, taken 2
		return getField(object, 0x30) ? 2 : 1;
	// The yard (room 5)
	case 0x784c25d8: // the cow at the gate: out while the gate is open; fed (the trough) 2
		visible = getFlag(kGate) != 0;
		return getFlag(kTrough) ? 2 : 1;
	case 0x784c23c6: // the cows on the meadow 106..108: out while the gate is open
		visible = getFlag(kGate) != 0;
		return 1;
	case 0x784c212e: // the open gate, the feed in the trough, the dung heaps, the
	case 0x784c22d0: // dung in the container: shown while +0x2f is set
	case 0x784c298d:
	case 0x784c2afb:
		visible = getFlag(object) != 0;
		return 1;
	case 0x784c5947: { // the fields by their stage
		static const int kStates[7] = { 2, 1, 1, 1, 3, 4, 1 };
		int stage = getFlag(object);
		return stage >= 0 && stage <= 6 ? kStates[stage] : 1;
	}
	case 0x784c445d: // the field sprinklers: in the yard until set on a field (+0x34)
		visible = getField(object, 0x34) == 0;
		return 1;
	case 0x784c4060: // their spray over field 115 / 116 while +0x34 is set at stage 4
		visible = getField(object, 0x34) != 0 && getFlag(object == 126 ? kFieldA : kFieldB) == 4;
		return 1;

	// Room 9 (all start with +0x2f = 0)
	case 0x784c5baa: // the bag holder: shown with a roll on it (1..3), state = the roll
		visible = getFlag(object) != 0;
		return getFlag(object) ? getFlag(object) : 1;
	case 0x784c5d2b: // the machine; the potatoes 131 / 133 / 134: shown when +0x2f is set
	case 0x784c6378:
	case 0x784c6fe9:
	case 0x784c74d1:
		visible = getFlag(object) != 0;
		return 1;
	case 0x784c690c: { // the bags in the machine: the count, from 5 / 9 by the holder's roll
		int count = getFlag(object);
		int roll = getFlag(kBagHolder);
		visible = count > 0;
		return count ? (roll == 2 ? 4 : (roll == 3 ? 8 : 0)) + count : 1;
	}
	case 0x784c5f2e: // the wagon's load: none (hidden), boxes 1, crates 2, potatoes 3
		visible = getFlag(object) != 0;
		return getFlag(object) ? getFlag(object) : 1;
	case 0x784c60e4: // the crates: on the ground until they are loaded
		visible = getFlag(object) == 0;
		return 1;
	default:
		return 0;
	}
}

void MiljostrupEngine::giveFarmItems() {
	// Testing aid (all_items): the piles' contents into their items and the
	// cellar's potatoes taken
	for (uint i = 0; i < ARRAYSIZE(kFarmPiles); ++i) {
		setField(kFarmPiles[i].item, kItemCount, getField(kFarmPiles[i].id, kPileCount));
		setPileCount(kFarmPiles[i].id, 0);
	}
	for (int object = kOrganicPotatoes; object <= kPotatoes; ++object) {
		setField(object, 0x30, 1);
		refreshObject(object);
	}
}

bool MiljostrupEngine::runFarmHandler(uint32 handler, int verb, int object, int second) {
	if (_location != kFarm)
		return false;
	switch (handler) {
	// The piles 161 / 163 (VMT 4a9a)
	case 0x784c0b7f: { // Se på: the text by the count
		const FarmPile *p = findPile(object);
		say(readPascalString(_ovr, p->texts[3 * pileForm(getField(object, kPileCount)) + 2]), 14, true);
		break;
	}
	case 0x784c0bf5: { // Tag: one of them into the item
		const FarmPile *p = findPile(object);
		int count = getField(object, kPileCount);
		if (!count) {
			sayRootMessage(kVerbTake);
			break;
		}
		setPileCount(object, count - 1);
		setItemCount(p->item, getField(p->item, kItemCount) + 1);
		break;
	}

	// The items: weedkiller 162 (VMT 4afa), fertiliser 160 (VMT 4b5a)
	case 0x784c0df7: // Se på: "Det er ukrudtsgift til " + n + " hektar."
		say(handlerText(0x784c0dd6) + Common::String::format("%d", getField(object, kItemCount)) +
			handlerText(0x784c0dee), 14, true);
		break;
	case 0x784c101d: // "Det er kunstgødning til " + n + " hektar."
		say(handlerText(0x784c0ffb) + Common::String::format("%d", getField(object, kItemCount)) +
			handlerText(0x784c1014), 14, true);
		break;
	case 0x784c0ecf: // Benyt with the sprayer: all of it in
		if (second == kSprayer) {
			if (getField(kSprayer, 0x34) > 0) {
				say(handlerText(0x784c0ea2), 14, true);
			} else {
				setField(kSprayer, 0x34, getField(object, kItemCount));
				setItemCount(object, 0);
				say(handlerText(0x784c0ec8), 14, true);
			}
		} else {
			usePileItem(object, second);
		}
		break;
	case 0x784c10ec: // Benyt with the spreader: all of it in
		if (second == kSpreader) {
			if (getField(kSpreader, 0x34) != kSpreaderEmpty) {
				say(handlerText(0x784c10c8), 14, true);
			} else {
				setField(kSpreader, 0x34, kFertiliserKind);
				setField(kSpreader, 0x35, getField(object, kItemCount));
				setItemCount(object, 0);
				say(handlerText(0x784c10e5), 14, true);
			}
		} else {
			usePileItem(object, second);
		}
		break;

	// The feed 156 / 158 (VMT 497a)
	case 0x784c0310: // Se på
		say(handlerText(object == kOrganicFeed ? 0x784c02c7 : 0x784c02ea), 14, true);
		break;

	case 0x784c06d1: // Benyt: with the trough 784c:04f1, with a cow a hint
		if (second == kTrough)
			feedCows(object);
		else if (second == kCow)
			say(handlerText(0x784c06ba), 14, true);
		else
			sayRootMessage(kVerbUse);
		break;
	case 0x784c073c: // Giv to a cow: a hint
		if (second == kCow)
			say(handlerText(0x784c0725), 14, true);
		else
			sayRootMessage(kVerbGive);
		break;

	// The cellar's switch 143 (VMT 4c7a)
	case 0x784c120c: // Se på
		say(handlerText(getFlag(object) ? 0x784c11ea : 0x784c11fa), 14, true);
		break;
	case 0x784c1380: // Tænd
		if (getFlag(object)) {
			say(handlerText(0x784c126e), 14, true);
			break;
		}
		setFlag(object, 1);
		playSound(kSoundClick);
		setRoomPalette(_scene->getRoom()->palette);
		if (getFlag(kOrganicPotatoes))
			award(-5, 0, -5, 0, 0x784c1287, 0);
		else
			award(10, 0, 10, 0, 0x784c12d4, 0);
		break;
	case 0x784c14dc: // Sluk
		if (!getFlag(object)) {
			say(handlerText(0x784c14c1), 14, true);
		} else if (!getFlag(kOrganicPotatoes)) {
			say(handlerText(0x784c149a), 14, true);
		} else {
			setFlag(object, 0);
			playSound(kSoundClick);
			setRoomPalette(_scene->getRoom()->palette);
			award(5, 5, 0, 0, 0x784c1453, 0);
		}
		break;

	// The shelf's potatoes 144 / 145 (VMT 4cda / 4d3a)
	case 0x784c167c: // Tag: only once they have sprouted
		if (!getFlag(object)) {
			say(handlerText(0x784c1662), 14, true);
			break;
		}
		setField(object, 0x30, 1);
		takeObject(object == kOrganicPotatoes ? 146 : 147);
		updatePotatoes(object);
		refreshObject(object);
		break;
	case 0x784c1831: // Se på
		say(handlerText(getFlag(object) ? 0x784c17e3 : 0x784c180b), 14, true);
		break;
	case 0x784c1935:
		say(handlerText(getFlag(object) ? 0x784c18ed : 0x784c190f), 14, true);
		break;

	// The seed potatoes 146 / 147 (VMT 4c1a / 4bba): Benyt with the planter
	case 0x784c1c85:
	case 0x784c1ac1:
		if (second != kPlanter) {
			sayRootMessage(kVerbUse);
		} else if (getField(kPlanter, 0x34)) {
			say(handlerText(handler == 0x784c1c85 ? 0x784c1c58 : 0x784c1a94), 14, true);
		} else {
			removeObject(object);
			setField(kPlanter, 0x34, 1);
			setField(kPlanter, 0x35, handler == 0x784c1c85 ? 1 : 0);
			say(handlerText(handler == 0x784c1c85 ? 0x784c1c7e : 0x784c1aba), 14, true);
		}
		break;

	// The trough 111 (VMT 4eba): Se på
	case 0x784c2240:
		if (!getFlag(object))
			say(handlerText(0x784c2224), 14, true);
		else
			say(handlerText(getField(object, 0x30) ? 0x784c21d6 : 0x784c21fc), 14, true);
		break;

	// The dung heaps 112 organic / 113 ordinary (VMT 4a3a)
	case 0x784c2717: // Se på
		say(handlerText(object == kOrganicDung ? 0x784c26d2 : 0x784c26f3), 14, true);
		break;
	case 0x784c295e: // Benyt with the spreader (28a9) or the container (2808)
		if (second == kSpreader) {
			if (getField(kSpreader, 0x34) != kSpreaderEmpty) {
				say(handlerText(0x784c2886), 14, true);
			} else {
				setFlag(object, 0);
				refreshObject(object);
				setField(kSpreader, 0x34, object == kOrganicDung ? 0 : 1);
				setField(kSpreader, 0x35, 1);
				say(handlerText(0x784c28a2), 14, true);
			}
		} else if (second == kContainer) {
			setFlag(object, 0);
			refreshObject(object);
			setFlag(kContainer, 1);
			refreshObject(kContainer);
			award(-10, 0, -10, 0, 0x784c2779, 0);
		} else {
			sayRootMessage(kVerbUse);
		}
		break;

	// The container 114 (VMT 49da): Se på
	case 0x784c2a99:
		say(handlerText(getFlag(object) ? 0x784c2a65 : 0x784c2a7d), 14, true);
		break;

	// The spreader 119 (VMT 503a): Se på by what is in it
	case 0x784c35ba:
		switch (getField(object, 0x34)) {
		case 0:
			say(handlerText(0x784c352f), 14, true);
			break;
		case 1:
			say(handlerText(0x784c3551), 14, true);
			break;
		case kFertiliserKind:
			say(handlerText(0x784c3575) + Common::String::format("%d", getField(object, 0x35)) +
				handlerText(0x784c3596), 14, true);
			break;
		default:
			say(handlerText(0x784c359b), 14, true);
			break;
		}
		break;

	// The machines' Benyt with a field (0x73 = 115, 0x74 = 116)
	case 0x784c2e18: // harrow: the meadow, while the cows are inside (2d69)
		if (second != kFieldA && second != kFieldB) {
			sayRootMessage(kVerbUse);
		} else if (getFlag(second) < 1) {
			if (getFlag(kGate))
				say(handlerText(0x784c2d42), 14, true);
			else
				workField(object, second, 0, 1);
		} else {
			say(handlerText(getFlag(second) == 6 ? 0x784c2ddd : 0x784c2e00), 14, true);
		}
		break;
	case 0x784c326b: // planter (3154): with seed potatoes in it, on a harrowed field
		if (second == kFieldA || second == kFieldB) {
			if (!getField(object, 0x34))
				say(handlerText(0x784c3223), 14, true);
			else if (getFlag(second) < 1)
				say(handlerText(0x784c3120), 14, true);
			else if (getFlag(second) < 2)
				workField(object, second, 1, 2);
			else
				say(handlerText(0x784c3135), 14, true);
		} else if (second == 146 || second == 147) {
			executeVerb(kVerbUse, second, object);
		} else {
			say(handlerText(0x784c3246), 14, true);
		}
		break;
	case 0x784c37fc: // spreader (36c9): with something in it, on a planted field
		if (second == kFieldA || second == kFieldB) {
			if (getField(object, 0x34) == kSpreaderEmpty)
				say(handlerText(0x784c37bc), 14, true);
			else if (getFlag(second) < 2)
				say(handlerText(0x784c368d), 14, true);
			else if (getFlag(second) < 3)
				workField(object, second, 2, 3);
			else
				say(handlerText(0x784c36ae), 14, true);
		} else if (second == kOrganicDung || second == 113 || second == 160) {
			executeVerb(kVerbUse, second, object);
		} else {
			say(handlerText(0x784c37de), 14, true);
		}
		break;
	case 0x784c4392: // field sprinklers (4272): on a fertilised field
		if (second != kFieldA && second != kFieldB)
			sayRootMessage(kVerbUse);
		else if (getFlag(second) < 3)
			say(handlerText(0x784c4350), 14, true);
		else if (getFlag(second) < 4)
			waterField(object, second);
		else
			say(handlerText(0x784c4369), 14, true);
		break;
	case 0x784c46a9: // sprayer (4621): with weedkiller, on a field with weeds
		if (second == kFieldA || second == kFieldB) {
			if (getField(object, 0x34) < 1)
				say(handlerText(0x784c4676), 14, true);
			else if (getFlag(second) == 4)
				workField(object, second, 5, 5);
			else
				say(handlerText(0x784c4657), 14, true);
		} else if (second == 162) {
			executeVerb(kVerbUse, second, object);
		} else {
			say(handlerText(0x784c468c), 14, true);
		}
		break;
	case 0x784c4c49: // burner (4bf4)
	case 0x784c5088: // plank (5033)
		if (second != kFieldA && second != kFieldB)
			sayRootMessage(kVerbUse);
		else if (getFlag(second) == 4)
			workField(object, second, handler == 0x784c4c49 ? 3 : 4, 5);
		else
			say(handlerText(handler == 0x784c4c49 ? 0x784c4c2a : 0x784c5069), 14, true);
		break;
	case 0x784c53dc: // harvester (5327): the potatoes appear in room 9
		if (second != kFieldA && second != kFieldB) {
			sayRootMessage(kVerbUse);
		} else if (getFlag(second) < 2) {
			say(handlerText(0x784c537b), 14, true);
		} else if (getFlag(second) < 5) {
			say(handlerText(0x784c539e), 14, true);
		} else if (getFlag(second) < 6) {
			// 63d9 / 704a / 7532: the potatoes in room 9, once
			static const int kHarvested[3] = { 131, 133, 134 };
			for (int i = 0; i < 3; ++i) {
				if (!getField(kHarvested[i], 0x30)) {
					setFlag(kHarvested[i], 1);
					setField(kHarvested[i], 0x30, 1);
				}
			}
			workField(object, second, 6, 6);
		} else {
			say(handlerText(0x784c53c1), 14, true);
		}
		break;

	// The planter's and the sprayer's Se på
	case 0x784c3090:
		if (!getField(object, 0x34))
			say(handlerText(0x784c3071), 14, true);
		else
			say(handlerText(getField(object, 0x35) ? 0x784c3022 : 0x784c3049), 14, true);
		break;
	case 0x784c458b:
		if (getField(object, 0x34) < 1)
			say(handlerText(0x784c453b), 14, true);
		else
			say(handlerText(0x784c4562) + Common::String::format("%d", getField(object, 0x34)) +
				handlerText(0x784c4582), 14, true);
		break;

	// Room 9: the bags 150 / 152 / 154 (VMT 491a) Benyt with the holder or
	// the machine: a roll on the holder (784c:0164)
	case 0x784c0164:
		if (second != kBagHolder && second != kBagMachine) {
			sayRootMessage(kVerbUse);
		} else if (getFlag(kBagHolder)) {
			say(handlerText(0x784c0142), 14, true);
		} else {
			setFlag(kBagHolder, object == 150 ? 1 : (object == 152 ? 2 : 3));
			refreshObject(kBagHolder);
			removeObject(object);
		}
		break;
	case 0x784c5b14: { // the holder's Se på by its roll
		static const uint32 kTexts[4] = { 0x784c5a91, 0x784c5aad, 0x784c5ace, 0x784c5af0 };
		say(handlerText(kTexts[CLIP(getFlag(object), 0, 3)]), 14, true);
		break;
	}
	case 0x784c62a6: // the potatoes for the bags: into the machine
		if (second == kBagMachine) {
			if (!getFlag(kBagHolder))
				say(handlerText(0x784c6241), 14, true);
			else
				startBagging();
		} else if (second == kVan || second == kPlasticCrates || second == kCardboardBoxes) {
			say(handlerText(0x784c6260), 14, true);
		} else if (second == kWagon) {
			say(handlerText(0x784c6280), 14, true);
		} else {
			sayRootMessage(kVerbUse);
		}
		break;
	case 0x784c6817: // the bags of potatoes: to the supermarket
		if (second == kCardboardBoxes) {
			room9Award(-10, 0x784c6521, object);
			// "Hvad er sodavandskasser lavet af?" Pap / Plast: 0, category 2
			setQuiz(0x784c65f2, 0x784c6614, 0x784c6618, 0, 2);
			loadCrate(kCardboardBoxes, object);
		} else if (second == kPlasticCrates) {
			room9Award(10, 0x784c661e, object);
			loadCrate(kPlasticCrates, object);
		} else if (second == kVan) {
			room9Award(0, 0x784c6701, object);
		} else {
			say(handlerText(0x784c6801), 14, true);
		}
		break;
	case 0x784c6e2a: // the loose potatoes: to the factory
		if (second == kCardboardBoxes) {
			room9Award(-10, 0x784c6b34, object);
			// "Kan madpapir anvendes til genbrugspapir?" Ja / Nej: 0, category 2
			setQuiz(0x784c6c1a, 0x784c6c43, 0x784c6c46, 0, 2);
			loadCrate(kCardboardBoxes, object);
		} else if (second == kPlasticCrates) {
			award(-6, -3, 0, -3, 0x784c6c4a, 0);
			setFlag(object, 0);
			refreshObject(object);
			// "Kan man genanvende plast?" Ja / Nej: 1, category 1
			setQuiz(0x784c6d24, 0x784c6c43, 0x784c6c46, 1, 1);
			loadCrate(kPlasticCrates, object);
		} else if (second == kWagon) {
			room9Award(10, 0x784c6d3e, object);
			setFlag(kWagon, 3);
			refreshObject(kWagon);
		} else if (second == kVan) {
			say(handlerText(0x784c6df4), 14, true);
		} else {
			say(handlerText(0x784c6e14), 14, true);
		}
		break;
	case 0x784c73fd: // the waste potatoes
		if (second == kRoom9Container) {
			room9Award(-10, 0x784c718a, object);
			// "Må kartofler lægges i kompostbunken?" Ja / Nej: 1, category 2
			setQuiz(0x784c723b, 0x784c7260, 0x784c7263, 1, 2);
		} else if (second == kRoom9Field) {
			room9Award(6, 0x784c7267, object);
		} else if (second == kMeadow) {
			room9Award(10, 0x784c72ff, object);
		} else {
			say(handlerText(0x784c73e7), 14, true);
		}
		break;

	// The fields 115 / 116 (VMT 4d9a): Se på by the stage
	case 0x784c5861: {
		static const uint32 kTexts[7] = { 0x784c579b, 0x784c57b7, 0x784c57c9, 0x784c57ea,
			0x784c5808, 0x784c5824, 0x784c5848 };
		int stage = getFlag(object);
		if (stage >= 0 && stage <= 6)
			say(handlerText(kTexts[stage]), 14, true);
		break;
	}

	// The barn gate 110 (VMT 4dfa)
	case 0x784c1da2: // Se på
		say(handlerText(getFlag(object) ? 0x784c1d86 : 0x784c1d93), 14, true);
		break;
	case 0x784c1efa: // Åbn: only while the right field is still a meadow
		if (getFlag(object)) {
			say(handlerText(0x784c1e04), 14, true);
		} else if (getFlag(kFieldB) >= 1) {
			say(handlerText(0x784c1e1a), 14, true);
		} else {
			setFlag(object, 1);
			refreshObject(object);
			playSound(kSoundClose);
			award(9, 3, 3, 3, 0x784c1e3b, 0x784c1ef4);
		}
		break;
	case 0x784c2016: // Luk: it stays open
		say(handlerText(getFlag(object) ? 0x784c1fe0 : 0x784c1ffe), 14, true);
		break;
	case 0x784c20b3: // Benyt: go in
		say(handlerText(getFlag(object) ? 0x784c2078 : 0x784c20a1), 14, true);
		break;

	// The cow at the gate 109 (VMT 4f1a), by the trough (+0x2f fed, +0x30 organic)
	case 0x784c24ba: // Se på
		if (!getFlag(kTrough))
			say(handlerText(0x784c249f), 14, true);
		else
			say(handlerText(getField(kTrough, 0x30) ? 0x784c245e : 0x784c2482), 14, true);
		break;
	case 0x784c2572: // Tal til
		say(handlerText(getFlag(kTrough) ? 0x784c2555 : 0x784c2565), 14, true);
		break;

	// The wagon 136 in room 9 (VMT 539a): Se på by its load
	case 0x784c5e98: {
		static const uint32 kTexts[4] = { 0x784c5e15, 0x784c5e33, 0x784c5e53, 0x784c5e78 };
		int load = getFlag(object);
		if (load >= 0 && load <= 3)
			say(handlerText(kTexts[load]), 14, true);
		break;
	}

	// The crates 138 / 139 in room 9 (VMT 53fa)
	case 0x784c6081: // Se på by ID: 0x8b the cardboard boxes
		say(handlerText(object == 139 ? 0x784c6043 : 0x784c6062), 14, true);
		break;

	default:
		return false;
	}
	return true;
}

void MiljostrupEngine::workField(int machine, int field, int anim, int stage) {
	// 784c:2bda: the machine's work is pending (+0x2f) on the field (+0x30);
	// 2bf7: the field gets its new stage, GoTo(6, 0) shows the machine on the
	// field (unit 7407) and the yard is built again
	int other = field == kFieldA ? kFieldB : kFieldA;
	int oldStage = getFlag(field);
	setFlag(machine, 1);
	setField(machine, 0x30, field);
	setFlag(field, stage);
	updateFieldName(field);
	runFieldWork(anim, field == kFieldA, oldStage, stage, getFlag(other));
	redrawRoom();
}

void MiljostrupEngine::fieldAward(int points, int energy, int waste, int water, const Common::String &text,
		uint32 page) {
	awardText(points, energy, waste, water, text, page ? handlerText(page) : Common::String());
}

void MiljostrupEngine::machineAward(int machine) {
	// VMT 0x5c of each machine; +0x30 the field it worked
	int field = getField(machine, 0x30);
	bool organic = getField(field, 0x30) != 0;
	switch (machine) {
	case kHarrow: // 784c:2f70
		award(0, 0, 0, 0, 0x784c2ee3, 0);
		break;
	case kPlanter: // 3433: ordinary seed potatoes end the field's being organic
		if (getField(kPlanter, 0x35)) {
			award(9, 3, 3, 3, 0x784c333d, 0);
		} else {
			award(0, 0, 0, 0, 0x784c33a8, 0);
			setField(field, 0x30, 0);
		}
		setField(kPlanter, 0x34, 0);
		break;
	case kSpreader: { // 3fae by what was in it
		int kind = getField(kSpreader, 0x34);
		if (kind == 0) { // 39eb: organic dung
			Common::String text = handlerText(0x784c38da);
			if (organic)
				text += handlerText(0x784c38fe) + handlerText(0x784c3910) + handlerText(0x784c393d);
			else
				text += handlerText(0x784c394e) + handlerText(0x784c3955) + handlerText(0x784c398a) +
					handlerText(0x784c39c3);
			fieldAward(9, 3, 3, 3, text, 0x784c39e5);
		} else if (kind == 1) { // 3c28: ordinary dung
			if (organic)
				setField(field, 0x30, 0);
			fieldAward(6, 2, 2, 2, handlerText(organic ? 0x784c3ae8 : 0x784c3b8e), 0x784c3c22);
		} else if (kind == kFertiliserKind) { // 3e84: by the amount
			Common::String suffix;
			if (organic) {
				suffix = handlerText(0x784c3cc4);
				setField(field, 0x30, 0);
			}
			int amount = getField(kSpreader, 0x35);
			if (amount == 1)
				fieldAward(0, 0, 0, 0, handlerText(0x784c3cec) + suffix, 0x784c3e7e);
			else if (amount == 2)
				fieldAward(-5, 0, 0, -5, handlerText(0x784c3d58) + suffix, 0x784c3e7e);
			else if (amount == 3)
				fieldAward(-10, 0, 0, -10, handlerText(0x784c3de9) + suffix, 0x784c3e7e);
		}
		setField(kSpreader, 0x34, kSpreaderEmpty);
		break;
	}
	case kSprayer: { // 49ea: by the amount, then the quiz
		Common::String suffix;
		if (organic) {
			suffix = handlerText(0x784c47dc);
			setField(field, 0x30, 0);
		}
		int amount = getField(kSprayer, 0x34);
		if (amount == 1)
			fieldAward(0, 0, 0, 0, handlerText(0x784c4804) + suffix, 0);
		else if (amount == 2)
			fieldAward(-5, 0, 0, -5, handlerText(0x784c4890) + suffix, 0);
		else
			fieldAward(-10, 0, 0, -10, handlerText(0x784c4923) + suffix, 0);
		// "Er den tomme giftdunk farligt affald?" Ja / Nej: 1, category 3
		setQuiz(0x784c49bd, 0x784c49e3, 0x784c49e6, 1, 3);
		setField(kSprayer, 0x34, 0);
		break;
	}
	case kBurner: // 4e36: the field stays organic; a quiz either way
		fieldAward(-5, -5, 0, 0, handlerText(0x784c4cd9) +
			(organic ? handlerText(0x784c4d1d) + handlerText(0x784c4d62) : handlerText(0x784c4db5)), 0);
		_award.quiz = true;
		if (organic) {
			// "Bliver drikkevandet aldrig undersøgt?" Jo / Nej: 1
			_award.question = handlerText(0x784c4d88);
			_award.answer1 = handlerText(0x784c4dae);
			_award.answer2 = handlerText(0x784c4db1);
			_award.correct = 1;
		} else {
			// "... CO2 kaldes?" Naturgas / Drivhusgas: 0
			_award.question = handlerText(0x784c4deb);
			_award.answer1 = handlerText(0x784c4e22);
			_award.answer2 = handlerText(0x784c4e2b);
			_award.correct = 0;
		}
		_award.category = 1;
		break;
	case kPlank: // 51e0
		fieldAward(9, 3, 3, 3, handlerText(0x784c5118) + (organic ? handlerText(0x784c51be) : Common::String()), 0);
		break;
	case kHarvester: { // 55eb: the first harvest says where the potatoes are
		Common::String suffix;
		if (getField(kHarvester, 0x35)) {
			setField(kHarvester, 0x34, 1);
			setField(kHarvester, 0x35, 0);
			suffix = handlerText(0x784c5507);
		}
		if (organic)
			fieldAward(9, 3, 3, 3, handlerText(0x784c5538) + suffix, 0);
		else
			fieldAward(0, 0, 0, 0, handlerText(0x784c5582) + suffix, 0);
		break;
	}
	default:
		break;
	}
}

void MiljostrupEngine::waterField(int sprinkler, int field) {
	// 784c:4272: the sprinkler goes onto the field (+0x34, hidden in the
	// yard), the field's stage 4; its spray (126 over 115, 127 over 116,
	// DS:c788 / c78c): +0x34 := 1, the rule 4060 shows it, Walk(1, 0, 0,
	// 25), message 0x1f, input off; +0x34 := 0 again (hidden at the next
	// rebuild). No room 6. The award follows the spray (tickFarm).
	setField(sprinkler, 0x34, 1);
	refreshObject(sprinkler);
	// the field's figure changes at the room's rebuild after the award
	setFlag(field, 4);
	updateFieldName(field);
	int spray = field == kFieldA ? kSprayA : kSprayB;
	setField(spray, 0x34, 1);
	refreshObject(spray);
	_scene->queueFigureWalk(spray, 1, 0, 0, 25);
	setField(spray, 0x34, 0);
	setInputOff(true);
	_sprayObject = spray;
}

void MiljostrupEngine::runFieldWork(int anim, bool leftField, int oldStage, int newStage, int otherStage) {
	// Room 6 (unit 7407): picture 890 at y 8 with both fields on it (x 9 the
	// field 115, x 156 the field 116; 7407:0a4e: stage 0 -> 891, 4 -> 893,
	// 5 -> 894, else 892). The machine (7407:0112: a picture pair, the
	// second for driving left, and an offset) drives over the worked field
	// row by row (20 lines each, from row 6 up), 3 pixels a frame, turning
	// at each end (7407:0790); behind it the row shows the new stage. Enter,
	// Esc or Space end it (7407:09b2). Frame: 7407:0977, 4 ticks of 100 Hz.
	static const int kMachinePictures[7] = { 895, 897, 899, 901, 903, 905, 907 };
	static const int kMachineOffsets[7] = { 1, 0, 7, 1, 1, 0, 9 };
	static const int kRowHeight = 20;
	static const int kFieldWidth = 140;
	auto stagePicture = [](int stage) {
		return stage == 0 ? 891 : (stage == 4 ? 893 : (stage == 5 ? 894 : 892));
	};
	if (anim < 0 || anim > 6)
		return;
	Graphics::ManagedSurface *bg = _archive.loadImage(890);
	Graphics::ManagedSurface *oldPic = _archive.loadImage(stagePicture(oldStage));
	Graphics::ManagedSurface *newPic = _archive.loadImage(stagePicture(newStage));
	Graphics::ManagedSurface *otherPic = _archive.loadImage(stagePicture(otherStage));
	Graphics::ManagedSurface *right = _archive.loadImage(kMachinePictures[anim]);
	Graphics::ManagedSurface *left = _archive.loadImage(kMachinePictures[anim] + 1);
	if (bg && oldPic && newPic && otherPic && right && left) {
		const int workX = leftField ? 9 : 156;
		const int otherX = leftField ? 156 : 9;
		const int offset = kMachineOffsets[anim];
		// the picture as the room keeps it: the background with both fields
		Graphics::ManagedSurface base(bg->w, bg->h, bg->format);
		base.blitFrom(*bg);
		base.transBlitFrom(*otherPic, Common::Point(otherX, 8), otherPic->getTransparentColor());
		base.transBlitFrom(*oldPic, Common::Point(workX, 8), oldPic->getTransparentColor());

		_screen->fillRect(Common::Rect(0, 0, 320, 200), 0);
		_screen->fillRect(Common::Rect(0, 8, 320, 163), 13);
		_screen->blitFrom(base, Common::Point(0, 8));
		_interface->showCursor(false);

		int row = 6, dir = 3;
		int remaining = kFieldWidth - right->w;
		int pos = 0;
		bool done = false;
		Graphics::FrameLimiter limiter(g_system, 25, false);
		while (!done && !shouldQuit()) {
			if (remaining < 0) {
				// the row is finished: it shows the new stage
				base.transBlitFrom(*newPic, Common::Rect(0, row * kRowHeight, newPic->w, row * kRowHeight + kRowHeight),
					Common::Point(workX, 8 + row * kRowHeight), newPic->getTransparentColor());
				_screen->blitFrom(base, Common::Rect(workX, 8 + row * kRowHeight, workX + kFieldWidth + 1,
					8 + row * kRowHeight + kRowHeight), Common::Point(workX, 16 + row * kRowHeight));
				dir = -dir;
				if (--row < 0)
					done = true;
				remaining = kFieldWidth - right->w;
				pos = dir < 1 ? remaining : 0;
			}
			if (!done) {
				const int y = row * kRowHeight;
				Graphics::ManagedSurface strip(kFieldWidth + 1, kRowHeight, base.format);
				strip.blitFrom(base, Common::Rect(workX, 8 + y, workX + kFieldWidth + 1, 8 + y + kRowHeight),
					Common::Point(0, 0));
				int split;
				Common::Rect part;
				if (dir > 0) {
					split = pos + offset;
					part = Common::Rect(0, y, MIN<int>(split, newPic->w), y + kRowHeight);
				} else {
					split = pos + left->w - offset;
					part = Common::Rect(MAX(split, 0), y, newPic->w, y + kRowHeight);
				}
				if (part.isValidRect() && !part.isEmpty())
					strip.transBlitFrom(*newPic, part, Common::Point(part.left, 0), newPic->getTransparentColor());
				Graphics::ManagedSurface *machine = dir > 0 ? right : left;
				strip.transBlitFrom(*machine, Common::Point(pos, 0), machine->getTransparentColor());
				_screen->blitFrom(strip, Common::Point(workX, 16 + y));
				pos += dir;
				remaining -= ABS(dir);
			}

			Common::Event e;
			while (g_system->getEventManager()->pollEvent(e)) {
				if (e.type == Common::EVENT_KEYDOWN && (e.kbd.keycode == Common::KEYCODE_RETURN ||
						e.kbd.keycode == Common::KEYCODE_KP_ENTER || e.kbd.keycode == Common::KEYCODE_ESCAPE ||
						e.kbd.keycode == Common::KEYCODE_SPACE))
					done = true;
			}
			limiter.delayBeforeSwap();
			_screen->update();
			limiter.startFrame();
		}
		_interface->showCursor(true);
	}
	delete bg;
	delete oldPic;
	delete newPic;
	delete otherPic;
	delete right;
	delete left;
}

void MiljostrupEngine::feedCows(int feed) {
	// 784c:04f1: the gate must be open; an empty trough gets the feed
	// (2312: +0x2f fed, +0x30 organic), the feed leaves the inventory
	if (!getFlag(kGate)) {
		say(handlerText(0x784c04dc), 14, true);
		return;
	}
	if (getFlag(kTrough)) {
		say(handlerText(0x784c0373), 14, true);
		return;
	}
	bool organic = feed == kOrganicFeed;
	setFlag(kTrough, 1);
	setField(kTrough, 0x30, organic ? 1 : 0);
	if (organic) {
		// the cows now give organic dung (784c:29ee on DS:c768)
		setFlag(kOrganicDung, 1);
		award(9, 3, 3, 3, 0x784c038d, 0x784c03f8);
	} else {
		award(-9, -3, -3, -3, 0x784c03fe, 0x784c03f8);
		// "Kan man købe økologisk mælk?" Ja / Nej: 1, category 3 (the correct
		// answer as b06f:1185 gets it: 1 the first, 0 the second, b21c:014b)
		setQuiz(0x784c04b8, 0x784c04d5, 0x784c04d8, 1, 3);
	}
	refreshObject(kCow);
	removeObject(feed);
}

void MiljostrupEngine::startBagging() {
	// 784c:62a6 / 5d6d: the potatoes go, the machine runs, command 0x20 to
	// the room (7554:070b): input off, counter 4, the first bag
	setFlag(kBagPotatoes, 0);
	refreshObject(kBagPotatoes);
	setFlag(kBagMachine, 1);
	refreshObject(kBagMachine);
	setInputOff(true);
	_bagsLeft = 4;
	nextBag();
}

void MiljostrupEngine::nextBag() {
	// 7554:001c: counter - 1 (at 0 the machine stops, 5d99); the bag falls
	// from (14, 83) in the states of the roll (0 / 4 / 8 + 1..4), the belt
	// parts move with it; message 0x1f when the bag's walks are done
	if (--_bagsLeft == 0) {
		setFlag(kBagMachine, 0);
		refreshObject(kBagMachine);
	}
	int roll = getFlag(kBagHolder);
	int r = roll == 2 ? 4 : (roll == 3 ? 8 : 0);
	// SetVisible, SetPos and the walks; the first walk step switches the
	// state before the original draws. The engine's tick comes after the
	// scene's update, so the bag is shown at the next tick (tickFarm), once
	// that step has switched it (else it shows the old image for a frame, or
	// a switch made here would let the walk show one image too many)
	_scene->setFigureVisible(kFallingBag, false);
	_scene->setFigurePos(kFallingBag, 14, 83);
	_showBag = true;
	_scene->queueFigureWalk(kFallingBag, r + 1, 0, 0, 5);
	_scene->queueFigureWalk(kFallingBag, r + 2, 24, 0, 8);
	_scene->queueFigureWalk(kFallingBag, r + 2, 0, 0, 1);
	_scene->queueFigureWalk(kFallingBag, r + 3, 0, 0, 1);
	_scene->queueFigureWalk(kFallingBag, r + 3, 39, 0, 13);
	_scene->queueFigureWalk(kFallingBag, r + 4, 0, 0, _bagsLeft == 0 ? 2 : 1);
	_scene->queueFigureWalk(kBelt1, 1, 0, 0, 5);
	_scene->queueFigureWalk(kBelt1, 2, 0, 0, 8);
	_scene->queueFigureWalk(kBelt1, 1, 0, 0, 2);
	_scene->queueFigureWalk(kBelt1, 2, 0, 0, 13);
	_scene->queueFigureWalk(kBelt1, 1, 0, 0, 1);
	_scene->queueFigureWalk(kBelt2, 1, 0, 0, 13);
	_scene->queueFigureWalk(kBelt2, 2, 0, 0, 1);
	_scene->queueFigureWalk(kBelt2, 1, 0, 0, 15);
	_scene->queueFigureWalk(kBelt3, 1, 0, 0, 14);
	_scene->queueFigureWalk(kBelt3, 2, 0, 0, 1);
	_scene->queueFigureWalk(kBelt3, 1, 0, 0, 14);
	_bagging = true;
}

void MiljostrupEngine::tickFarm() {
	if (_location != kFarm)
		return;
	// The sprinkler's spray (784c:4060, Walk(1, 0, 0, 25)); then message
	// 0x1f to the yard (7676:0000): command 0x40, the award of 4272
	if (_sprayObject && !_scene->isFigureBusy(_sprayObject)) {
		_sprayObject = 0;
		setInputOff(false);
		award(5, 0, 5, 0, 0x784c41d6, 0x784c426f);
	}
	if (_showBag) {
		_showBag = false;
		_scene->setFigureVisible(kFallingBag, true);
	}
	if (!_bagging || _scene->isFigureBusy(kFallingBag))
		return;
	// message 0x1f (7554:06d4): another bag at the next idle tick (02c0:
	// 001c, then 69da), or the end (05a6)
	if (_bagsLeft >= 1) {
		nextBag();
		setFlag(kMachineBags, getFlag(kMachineBags) + 1);
		refreshObject(kMachineBags);
		return;
	}
	_bagging = false;
	_scene->setFigureVisible(kFallingBag, false);
	_scene->setFigureState(kFallingBag, 2);
	setInputOff(false);
	switch (getFlag(kBagHolder)) {
	case 1: // PE
		award(10, 0, 10, 0, 0x75540357, 0x7554043a);
		break;
	case 2: // PVC: "Hvad er dioxin?" Tandpasta / Et giftstof: 0, category 2
		award(-10, 0, -10, 0, 0x7554043d, 0x755404c2);
		setQuiz(0x755404c5, 0x755404d5, 0x755404df, 0, 2);
		break;
	default: // paper
		award(10, 0, 10, 0, 0x755404eb, 0);
		break;
	}
	setFlag(kMachineBags, getFlag(kMachineBags) + 1);
	refreshObject(kMachineBags);
}

void MiljostrupEngine::room9Award(int value, uint32 text, int object) {
	// 784c:64af / 6ab5 / 7119: Award(value, 0, value, 0), the object goes
	award(value, 0, value, 0, text, 0);
	setFlag(object, 0);
	refreshObject(object);
}

void MiljostrupEngine::loadCrate(int crate, int item) {
	// 784c:6154: the crates are used (+0x2f); with the loose potatoes they go
	// onto the wagon (boxes 1, crates 2)
	setFlag(crate, 1);
	refreshObject(crate);
	if (item == kLoosePotatoes)
		setFlag(kWagon, crate == kCardboardBoxes ? 1 : 2);
	refreshObject(kWagon);
}

void MiljostrupEngine::usePileItem(int item, int second) {
	// 784c:0853: Benyt with its own pile puts one back, else the root's Benyt
	const FarmPile *p = findPile(item);
	if (second != p->id) {
		sayRootMessage(kVerbUse);
		return;
	}
	setItemCount(item, getField(item, kItemCount) - 1);
	setPileCount(p->id, getField(p->id, kPileCount) + 1);
}

} // End of namespace Miljostrup
