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

// VILLA / HAVE, ENTRE / GARAGE, SLOEJD, GANG7 / KLASSE2
enum {
	kIdGate = 150, kIdMiljoebil = 161, kIdPostPlace = 166, kIdPostbox = 168,
	kIdLeash = 175, kIdCart = 191,
	kIdSpeaker = 173, kIdSpeakerGarage = 210, kIdEarMuffs = 169, kIdStandIn = 208,
	kIdPaintCupboard = 185, kIdSalmiak = 192, kIdBucket = 197, kIdPaintGirl = 199,
	kIdAlkyd = 200, kIdAkryl = 201,
	kIdHooks = 215, kIdTeacher = 216, kIdJacket = 217, kIdCoat = 218, kIdKids2 = 219,
	kIdKids2b = 220, kIdPupils = 224, kIdPupil1 = 225, kIdPupil2 = 226,
	kIdJacketHook = 227, kIdCoatHook = 228, kIdMegaphone = 229
};

enum {
	kVmtDog = 0x0b7e, kVmtPaperBoy = 0x0b46, kVmtPostPlace = 0x05ca, kVmtPostbox = 0x0592,
	kVmtLeash = 0x0bb6, kVmtOwner = 0x0c26, kVmtCart = 0x071a,
	kVmtSpeaker = 0x0cd2, kVmtSister = 0x0d42, kVmtSisterGarage = 0x0d7a,
	kVmtEarMuffs = 0x0d0a, kVmtStandIn = 0x0c9a,
	kVmtBucket = 0x0752, kVmtPaintCupboard = 0x0602, kVmtPaintGirl = 0x078a,
	kVmtAlkyd = 0x07c2, kVmtAkryl = 0x07fa,
	kVmtTeacher = 0x0e22, kVmtWetClothes = 0x0eca, kVmtKids2 = 0x0db2, kVmtPanes = 0x0e5a,
	kVmtHook = 0x0e92, kVmtMegaphone = 0x0f02, kVmtPupil = 0x0dea
};

