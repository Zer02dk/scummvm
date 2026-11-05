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

#include "common/debug.h"

#include "olga/locations_data.h"
#include "olga/olga.h"
#include "olga/states_data.h"
#include "olga/walk.h"

namespace Olga {

namespace {

// The base handler's verbs (unit Room 1cf0:17ef .. 19b7): their texts,
// in button order; Tal til is said in colour 0xd, the rest in 0xe
const uint16 kBaseVerbText[kNumVerbs] = {
	0x17d8, 0x1811, 0x184a, 0x1888, 0x18bb, 0x18ee, 0x1931, 0x1964, 0x1997
};

// Door texts (unit Situations 1222)
enum {
	kTextDoorOpen = 0x0f5a,        // "Den er åben."
	kTextDoorClosed = 0x0f67,      // "Den er lukket."
	kTextAlreadyOpen = 0x0ff2,
	kTextAlreadyClosed = 0x10a9,
	kTextGoClosed = 0x1158,        // Door_GoThrough: closed
	kTextLocked = 0x0ebe,          // Passage_GoThrough: bit 0 clear
	kTextCupboard = 0x1246,        // "Der får du mig ikke ind!"
	kTextFarvehDoor = 0x393f
};

const uint16 kSegRoom = 0x1cf0;
const uint16 kSegSituations = 0x1222;

} // End of anonymous namespace

Common::String OlgaEngine::objectName(int id) const {
	if (id <= kNoObject || id > kLastId)
		return Common::String();
	// Some refreshes rename their object (1222:3d0b, 1da5)
	if (!_names[id].empty())
		return _names[id];
	const StateDef &s = kStates[id - kFirstId];
	return _exe.readString(s.nameSeg, s.nameOff);
}

void OlgaEngine::resetSentence() {
	// Responses_ResetSentence (5427:15fc): stage 1, no objects
	_stage = 1;
	_obj1 = _obj2 = kNoObject;
	drawSentence(false);
}

void OlgaEngine::say(const Common::String &text, byte color, bool talk) {
	// Room_Say (1cf0:15d1): the message line, Olga talks (animation 7), the
	// text goes after length * text speed frames
	_screen->fillRect(Common::Rect(0, 0, 320, kRoomTop), 0);
	_font.drawCentred(*_screen, text, 0, color);
	SceneObject *olga = _scene.olga();
	if (talk && olga)
		Scene::setAnim(*olga, 7);
	_textTimer = text.size() * _textSpeed;
}

void OlgaEngine::showText(const Common::String &text, byte color) {
	_screen->fillRect(Common::Rect(0, 0, 320, kRoomTop), 0);
	_font.drawCentred(*_screen, text, 0, color);
	_textTimer = 0;
}

void OlgaEngine::sayExe(uint16 seg, uint16 off, byte color, bool talk) {
	say(_exe.readString(seg, off), color, talk);
}

void OlgaEngine::clearText() {
	// Room_16d3 (method +0x40): empty line, Olga stands again
	_screen->fillRect(Common::Rect(0, 0, 320, kRoomTop), 0);
	_textTimer = 0;
	SceneObject *olga = _scene.olga();
	if (olga && !olga->moving)
		Scene::setAnim(*olga, 1);
}

void OlgaEngine::cancelWalk() {
	// Responses_CancelWalk (5427:15ad)
	SceneObject *olga = _scene.olga();
	if (_pending && olga) {
		Scene::stopScript(*olga);
		Scene::setAnim(*olga, 1);
	}
	_pending = false;
	resetSentence();
}

void OlgaEngine::startVerb() {
	// Responses_StartVerb (5427:1cb7)
	if (_textTimer > 0)
		clearText();
	int target = (_stage == 2 && carried(_obj1)) ? _obj2 : _obj1;
	SceneObject *olga = _scene.olga();
	if (carried(target) || !olga) {
		runVerb();
		return;
	}
	// Responses_WalkTarget: the bottom centre of the hotspot's rectangle
	const ScnFile &scn = _scene.scn();
	const ScnHotspot *h = nullptr;
	int dx = 0, dy = 0;
	for (uint i = 0; i < scn.objects.size() && !h; i++)
		if (scn.objects[i].hotspot.id == target) {
			h = &scn.objects[i].hotspot;
			SceneObject *o = _scene.findObject(target);
			if (o) {
				dx = o->hotDx;
				dy = o->hotDy;
			}
		}
	for (uint i = 0; i < scn.hotspots.size() && !h; i++)
		if (scn.hotspots[i].id == target)
			h = &scn.hotspots[i];
	Common::Array<SceneObject::Move> moves;
	if (!h || !planWalk(scn, _scene, *olga, Common::Point((h->x1 + h->x2) / 2 + dx, h->y2 + dy), moves)) {
		runVerb();
		return;
	}
	Scene::stopScript(*olga);
	if (_fastWalk) {
		// Debugging only: the moves' sum puts her feet on the walk's last
		// point; the verb runs at once
		for (uint i = 0; i < moves.size(); i++) {
			olga->x += moves[i].dx;
			olga->y += moves[i].dy;
		}
		runVerb();
		return;
	}
	olga->moves = moves;
	_scene.startScript(*olga, kEventArrived);
	_pending = true;
	drawSentence(true);
}

void OlgaEngine::handleEvent(const SceneEvent &e) {
	// Responses_RoomEvent (5427:1454): 3 = Olga arrived, the rest belong to
	// the situations
	if (_kind == kLocKemSkab) {
		kemEvent(e);
	} else if (_kind == kLocGameEnd) {
		gameEndDone(); // GameEnd_02a9: any room event
	} else if (e.code == kEventArrived) {
		if (_pending && e.object == _scene.olgaIndex()) {
			_pending = false;
			runVerb();
		}
	} else {
		situationEvent(e);
	}
}

void OlgaEngine::runVerb() {
	// Responses_RunVerb (5427:1820): Olga stands, the first object's handler
	// gets the verb (Brug and Giv with the second object), then the sentence
	// is reset
	SceneObject *olga = _scene.olga();
	if (olga)
		Scene::setAnim(*olga, 1);
	int id = _obj1;
	debugC(1, kDebugGraphics, "verb %d on %d (%d)", _verb, id, _obj2);
	if (id > kNoObject && id <= kLastId) {
		uint16 vmt = kStates[id - kFirstId].vmt;
		if (!verbSituation(id, _verb, _obj2)) {
			if (isDoor(id) || vmt == kVmtPassage)
				verbDoor(id, _verb);
			else if (!verbThing(id, _verb, _obj2))
				verbBase(_verb);
		}
	}
	resetSentence();
}

const ThingDef *OlgaEngine::thing(int id) const {
	for (uint i = 0; i < ARRAYSIZE(kThings); i++)
		if (kThings[i].id == id)
			return &kThings[i];
	return nullptr;
}

bool OlgaEngine::hotspotOff(int id) const {
	const ThingDef *t = thing(id);
	return _controlOff[id] || (_flags[id] & 0x100) || (t && t->pronounOff && (_flags[id] & 1));
}

void OlgaEngine::addToInventory(int id) {
	// IntList_Add on DS:3ef0, then InvPanel_Redraw (1c51:0456): the last row
	if (!carried(id))
		_inventory.push_back(id);
	int n = _inventory.size();
	_inventoryRow = n < 6 ? 1 : (n - 1) / 5;
	drawInventory();
}

void OlgaEngine::removeFromInventory(int id) {
	for (uint i = 0; i < _inventory.size(); i++)
		if (_inventory[i] == id)
			_inventory.remove_at(i--);
	int n = _inventory.size();
	_inventoryRow = n < 6 ? 1 : (n - 1) / 5;
	drawInventory();
}

bool OlgaEngine::verbThing(int id, int verb, int other) {
	const ThingDef *t = thing(id);
	if (!t)
		return false;
	if (!t->pronounOff) {
		// Look-only handlers (VMT DS:02e6, 1222:007f): Se på says the text
		if (verb != kVerbSePaa)
			return false;
		sayExe(kSegSituations, t->descOff);
		return true;
	}
	Common::String pronoun = _exe.readString(kSegSituations, t->pronounOff);
	switch (verb) {
	case kVerbTag:
		// Thing_Tag (1222:1a47): bit 0 = carried
		if (_flags[id] & 1) {
			say(_exe.readString(kSegSituations, 0x1a34) + pronoun + _exe.readString(kSegSituations, 0x1a3c));
		} else {
			_flags[id] |= 1;
			refresh(id);
			addToInventory(id);
		}
		return true;
	case kVerbGiv:
		// Thing_Giv (1222:19dc): to its home is the same as Brug
		if (other != t->home)
			return false;
		// fall through
	case kVerbBrug:
		// Thing_Brug (1222:18b7): Brug X med its home puts it back
		if (other != t->home)
			return false;
		if (_flags[id] & 1) {
			_flags[id] &= ~1;
			refresh(id);
			removeFromInventory(id);
		} else {
			// The pronoun with a capital D; "Dem" becomes "De"
			Common::String p = pronoun;
			if (!p.empty())
				p.setChar('D', 0);
			if (p == _exe.readString(kSegSituations, 0x1899))
				p = _exe.readString(kSegSituations, 0x189d);
			say(p + _exe.readString(kSegSituations, 0x18a0));
		}
		return true;
	case kVerbSePaa:
		// Thing_SePaa (1222:1a0e): the description
		sayExe(kSegSituations, t->descOff);
		return true;
	default:
		return false;
	}
}

void OlgaEngine::verbBase(int verb) {
	int k = verb - kVerbSkub;
	sayExe(kSegRoom, kBaseVerbText[k], verb == kVerbTalTil ? 0xd : 0xe);
}

bool OlgaEngine::isDoor(int id) const {
	uint16 vmt = kStates[id - kFirstId].vmt;
	return vmt == kVmtDoor || vmt == kVmtCupboard || vmt == kVmtFarvehDoor || vmt == kVmtFrontDoor;
}

void OlgaEngine::refresh(int id) {
	// The handlers' refresh (method 11): doors show animation bit 0 + 1
	// (1222:11b3), things 2 when carried (Thing_Refresh), the rest 1
	if (id <= kNoObject || id > kLastId)
		return;
	// Situations_1281: a cupboard shows (clears bit 0x100) or hides its
	// contents' hotspots as it is open or closed, and refreshes them
	if (kStates[id - kFirstId].vmt == kVmtCupboard) {
		bool shown = (_flags[id] & 0x101) == 1;
		for (uint i = 0; i < ARRAYSIZE(kChildren); i++) {
			if (kChildren[i].parent != id)
				continue;
			int c = kChildren[i].child;
			if (shown)
				_flags[c] &= ~0x100;
			else
				_flags[c] |= 0x100;
			refresh(c);
		}
	}
	SceneObject *o = _scene.findObject(id);
	if (!o)
		return;
	if (refreshSituation(id, *o))
		return;
	const ThingDef *t = thing(id);
	if (isDoor(id))
		Scene::setAnim(*o, (_flags[id] & 1) + 1);
	else if (t && t->pronounOff)
		Scene::setAnim(*o, (_flags[id] & 1) ? 2 : 1); // Thing_Refresh
	else
		Scene::setAnim(*o, 1);
}

void OlgaEngine::verbDoor(int id, int verb) {
	// Doors (VMT DS:03ca, notes/verbs.md): bit 0 = open. Passages (DS:038e)
	// only go through with Brug, Tag and Åben
	bool passage = kStates[id - kFirstId].vmt == kVmtPassage;
	bool open = (_flags[id] & 1) != 0;
	switch (verb) {
	case kVerbBrug:
	case kVerbTag:
		goThrough(id);
		break;
	case kVerbAaben:
		if (passage) {
			goThrough(id);
		} else if (open) {
			sayExe(kSegSituations, kTextAlreadyOpen);
		} else {
			_flags[id] |= 1;
			refresh(id);
		}
		break;
	case kVerbLuk:
		if (passage) {
			verbBase(verb);
		} else if (!open) {
			sayExe(kSegSituations, kTextAlreadyClosed);
		} else {
			_flags[id] &= ~1;
			refresh(id);
		}
		break;
	case kVerbSePaa:
		if (passage)
			verbBase(verb);
		else
			sayExe(kSegSituations, open ? kTextDoorOpen : kTextDoorClosed);
		break;
	default:
		verbBase(verb);
		break;
	}
}

void OlgaEngine::goThrough(int id) {
	// Method 13: Door_GoThrough (1222:1167) -> Passage_GoThrough (0ecb) ->
	// Situations_DoorExit (09fc)
	uint16 vmt = kStates[id - kFirstId].vmt;
	if (vmt == kVmtCupboard) {
		sayExe(kSegSituations, kTextCupboard);
		return;
	}
	if (vmt == kVmtFarvehDoor) {
		sayExe(kSegSituations, kTextFarvehDoor);
		return;
	}
	if (vmt != kVmtPassage && !(_flags[id] & 1)) {
		sayExe(kSegSituations, kTextGoClosed);
		return;
	}
	if (!(_flags[id] & 1)) {
		sayExe(kSegSituations, kTextLocked);
		return;
	}
	for (uint i = 0; i < ARRAYSIZE(kDoorExits); i++) {
		if (kDoorExits[i].hotspot == id) {
			setExit(kDoorExits[i].location, kDoorExits[i].entry);
			return;
		}
	}
	// 194, the ENTRE front door: the end-of-game check (method 13)
	if (id == 194) {
		endCheck();
		return;
	}
	verbBase(kVerbBrug);
}

} // End of namespace Olga
