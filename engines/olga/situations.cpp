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

// The situations' handler classes (unit Situations 1222) and the room
// events of the overlay unit Responses (5427), from
// notes/situation-logic.md. Texts are read from OLGA.EXE / OLGA.OVR.

#include "olga/olga.h"
#include "olga/states_data.h"

namespace Olga {

namespace {

const uint16 kSeg = 0x1222;          // unit Situations
const uint16 kSegResponses = 0x5427; // overlay unit Responses

// Handler ids
enum {
	kIdWindow = 102, kIdRadiator = 103, kIdBoy = 104, kIdCeilingLamp = 106,
	kIdBlackboard = 108, kIdSkylight = 124, kIdDevil = 134, kIdPraise = 190,
	kIdGirl = 205,
	kIdPicture = 139, kIdWall = 140, kIdMarkers = 204, kIdFormGirl = 202,
	kIdCrayons = 142, kIdDog = 133, kIdPaperBoy = 171, kIdOwner = 206,
	kIdSister = 174, kIdSisterGarage = 209, kIdLars = 164, kIdGroup = 165,
	kIdFarvehDoor = 162, kIdSink = 207, kIdKitchenDoor = 193
};

// Handler classes (VMTs, tools/situations.py)
enum {
	kVmtWindow = 0x0356, kVmtRadiator = 0x0442, kVmtBoy = 0x031e,
	kVmtCeilingLamp = 0x0a9e, kVmtBlackboard = 0x0986, kVmtGirl = 0x09be,
	kVmtLamp = 0x0ad6, kVmtDevil = 0x047a, kVmtPraise = 0x04b2,
	// FORMNING
	kVmtFormWindow = 0x0a2e, kVmtKids = 0x0a66, kVmtFormGirl = 0x0832,
	kVmtMarkers = 0x094e, kVmtMarkerThing = 0x08de, kVmtGasMask = 0x0916,
	kVmtPicture = 0x0522, kVmtWall = 0x055a,
	// GANG6 / HKOEKKEN: poison and salmiak
	kVmtPoison = 0x0bee, kVmtSalmiak = 0x086a,
	// GAARD
	kVmtLars = 0x06aa, kVmtGroup = 0x06e2,
	// GANG1 / KOEKKEN
	kVmtBagBoy = 0x0672, kVmtLifters = 0x0b0e, kVmtTrolley = 0x09f6
};

enum {
	kIdBagBoy = 186, kIdLifters = 188, kIdTrolley = 167,
	kIdClassDoor = 112, kIdKitchenDoorSchool = 123
};

// Room events (Responses_RoomEvent 5427:1454)
enum {
	kEventDevilGone = 4, kEventDevilTalk = 5, kEventDevilLaugh = 6,
	kEventBagBoyIn = 7, kEventLarsAtGroup = 8, kEventPraiseDone = 9,
	kEventLarsWalks = 0xc, kEventLiftingDone = 0xd,
	kEventOwnerShouts = 0x10, kEventPolice = 0x11
};

// Brug gift / salmiak med these is the right disposal (set 1222:3853);
// Giv also counts the sink (set 1222:38ab)
bool isDisposal(int id) {
	return id == 161 || id == kIdFarvehDoor || id == 197;
}

SceneObject::Move move(int anim, int steps) {
	SceneObject::Move m = { 0, 0, anim, steps };
	return m;
}

} // End of anonymous namespace

void OlgaEngine::setInput(bool enabled) {
	// App_02e9 (1f77:02e9): the arrow while the view takes input, else the
	// hourglass
	_inputEnabled = enabled;
	setCursor(enabled ? 0x134e : 0x138e);
}

void OlgaEngine::showDevil() {
	_flags[kIdDevil] = 1;
	refresh(kIdDevil);
}

void OlgaEngine::showPraise() {
	_flags[kIdPraise] = 1;
	refresh(kIdPraise);
}

void OlgaEngine::startPendingFeedback() {
	// Responses_19c6 (5427:19c6): only if the sprite is in this room
	if (_pendingDevil) {
		if (_scene.findObject(kIdDevil))
			showDevil();
	} else if (_pendingPraise) {
		if (_scene.findObject(kIdPraise))
			showPraise();
	}
	_pendingDevil = _pendingPraise = false;
}

bool OlgaEngine::refreshSituation(int id, SceneObject &o) {
	uint32 f = _flags[id];
	SceneObject *olga = _scene.olga();
	switch (kStates[id - kFirstId].vmt) {
	case kVmtDevil:
		// Situations_155b: appears (animation 2, 7 steps), then event 5; the
		// room takes no input, Olga is startled (animation 8)
		if (!f) {
			Scene::setAnim(o, 1);
		} else {
			_flags[id] = 0;
			Scene::stopScript(o);
			o.moves.push_back(move(2, 7));
			_scene.startScript(o, kEventDevilTalk);
			setInput(false);
			if (olga)
				Scene::setAnim(*olga, 8);
		}
		return true;
	case kVmtPraise:
		// Situations_16cf: animation 2 for 15 steps, then 1, then event 9;
		// one of four texts in colour 15
		if (!f) {
			Scene::setAnim(o, 1);
		} else {
			static const uint16 texts[4] = { 0x1670, 0x1687, 0x16ae, 0x16bd };
			_flags[id] = 0;
			Scene::stopScript(o);
			o.moves.push_back(move(2, 15));
			o.moves.push_back(move(1, 1));
			_scene.startScript(o, kEventPraiseDone);
			setInput(false);
			sayExe(kSeg, texts[getRandomNumber(3)], 0xf, false);
			if (olga)
				Scene::setAnim(*olga, 8);
		}
		return true;
	case kVmtWindow:
	case kVmtRadiator:
	case kVmtBlackboard:
		// 1222:091f / 14fe / 3e79: open or done = animation 2
		Scene::setAnim(o, (f & 1) + 1);
		return true;
	case kVmtBoy:
		// 1222:0587: cold 3, just right 4, rather warm 2, sweating 1
		Scene::setAnim(o, (f & 2) ? 3 : (f & 4) ? 4 : (f & 1) ? 2 : 1);
		return true;
	case kVmtCeilingLamp:
		// 1222:4652: all three ceiling lamps follow S[106]
		Scene::setAnim(o, _flags[kIdCeilingLamp] ? 2 : 1);
		return true;
	case kVmtGirl:
		// 1222:4184: good light 3, the headlamp 2, too dark 1
		Scene::setAnim(o, (f & 1) ? 3 : (f & 2) ? 2 : 1);
		return true;
	case kVmtFormWindow:
	case kVmtKids:
		// 1222:091f / 4561
		Scene::setAnim(o, (f & 1) + 1);
		return true;
	case kVmtFormGirl:
		// 1222:351d: cold 1, markers swapped 3, gas mask 4, the spirit markers 2
		Scene::setAnim(o, (f & 1) ? 1 : (f & 4) ? 3 : (f & 2) ? 4 : 2);
		return true;
	case kVmtMarkers:
		// 1222:3d0b: animation state + 1, and the name follows what lies there
		Scene::setAnim(o, f + 1);
		_names[id] = _exe.readString(kSeg, f == 1 ? 0x3cfd : 0x3cf5);
		return true;
	case kVmtWall:
		// 1222:1da5: the ugly wall, or Olga's picture on it (renamed)
		Scene::setAnim(o, f ? 2 : 1);
		_names[id] = _exe.readString(kSeg, f ? 0x1d9d : 0x1d94);
		return true;
	case kVmtBagBoy:
		// 1222:26fb: the TASKVALG answer (bit 2) makes him walk into the
		// classroom (stand 8, 105 px right in 15 steps, stand 8; event 7);
		// in (bit 1): animation 3, hotspot off; else 1, or 2 with bit 0
		if (f & 4) {
			_flags[id] &= ~4;
			Scene::stopScript(o);
			o.moves.push_back(move(4, 8));
			SceneObject::Move walk = { 0x69, 0, 2, 0xf };
			o.moves.push_back(walk);
			o.moves.push_back(move(4, 8));
			_scene.startScript(o, kEventBagBoyIn);
			setInput(false);
		} else if (f & 2) {
			Scene::setAnim(o, 3);
		} else {
			Scene::setAnim(o, (f & 1) + 1);
		}
		return true;
	case kVmtLifters: {
		// 1222:4b48: the LOEFT answer: 2 carry it (animation 2), 3 with the
		// trolley 167 (animation 3); both 138 px in 46 steps, then state 1
		// and event 0xd. State 1: animation 4, hotspot off
		int s = f & 3;
		if (s == 0) {
			Scene::setAnim(o, 1);
		} else if (s == 1) {
			Scene::setAnim(o, 4);
		} else {
			_flags[id] = 1;
			Scene::stopScript(o);
			SceneObject::Move carry = { 0x8a, 0, s == 2 ? 2 : 3, 0x2e };
			o.moves.push_back(carry);
			_scene.startScript(o, kEventLiftingDone);
			SceneObject *trolley = s == 3 ? _scene.findObject(kIdTrolley) : nullptr;
			if (trolley) {
				Scene::stopScript(*trolley);
				SceneObject::Move roll = { 0x8a, 0, 2, 0x2e };
				trolley->moves.push_back(roll);
				_scene.startScript(*trolley, 2);
			}
			setInput(false);
		}
		return true;
	}
	case kVmtTrolley:
		// 1222:3eea
		Scene::setAnim(o, (f & ~0x100) + 1);
		return true;
	case kVmtLars:
		// 1222:2a91: with the group (bit 1): animation 3, his hotspot off;
		// taken (bit 0): he starts to walk (event 0xc), the room waits
		if (f & 2) {
			Scene::setAnim(o, 3); // the hotspot: OlgaEngine::hotspotOff
		} else if (!(f & 1)) {
			Scene::setAnim(o, 1);
		} else {
			if (olga)
				Scene::setAnim(*olga, 1);
			Scene::stopScript(o);
			o.moves.push_back(move(1, 0));
			_scene.startScript(o, kEventLarsWalks);
			setInput(false);
		}
		return true;
	default:
		return false;
	}
}

void OlgaEngine::situationEvent(const SceneEvent &e) {
	SceneObject *devil = _scene.findObject(kIdDevil);
	SceneObject *olga = _scene.olga();
	switch (e.code) {
	case kEventDevilTalk:
		// Responses_0c2a: talking (animation 3, 4 steps), one of three
		// taunts in colour 12, three laughs to come
		if (devil) {
			devil->moves.push_back(move(3, 4));
			_scene.startScript(*devil, kEventDevilLaugh);
			int r = getRandomNumber(2);
			uint16 text = r == 0 ? 0x0bc8 : r == 2 ? 0x0c0a : (_devilFirst ? 0x0be7 : 0x0bf6);
			_devilFirst = false;
			sayExe(kSegResponses, text, 0xc, false);
			_devilCount = 3;
		}
		break;
	case kEventDevilLaugh:
		// Responses_0d44: laughs (animation 4, 2 steps) until the count runs
		// out, then leaves (animation 5, 10 steps)
		if (devil) {
			if (--_devilCount == 0) {
				devil->moves.push_back(move(5, 10));
				_scene.startScript(*devil, kEventDevilGone);
			} else {
				devil->moves.push_back(move(4, 2));
				_scene.startScript(*devil, kEventDevilLaugh);
			}
		}
		break;
	case kEventDevilGone:
		// Responses_0b07: hidden again, input back, Olga stands
		if (devil) {
			Scene::stopScript(*devil);
			Scene::setAnim(*devil, 1);
		}
		setInput(true);
		if (olga)
			Scene::setAnim(*olga, 1);
		break;
	case kEventPraiseDone:
		// Responses_0b78
		if (olga)
			Scene::setAnim(*olga, 1);
		setInput(true);
		break;
	case kEventBagBoyIn:
		// Responses_0e08: the boy is in (bit 1), the classroom door 112
		// toggles, input back
		_flags[kIdBagBoy] |= 2;
		refresh(kIdBagBoy);
		_flags[kIdClassDoor] ^= 1;
		refresh(kIdClassDoor);
		setInput(true);
		break;
	case kEventLiftingDone:
		// Responses_1123: the trolley's bit 0 off, the kitchen door 123
		// toggles, praise
		refresh(kIdLifters);
		_flags[kIdTrolley] &= ~1;
		refresh(kIdTrolley);
		_flags[kIdKitchenDoorSchool] ^= 1;
		refresh(kIdKitchenDoorSchool);
		showPraise();
		break;
	case kEventLarsWalks: {
		// Responses_107b: Lars walks 168 pixels right in 56 steps
		// (animation 2), then event 8; Olga points (animation 9)
		SceneObject *lars = _scene.findObject(kIdLars);
		if (lars) {
			Scene::setAnim(*lars, 2);
			Scene::stopScript(*lars);
			SceneObject::Move m = { 0xa8, 0, 2, 0x38 };
			lars->moves.push_back(m);
			_scene.startScript(*lars, kEventLarsAtGroup);
		}
		setInput(false);
		if (olga)
			Scene::setAnim(*olga, 9);
		break;
	}
	case kEventLarsAtGroup: {
		// Responses_0e6f: Lars and the group talk; Lars at 84,82, Olga at
		// 274,77, praise
		_flags[kIdLars] |= 2;
		_flags[kIdGroup] |= 1;
		SceneObject *lars = _scene.findObject(kIdLars);
		if (lars)
			Scene::setPosition(*lars, 0x54, 0x52);
		refresh(kIdLars);
		if (olga) {
			Scene::setPosition(*olga, 0x112, 0x4d);
			Scene::setAnim(*olga, 1);
		}
		showPraise();
		break;
	}
	case kEventOwnerShouts: {
		// Responses_120e: the owner shouts (colour 12, no timer) for
		// text speed * 34 / 3 frames, then event 0x11
		SceneObject *owner = _scene.findObject(kIdOwner);
		_screen->fillRect(Common::Rect(0, 0, 320, kRoomTop), 0);
		_font.drawCentred(*_screen, _exe.readString(kSegResponses, 0x11eb), 0, 0xc);
		if (owner) {
			Scene::stopScript(*owner);
			owner->moves.push_back(move(4, _textSpeed * 0x22 / 3));
			_scene.startScript(*owner, kEventPolice);
		} else {
			setExit(100, 2);
		}
		break;
	}
	case kEventPolice:
		// Responses_129e: the police ending
		setExit(100, 2);
		break;
	default:
		break;
	}
}

bool OlgaEngine::verbLamp(int id, const ThingDef &t, int other) {
	// Lamp_Brug (1222:472d): on its target (the skylight also on the ceiling
	// lamp 106) the lamp's value is or-ed into the target's state, unless the
	// target is already solved and the lamp is the wrong one
	if (other != t.lampTarget && !(id == kIdSkylight && other == kIdCeilingLamp))
		return false;
	int target = t.lampTarget;
	if (t.lampValue == 1 || _flags[target] != 1) {
		_flags[target] |= t.lampValue;
		refresh(target);
		removeFromInventory(id);
		SceneObject *olga = _scene.olga();
		if (olga)
			Scene::setAnim(*olga, 1);
		if (t.lampValue == 1)
			showPraise();
		else
			showDevil();
		if (t.lampValue == 1 && id == kIdSkylight) {
			_flags[kIdCeilingLamp] |= 1;
			refresh(kIdCeilingLamp);
			refresh(230);
			refresh(231);
		}
	} else {
		say(_exe.readString(kSeg, 0x470d) + _exe.readString(kSeg, t.pronounOff) +
			_exe.readString(kSeg, 0x4728));
	}
	return true;
}

bool OlgaEngine::verbSituation(int id, int verb, int other) {
	uint32 &f = _flags[id];
	SceneObject *olga = _scene.olga();
	switch (kStates[id - kFirstId].vmt) {
	case kVmtWindow:
		// KLASSEN's window: opening it makes the boy cold (devil), closing
		// it again is right (1222:06ce / 0760 / 084e)
		if (verb == kVerbSePaa) {
			sayExe(kSeg, (f & 1) ? 0x06b1 : 0x06bf);
		} else if (verb == kVerbAaben) {
			if (f & 1) {
				sayExe(kSeg, 0x074a);
			} else {
				f |= 1;
				_flags[kIdBoy] |= 2;
				refresh(id);
				refresh(kIdBoy);
				if (olga)
					Scene::setAnim(*olga, 1);
				showDevil();
			}
		} else if (verb == kVerbLuk) {
			if (!(f & 1)) {
				sayExe(kSeg, 0x0837);
			} else {
				f &= ~1;
				_flags[kIdBoy] &= ~2;
				refresh(kIdBoy);
				if (olga)
					Scene::setAnim(*olga, 1);
				showPraise();
				refresh(id);
			}
		} else {
			return false;
		}
		return true;
	case kVmtRadiator:
		// Brug / Åben / Luk: the RADIATOR pop-up (location 10, entry 2) until
		// it is set right (1222:1487); Se på 13e6
		if (verb == kVerbBrug || verb == kVerbAaben || verb == kVerbLuk) {
			if (!(f & 1))
				setExit(10, 2);
			else
				sayExe(kSeg, 0x1467);
		} else if (verb == kVerbSePaa) {
			sayExe(kSeg, (f & 1) ? 0x13d1 : 0x13bb);
		} else {
			return false;
		}
		return true;
	case kVmtBoy:
		// 1222:0352 / 04a5: cold (bit 1), just right (bit 2), warm (bit 0)
		if (verb == kVerbSePaa) {
			sayExe(kSeg, (f & 2) ? 0x02f9 : (f & 4) ? 0x030c : (f & 1) ? 0x0325 : 0x0346);
		} else if (verb == kVerbTalTil) {
			if (f & 2)
				sayExe(kSeg, 0x043b, 0xb, false);
			else if (f & 4)
				sayExe(kSeg, 0x0458, 0x2, false);
			else if (!(f & 1))
				sayExe(kSeg, 0x0477, 0xc, false);
			else
				sayExe(kSeg, 0x048e, 0xc, false);
		} else {
			return false;
		}
		return true;
	case kVmtCeilingLamp:
		if (verb != kVerbSePaa)
			return false;
		sayExe(kSeg, _flags[kIdCeilingLamp] ? 0x45ee : 0x45d2);
		return true;
	case kVmtBlackboard:
		if (verb != kVerbSePaa)
			return false;
		refresh(id);
		sayExe(kSeg, (f & 1) ? 0x3df8 : 0x3de1);
		return true;
	case kVmtGirl:
		// 1222:3fe4 / 4108: good light (bit 0), the headlamp (bit 1)
		if (verb == kVerbSePaa)
			sayExe(kSeg, (f & 1) ? 0x3f6f : (f & 2) ? 0x3f97 : 0x3fbf);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, (f & 1) ? 0x40a1 : (f & 2) ? 0x40bc : 0x40e4, 0x2, false);
		else
			return false;
		return true;
	case kVmtLamp: {
		const ThingDef *t = thing(id);
		if (!t || (verb != kVerbBrug && verb != kVerbGiv))
			return false;
		return verbLamp(id, *t, other);
	}
	case kVmtFormWindow:
		// FORMNING's window (1222:427e / 436c): open makes the girl 202 cold
		if (verb == kVerbSePaa) {
			sayExe(kSeg, (f & 1) ? 0x06b1 : 0x06bf);
		} else if (verb == kVerbAaben) {
			if (f & 1) {
				sayExe(kSeg, 0x4268);
			} else {
				f |= 1;
				_flags[kIdFormGirl] |= 1;
				refresh(id);
				refresh(kIdFormGirl);
				if (olga)
					Scene::setAnim(*olga, 1);
				showDevil();
			}
		} else if (verb == kVerbLuk) {
			if (!(f & 1)) {
				sayExe(kSeg, 0x4355);
			} else {
				f &= ~1;
				_flags[kIdFormGirl] &= ~1;
				refresh(kIdFormGirl);
				if (olga)
					Scene::setAnim(*olga, 1);
				showPraise();
				refresh(id);
			}
		} else {
			return false;
		}
		return true;
	case kVmtKids:
		// 1222:446b / 4503: the arkitektlampe (107) is their light
		if (verb == kVerbSePaa)
			sayExe(kSeg, (f & 1) ? 0x4458 : 0x443d);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, (f & 1) ? 0x44ed : 0x44c9, 0x2, false);
		else
			return false;
		return true;
	case kVmtFormGirl:
		// 1222:3326 / 345c: cold (bit 0), markers swapped (bit 2), gas mask (bit 1)
		if (verb == kVerbSePaa) {
			sayExe(kSeg, (f & 1) ? 0x32c8 : (f & 4) ? 0x32df : (f & 2) ? 0x32f4 : 0x3309);
		} else if (verb == kVerbTalTil) {
			if (f & 1)
				sayExe(kSeg, 0x3417, 0xb, false);
			else if (f & 4)
				sayExe(kSeg, 0x3430, 0x2, false);
			else if (f & 2)
				sayExe(kSeg, 0x3441, 0x2, false);
			else
				sayExe(kSeg, 0x3456, 0xa, false);
		} else {
			return false;
		}
		return true;
	case kVmtMarkers:
		// 1222:3dbf / 3cad: the markers on the girl's table
		if (verb == kVerbTag)
			sayExe(kSeg, 0x3da0);
		else if (verb == kVerbSePaa)
			sayExe(kSeg, f == 0 ? 0x3c56 : f == 1 ? 0x3c7a : 0x3c90);
		else
			return false;
		return true;
	case kVmtMarkerThing:
		// 1222:39d6: Giv tuscher (141) / farveblyanter (142) til pige 202
		if (verb != kVerbGiv || other != kIdFormGirl)
			return false;
		if (!(f & 1)) {
			say(_exe.readString(kSeg, 0x39c7) + _exe.readString(kSeg, thing(id)->pronounOff) +
				_exe.readString(kSeg, 0x39cf));
		} else if (!(_flags[kIdFormGirl] & 4)) {
			_flags[kIdFormGirl] |= 4;
			refresh(kIdFormGirl);
			_flags[kIdMarkers] = id == kIdCrayons ? 1 : 2;
			refresh(kIdMarkers);
			showPraise();
			removeFromInventory(id);
		} else {
			sayExe(kSeg, id == kIdCrayons ? 0x3989 : 0x39a5);
		}
		return true;
	case kVmtGasMask:
		// 1222:3b6d: Giv gasmaske til pige 202 is the wrong answer
		if (verb != kVerbGiv || other != kIdFormGirl)
			return false;
		if (!(_flags[kIdFormGirl] & 5)) {
			_flags[kIdFormGirl] |= 2;
			refresh(kIdFormGirl);
			removeFromInventory(id);
			showDevil();
		} else {
			sayExe(kSeg, 0x3b4c);
		}
		return true;
	case kVmtPicture:
		// 1222:1c03 / 1c89: Olga's picture on the ugly wall 140
		if ((verb != kVerbBrug && verb != kVerbGiv) || other != kIdWall)
			return false;
		f = 1;
		removeFromInventory(id);
		_flags[kIdWall] = 1;
		refresh(kIdWall);
		showPraise();
		return true;
	case kVmtWall:
		// 1222:1d50 / 1cdb
		if (verb == kVerbTag && f)
			sayExe(kSeg, 0x1d30);
		else if (verb == kVerbSePaa)
			sayExe(kSeg, f ? 0x1cc8 : 0x1cb5);
		else
			return false;
		return true;
	case kVmtPoison:
	case kVmtSalmiak:
		// 1222:3873 (Brug) / 51c4, 38cb (Giv): miljøbil, the paint shop's
		// door or the bucket are right (state 3), the sink is wrong (5);
		// giving the poison to the dog ends the game (police)
		if (verb == kVerbGiv && id == 128 && other == kIdDog) {
			removeFromInventory(id);
			_flags[kIdDog] = 4;
			refresh(kIdDog);
			_flags[kIdPaperBoy] = 4;
			refresh(kIdPaperBoy);
			if (olga)
				Scene::setAnim(*olga, 8);
			setInput(false);
			SceneObject *owner = _scene.findObject(kIdOwner);
			if (owner) {
				Scene::stopScript(*owner);
				owner->moves.push_back(move(3, 10));
				_scene.startScript(*owner, kEventOwnerShouts);
			} else {
				situationEvent({ kEventOwnerShouts, -1 });
			}
			return true;
		}
		if (verb == kVerbGiv && id == 128 && (other == kIdSister || other == 209)) {
			sayExe(kSeg, 0x51a2);
			return true;
		}
		if (verb != kVerbBrug && !(verb == kVerbGiv && (isDisposal(other) || other == kIdSink)))
			return false;
		if (isDisposal(other)) {
			// FUN_1222_364a
			if (f & 1)
				removeFromInventory(id);
			f = 3;
			refresh(id);
			if (olga)
				Scene::setAnim(*olga, 1);
			sayExe(kSeg, 0x362e);
			if (other == kIdFarvehDoor) {
				_flags[kIdFarvehDoor] ^= 1;
				refresh(kIdFarvehDoor);
			}
		} else if (other == kIdSink) {
			// Situations_3771: down the sink; the kitchen door shuts
			if (f & 1)
				removeFromInventory(id);
			f = 5;
			refresh(id);
			if (_flags[kIdKitchenDoor] == 1) {
				_flags[kIdKitchenDoor] = 0;
				refresh(kIdKitchenDoor);
			}
			showDevil();
		} else {
			return verbThing(id, verb, other);
		}
		return true;
	case kVmtLars:
		// 1222:295e (Tag; Skub and Træk call it) and Brug / Giv med gruppe 165
		if (verb == kVerbTag || verb == kVerbSkub || verb == kVerbTraek ||
			((verb == kVerbBrug || verb == kVerbGiv) && other == kIdGroup)) {
			f |= 1;
			refresh(id);
		} else if (verb == kVerbSePaa) {
			sayExe(kSeg, 0x28ec);
		} else if (verb == kVerbTalTil) {
			sayExe(kSeg, 0x2937, 0x2, false);
		} else {
			return false;
		}
		return true;
	case kVmtBagBoy:
		// Brug / Giv: the TASKVALG pop-up (1222:26a6 -> 26c0); 264a / 2684
		if (verb == kVerbBrug || verb == kVerbGiv)
			setExit(24, 1);
		else if (verb == kVerbSePaa)
			sayExe(kSeg, 0x2625);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, 0x266c, 0x2, false);
		else
			return false;
		return true;
	case kVmtLifters:
		// Brug / Giv: the LOEFT pop-up (1222:4993, 49c3); 4930 / 4971
		if (verb == kVerbBrug || verb == kVerbGiv)
			setExit(28, 7);
		else if (verb == kVerbSePaa)
			sayExe(kSeg, 0x4911);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, 0x4952, 0xc, false);
		else
			return false;
		return true;
	case kVmtGroup:
		// 1222:2c28 / 2cb4
		if (verb == kVerbSePaa)
			sayExe(kSeg, (f & 1) ? 0x2c07 : 0x2be7);
		else if (verb == kVerbTalTil && !(f & 1))
			sayExe(kSeg, 0x2c86, 0x2, false);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, 0x2c95);
		else
			return false;
		return true;
	default:
		return false;
	}
}

} // End of namespace Olga