// Room events (Responses_RoomEvent 5427:1454)
enum {
	kEventDevilGone = 4, kEventDevilTalk = 5, kEventDevilLaugh = 6,
	kEventBagBoyIn = 7, kEventLarsAtGroup = 8, kEventPraiseDone = 9,
	kEventPaperBoyGone = 0xa, kEventCartParked = 0xb,
	kEventLarsWalks = 0xc, kEventLiftingDone = 0xd,
	kEventPaperBoyWalks = 0xe, kEventOwnerTakesDog = 0xf,
	kEventOwnerShouts = 0x10, kEventPolice = 0x11,
	kEventStandInRefresh = 0x12, kEventStandInDone = 0x13,
	kEventSpeakerCarried = 0x14, kEventClassQuiet = 0x15
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
			setControl(id, false);
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
			setControl(id, false);
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
	case kVmtLeash:
	case kVmtTeacher:
		// 1222:3eea
		Scene::setAnim(o, (f & ~0x100) + 1);
		return true;
	case kVmtDog:
	case kVmtPaperBoy: {
		// 1222:4da5 / 4cf6: animation state + 1, the hotspot on in states 0,
		// 1 and 4; the leash 175 is on the dog in state 1. The paper boy in
		// state 2 (the dog is in) sets off at once (4c28): both 3, his
		// animation 6 for 4 steps, then event 0xe; the garden gate 150
		// toggles and the room waits
		int s = f & 7;
		if (s > 4)
			return true;
		if (id == kIdPaperBoy && s == 2) {
			_flags[kIdPaperBoy] = 3;
			_flags[kIdDog] = 3;
			Scene::stopScript(o);
			o.moves.push_back(move(6, 4));
			_scene.startScript(o, kEventPaperBoyWalks);
			setInput(false);
			_flags[kIdGate] ^= 1;
			refresh(kIdGate);
		} else {
			Scene::setAnim(o, s + 1);
			setControl(id, s == 0 || s == 1 || s == 4);
		}
		_flags[kIdLeash] = s == 1 ? 1 : 0;
		refresh(kIdLeash);
		return true;
	}
	case kVmtPostPlace:
		// 1222:2077: the empty place 2 (state 0) or 3 (2), the postbox
		// hanging there 1 (1); renamed
		if (f > 2)
			return true;
		Scene::setAnim(o, f == 1 ? 1 : f == 2 ? 3 : 2);
		_names[id] = _exe.readString(kSeg, f == 1 ? 0x206d : 0x2063);
		return true;
	case kVmtCart:
		// 1222:2d12: parked 3 (bit 1), not there 1; bit 0 alone: it drives
		// 312 px right in 52 steps (animation 2), then event 0xb
		if (f & 2) {
			Scene::setAnim(o, 3);
		} else if (!(f & 1)) {
			Scene::setAnim(o, 1);
		} else {
			_flags[id] = 2;
			Scene::stopScript(o);
			SceneObject::Move drive = { 0x138, 0, 2, 0x34 };
			o.moves.push_back(drive);
			_scene.startScript(o, kEventCartParked);
		}
		return true;
	case kVmtOwner:
		// 1222:5467
		Scene::setAnim(o, 1);
		return true;
	case kVmtSpeaker: {
		// 1222:5c27: 173 is in ENTRE, 210 in the GARAGE (S[173] bit 1); the
		// one in this room plays (1) or stands still (3: turned down, bit 0,
		// or being carried, bit 2); the other is hidden (2), hotspot off
		bool here = ((_flags[kIdSpeaker] & 2) == 0) == (id == kIdSpeaker);
		if (here)
			Scene::setAnim(o, (_flags[kIdSpeaker] & 5) ? 3 : 1);
		else
			Scene::setAnim(o, 2);
		setControl(id, here);
		return true;
	}
	case kVmtSister:
		// 1222:607d: in the garage (bit 1): hidden, hotspot off
		Scene::setAnim(o, (f & 2) ? 2 : 1);
		setControl(id, !(f & 2));
		return true;
	case kVmtSisterGarage:
		// 1222:61bd: the sister in the GARAGE follows S[174] bit 1
		Scene::setAnim(o, (_flags[kIdSister] & 2) ? 1 : 2);
		setControl(id, (_flags[kIdSister] & 2) != 0);
		return true;
	case kVmtStandIn: {
		// 1222:58b2: "Snyde Olga" plays Olga's special animations in her
		// place (her position without her animation offset); Olga is hidden
		// meanwhile (animation 9)
		if (olga) {
			int dx = 0, dy = 0;
			if (olga->anim >= 1) {
				dx = olga->def.anims[olga->anim - 1].dx;
				dy = olga->def.anims[olga->anim - 1].dy;
			}
			Scene::setPosition(o, olga->x - dx, olga->y - dy);
		}
		if (f == 1) {
			// 55d3: the ear muffs while the music plays: the devil, then
			// animation 2 for 34 steps and event 0x13
			showDevil();
			if (olga)
				Scene::setAnim(*olga, 9);
			Scene::stopScript(o);
			o.moves.push_back(move(2, 0x22));
			_scene.startScript(o, kEventStandInDone);
			setInput(false);
		} else if (f == 2 || f == 3) {
			// 56fb: carrying the loudspeaker (animation 3, or 4), one step,
			// then event 0x14
			int n = f == 2 ? 3 : 4;
			if (olga)
				Scene::setAnim(*olga, 9);
			_flags[id] = 0;
			Scene::setAnim(o, n);
			Scene::stopScript(o);
			o.moves.push_back(move(n, 1));
			_scene.startScript(o, kEventSpeakerCarried);
		} else if (f == 4) {
			// 57ac: the megaphone (animation 5) while she shouts, length *
			// text speed / 3 steps, then event 0x15
			Common::String text = _exe.readString(kSeg, 0x579d);
			if (olga)
				Scene::setAnim(*olga, 9);
			setInput(false);
			_flags[id] = 0;
			Scene::setAnim(o, 5);
			Scene::stopScript(o);
			o.moves.push_back(move(5, text.size() * _textSpeed / 3));
			_scene.startScript(o, kEventClassQuiet);
			say(text, 0xc, false);
		} else {
			// 567e: put away; Olga back, the room takes input
			Scene::setPosition(o, 0, 0);
			Scene::setAnim(o, 1);
			if (olga)
				Scene::setAnim(*olga, 1);
			setInput(true);
		}
		return true;
	}
	case kVmtPaintGirl:
		// 1222:2f2d: well again (bit 0) 2, green in the face 1
		Scene::setAnim(o, (f & 1) ? 2 : 1);
		return true;
	case kVmtAlkyd:
	case kVmtAkryl:
		// 1222:307c: in the bucket (bit 0): 2, hotspot off
		Scene::setAnim(o, (f & 1) ? 2 : 1);
		setControl(id, !(f & 1));
		return true;
	case kVmtKids2:
		// 1222:6361: all three follow S[219]
		Scene::setAnim(o, _flags[kIdKids2] + 1);
		return true;
	case kVmtPupil:
		// 1222:661a: fidgeting (1) with the hotspot off while the class is
		// noisy; once quiet the hotspot is on, 2 on the wrong furniture,
		// 3 when his own state says he sits right (BordStol)
		setControl(id, _flags[kIdKids2] != 0);
		Scene::setAnim(o, !_flags[kIdKids2] ? 1 : _flags[id] ? 3 : 2);
		return true;
	case kVmtPanes:
		// 1222:6888: clear (2) when the jacket and the coat both hang
		Scene::setAnim(o, ((_flags[kIdJacket] & 2) && (_flags[kIdCoat] & 2)) ? 2 : 1);
		return true;
	case kVmtHook:
		// 1222:69b5: the jacket / coat on the hook (state 1) 1, else 2 with
		// the hotspot off
		Scene::setAnim(o, (f & 1) ? 1 : 2);
		setControl(id, (f & 1) != 0);
		return true;
	case kVmtLars:
		// 1222:2a91: with the group (bit 1): animation 3, his hotspot off;
		// taken (bit 0): he starts to walk (event 0xc), the room waits
		if (f & 2) {
			Scene::setAnim(o, 3);
			setControl(id, false);
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
	case kEventPaperBoyWalks: {
		// Responses_0f3b: he walks 258 px right in 43 steps (animation 3),
		// then event 0xa; the cart 191 drives off; he thanks Olga
		SceneObject *boy = _scene.findObject(kIdPaperBoy);
		if (boy) {
			Scene::stopScript(*boy);
			boy->moves.push_back(move(6, 1));
			SceneObject::Move walk = { 0x102, 0, 3, 0x2b };
			boy->moves.push_back(walk);
			_scene.startScript(*boy, kEventPaperBoyGone);
		}
		_flags[kIdCart] |= 1;
		refresh(kIdCart);
		sayExe(kSegResponses, 0x0f24, 0x2, false);
		break;
	}
	case kEventPaperBoyGone: {
		// Responses_0ffe: both 3 (solved), the paper boy put at 50,50
		_flags[kIdDog] = 3;
		refresh(kIdDog);
		_flags[kIdPaperBoy] = 3;
		SceneObject *boy = _scene.findObject(kIdPaperBoy);
		if (boy)
			Scene::setPosition(*boy, 0x32, 0x32);
		refresh(kIdPaperBoy);
		break;
	}
	case kEventCartParked:
		// Responses_1050
		setInput(true);
		refresh(kIdCart);
		break;
	case kEventOwnerTakesDog:
		// Responses_1199: the owner took the dog in; on to VILLA, entry 5,
		// where the paper boy sets off
		_flags[kIdDog] = 2;
		_flags[kIdPaperBoy] = 2;
		setExit(16, 5);
		break;
	case kEventStandInRefresh:
		// Responses_12d8
		refresh(kIdStandIn);
		break;
	case kEventStandInDone:
		// Responses_12ec
		_flags[kIdStandIn] = 0;
		refresh(kIdStandIn);
		break;
	case kEventSpeakerCarried:
		// Responses_1308: the loudspeaker and the sister change rooms; to the
		// GARAGE (entry 2) is wrong (the devil there), back to ENTRE (entry 2)
		_flags[kIdSpeaker] &= ~4;
		_flags[kIdSpeaker] ^= 2;
		_flags[kIdSister] ^= 2;
		if (_flags[kIdSpeaker] & 2) {
			setExit(20, 2);
			_pendingDevil = true;
		} else {
			setExit(19, 2);
		}
		break;
	case kEventClassQuiet:
		// Responses_13b9: the class is quiet (219 = 1), the teacher 216
		// teaches, praise
		_flags[kIdStandIn] = 0;
		refresh(kIdStandIn);
		_flags[kIdKids2] = 1;
		refresh(kIdKids2);
		refresh(kIdKids2b);
		refresh(kIdPupils);
		refresh(kIdPupil1);
		refresh(kIdPupil2);
		_flags[kIdTeacher] = 1;
		refresh(kIdTeacher);
		showPraise();
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
	case kVmtDog:
		// 1222:4f95 / 502c
		if (verb == kVerbSePaa)
			sayExe(kSeg, (f == 3 || f == 4) ? 0x4f4b : 0x4f6c);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, 0x5011, 0xc, false);
		else
			return false;
		return true;
	case kVmtPaperBoy:
		// 1222:4e55 / 4ead; score 4f0b: state 3
		if (verb == kVerbSePaa)
			sayExe(kSeg, 0x4e2d);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, (f & 7) == 0 ? 0x4e77 : 0x4e96, 0xc, false);
		else
			return false;
		return true;
	case kVmtPostPlace:
		// 1222:200a (Tag: the base verb unless the postbox hangs there) /
		// 1f8c
		if (verb == kVerbTag && f == 1) {
			sayExe(kSeg, 0x1fea);
		} else if (verb == kVerbSePaa) {
			if (f == 1)
				sayExe(kSeg, 0x1f73);
			else if (f == 0 || f == 2)
				sayExe(kSeg, 0x1f57);
		} else {
			return false;
		}
		return true;
	case kVmtPostbox:
		// 1222:1ea3 (Brug; Giv calls it, 19dc): put back on its place 166 in
		// VILLA it hangs there (166 = 1; this one stays hidden, state 1), and
		// the paper boy, if not yet on his way, goes (171 = 133 = 2)
		if ((verb != kVerbBrug && verb != kVerbGiv) || other != kIdPostPlace)
			return false;
		verbThing(id, kVerbBrug, other);
		f = 1;
		refresh(id);
		_flags[kIdPostPlace] = 1;
		refresh(kIdPostPlace);
		if (_flags[kIdPaperBoy] < 2) {
			_flags[kIdPaperBoy] = 2;
			refresh(kIdPaperBoy);
			_flags[kIdDog] = 2;
			refresh(kIdDog);
		}
		return true;
	case kVmtLeash:
		// 1222:50e0 (Brug; Giv calls it, 5098): the leash on the dog is wrong
		if ((verb == kVerbBrug || verb == kVerbGiv) && other == kIdDog) {
			if (f == 0) {
				f = 1;
				refresh(id);
				_flags[kIdDog] = 1;
				refresh(kIdDog);
				_flags[kIdPaperBoy] = 1;
				refresh(kIdPaperBoy);
				showDevil();
			} else {
				sayExe(kSeg, 0x50c4);
			}
		} else if (verb == kVerbSePaa) {
			sayExe(kSeg, 0x504e);
		} else {
			return false;
		}
		return true;
	case kVmtOwner:
		// 1222:52fa / 535a: Olga tells the owner (the line stays while she
		// talks, text speed * 12 frames), then event 0xf; he comes to the door
		if (verb == kVerbSePaa) {
			sayExe(kSeg, 0x52e4);
		} else if (verb == kVerbTalTil) {
			if ((_flags[kIdDog] & 7) == 3) {
				sayExe(kSeg, 0x531c);
			} else {
				showText(_exe.readString(kSeg, 0x5335), 0xe);
				f = 1;
				setInput(false);
				if (olga) {
					Scene::stopScript(*olga);
					olga->moves.push_back(move(7, _textSpeed * 0x24 / 3));
					_scene.startScript(*olga, kEventOwnerTakesDog);
				}
				SceneObject *owner = _scene.findObject(id);
				if (owner)
					Scene::setAnim(*owner, 2);
			}
		} else {
			return false;
		}
		return true;
	case kVmtSpeaker:
		// 1222:5a9f (Skub; Træk calls it), 5b54 (Brug; Luk calls it), 59bf
		if (verb == kVerbSkub || verb == kVerbTraek) {
			if (_flags[kIdSpeaker] & 1) {
				sayExe(kSeg, 0x5a85);
			} else if (_scene.findObject(kIdStandIn)) {
				// 5a10: Olga carries it away (208 = 2, then event 0x14)
				_flags[kIdStandIn] = 2;
				refresh(kIdStandIn);
				_flags[kIdSpeaker] |= 4;
				refresh(id);
				setInput(false);
			}
		} else if (verb == kVerbBrug || verb == kVerbLuk) {
			// The VOLUME pop-up (location 29, entry 1)
			if (id == kIdSpeakerGarage)
				sayExe(kSeg, 0x5b0a);
			else if (f & 1)
				sayExe(kSeg, 0x5b30);
			else
				setExit(29, 1);
		} else if (verb == kVerbSePaa) {
			sayExe(kSeg, (_flags[kIdSpeaker] & 1) ? 0x59a3 : 0x5989);
		} else {
			return false;
		}
		return true;
	case kVmtSister:
		// 1222:5ed4 / 5fdd
		if (verb == kVerbSePaa) {
			sayExe(kSeg, f == 0 ? 0x5e79 : (f & 2) ? 0x5e91 : 0x5eb8);
		} else if (verb == kVerbTalTil) {
			if (f == 0)
				sayExe(kSeg, 0x5f74, 0xb, false);
			else
				sayExe(kSeg, (f & 2) ? 0x5f9b : 0x5fbe, 0x2, false);
		} else {
			return false;
		}
		return true;
	case kVmtSisterGarage:
		// 1222:6156 / 619b
		if (verb == kVerbSePaa)
			sayExe(kSeg, 0x612f);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, 0x6178, 0x2, false);
		else
			return false;
		return true;
	case kVmtEarMuffs:
		// 1222:5dd5: wearing them while the loudspeaker plays loud in this
		// room is wrong (208 = 1: the devil); Giv is the base verb (5e65)
		if (verb == kVerbBrug) {
			if (f == 0)
				sayExe(kSeg, 0x5d9d);
			else if ((_scene.findObject(kIdSister) && (_flags[kIdSpeaker] & 3) == 0) ||
					 (_scene.findObject(kIdSisterGarage) && (_flags[kIdSpeaker] & 3) == 2)) {
				_flags[kIdStandIn] = 1;
				refresh(kIdStandIn);
			} else {
				sayExe(kSeg, 0x5dae);
			}
		} else if (verb == kVerbGiv) {
			verbBase(verb);
		} else if (verb == kVerbSePaa) {
			sayExe(kSeg, 0x5ceb);
		} else {
			return false;
		}
		return true;
	case kVmtBucket:
		// 1222:24cd (Brug), 2568 (Giv), 23bc (Tag); score 235e: -1 while it
		// is carried
		if (verb == kVerbBrug || (verb == kVerbGiv && (other == kIdMiljoebil || other == kIdFarvehDoor))) {
			if (other == kIdPaintCupboard) {
				// 243c: the KemSkab screen (location 23, entry 8)
				if (!(_flags[kIdPaintCupboard] & 1))
					setExit(23, 8);
				else
					sayExe(kSeg, 0x2420);
			} else if (other == kIdAlkyd || other == kIdAkryl || other == kIdSalmiak) {
				return verbSituation(other, kVerbBrug, id);
			} else if (other == kIdMiljoebil || other == kIdFarvehDoor) {
				sayExe(kSeg, 0x24a7);
			} else {
				return false;
			}
		} else if (verb == kVerbTag) {
			bool wasCarried = (f & 1) != 0;
			verbThing(id, verb, other);
			if (!wasCarried)
				sayExe(kSeg, 0x2398);
		} else {
			return false;
		}
		return true;
	case kVmtPaintCupboard:
		// 1222:2220 / 2251: Brug / Giv med spand go to the bucket's verb;
		// 21c2, 22c0; KemSkab sets bit 0
		if ((verb == kVerbBrug || verb == kVerbGiv) && other == kIdBucket) {
			if (!verbSituation(kIdBucket, verb, id) && !verbThing(kIdBucket, verb, id))
				verbBase(verb);
		} else if (verb == kVerbTag) {
			sayExe(kSeg, (f & 1) ? 0x21a8 : 0x2181);
		} else if (verb == kVerbSePaa) {
			sayExe(kSeg, (f & 1) ? 0x22ab : 0x2282);
		} else {
			return false;
		}
		return true;
	case kVmtPaintGirl:
		// 1222:2e32 / 2ecf
		if (verb == kVerbSePaa)
			sayExe(kSeg, (f & 1) ? 0x2e1d : 0x2e01);
		else if (verb == kVerbTalTil && (f & 1))
			sayExe(kSeg, 0x2eb2, 0x2, false);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, 0x2e90, 0xa, false);
		else
			return false;
		return true;
	case kVmtAlkyd:
	case kVmtAkryl:
		// 1222:2f86 (Brug; Giv calls it, 3050): into the bucket. The alkyd
		// paint with the black cross is right (the girl 199 recovers), the
		// water based akryl paint is wrong (score -1 while in the bucket)
		if ((verb == kVerbBrug || verb == kVerbGiv) && other == kIdBucket) {
			f |= 1;
			refresh(id);
			if (id == kIdAlkyd) {
				showPraise();
				_flags[kIdPaintGirl] |= 1;
				refresh(kIdPaintGirl);
			} else {
				showDevil();
			}
		} else if (verb == kVerbTag) {
			sayExe(kSeg, id == kIdAlkyd ? 0x3146 : 0x3208);
		} else if (verb == kVerbSePaa) {
			sayExe(kSeg, id == kIdAlkyd ? 0x30fb : 0x31c3);
		} else {
			return false;
		}
		return true;
	case kVmtTeacher:
		// 1222:671b / 679d
		if (verb == kVerbSePaa)
			sayExe(kSeg, _flags[kIdKids2] ? 0x66f5 : 0x66d2);
		else if (verb == kVerbTalTil)
			sayExe(kSeg, _flags[kIdKids2] ? 0x6786 : 0x6763, 0x2, false);
		else
			return false;
		return true;
	case kVmtWetClothes: {
		// 1222:6a3a (Brug; Giv calls it, 6b08): on the coat hooks 215 in
		// GANG7: hung up (bit 1), the hook 227 / 228 shows it, praise
		if ((verb != kVerbBrug && verb != kVerbGiv) || other != kIdHooks)
			return false;
		removeFromInventory(id);
		f |= 2;
		int hook = id == kIdJacket ? kIdJacketHook : kIdCoatHook;
		_flags[hook] = 1;
		refresh(hook);
		showPraise();
		return true;
	}
	case kVmtKids2:
		// 1222:6266 / 62ec: only the megaphone 229 is loud enough (208 = 4)
		if (verb == kVerbSePaa) {
			sayExe(kSeg, _flags[kIdKids2] ? 0x624e : 0x622f);
		} else if (verb == kVerbTalTil) {
			if (_flags[kIdKids2]) {
				sayExe(kSeg, 0x62c9);
			} else if (carried(kIdMegaphone)) {
				_flags[kIdStandIn] = 4;
				refresh(kIdStandIn);
			} else {
				sayExe(kSeg, 0x62ae);
			}
		} else {
			return false;
		}
		return true;
	case kVmtPanes:
		// 1222:6825
		if (verb != kVerbSePaa)
			return false;
		sayExe(kSeg, ((_flags[kIdJacket] & 2) && (_flags[kIdCoat] & 2)) ? 0x680d : 0x67e5);
		return true;
	case kVmtHook:
		// 1222:6904 / 696b. Se på compares the hook's own id with 218, so
		// both hooks say "Nu kan jakken hænge og tørre her."
		if (verb == kVerbTag)
			sayExe(kSeg, 0x68e6);
		else if (verb == kVerbSePaa)
			sayExe(kSeg, id == kIdCoat ? 0x6926 : 0x6949);
		else
			return false;
		return true;
	case kVmtMegaphone:
		// 1222:6b73: Brug megafon med børn 219 is Tal til børn
		if (verb == kVerbBrug && other == kIdKids2)
			return verbSituation(kIdKids2, kVerbTalTil, kNoObject);
		return false;
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
