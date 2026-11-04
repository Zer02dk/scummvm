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
	kIdGirl = 205
};

// Handler classes (VMTs, tools/situations.py)
enum {
	kVmtWindow = 0x0356, kVmtRadiator = 0x0442, kVmtBoy = 0x031e,
	kVmtCeilingLamp = 0x0a9e, kVmtBlackboard = 0x0986, kVmtGirl = 0x09be,
	kVmtLamp = 0x0ad6, kVmtDevil = 0x047a, kVmtPraise = 0x04b2
};

// Room events (Responses_RoomEvent 5427:1454)
enum {
	kEventDevilGone = 4, kEventDevilTalk = 5, kEventDevilLaugh = 6, kEventPraiseDone = 9
};

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
	default:
		return false;
	}
}

} // End of namespace Olga
