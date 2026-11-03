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

// ScummVM save games (notes/engine.md "Save games"). An engine addition: the
// original only keeps the status file MILJØGEM.NNN per phase. A save holds
// the location's whole state: the objects' fields, the doors, the inventory,
// the location's score, the phase and the game's totals, the room and the
// player's node. Saving is allowed only while the room waits for input and
// nothing runs (no walk, award, close-up or animation), so the running
// animations never need to be stored; loading builds the room again, as
// the original does after an award.

#include "common/serializer.h"
#include "common/translation.h"

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"

namespace Miljostrup {

enum {
	kSaveVersion = 1
};

bool MiljostrupEngine::isGameIdle() const {
	// The room loop waits for input (_canSaveNow is set only around its
	// pollEvent, where the main menu opens) and nothing is running
	return _canSaveNow && _scene && _scene->getRoom() && !_award.active && !_inputOff && !_phaseDone &&
		!_scene->isWalking() && !_hosing && !_bagging && !_brotherWalking && _lineWaits.empty() &&
		_greenhouseStage == 0 && _gymStage == 0 && _batStage == 0 && _toasterTimer == 0 && _shopFetching == 0;
}

bool MiljostrupEngine::canSaveGameStateCurrently(Common::U32String *msg) {
	return isGameIdle();
}

bool MiljostrupEngine::canLoadGameStateCurrently(Common::U32String *msg) {
	// From the launcher (before the game has started) or while idle
	return !_scene || isGameIdle();
}

void MiljostrupEngine::syncGame(Common::Serializer &s) {
	s.syncAsSint16LE(_user);
	s.syncAsSint16LE(_phase);
	s.syncAsSint16LE(_startPhase);
	for (int k = 0; k < 8; ++k)
		s.syncAsSint32LE(_totals[k]);
	s.syncAsSint16LE(_location);
	s.syncAsSint16LE(_room);
	s.syncAsSint16LE(_loadedNode);
	for (int k = 0; k < 4; ++k)
		s.syncAsSint32LE(_score[k]);

	// the objects' fields
	uint32 count = _flags.size();
	s.syncAsUint32LE(count);
	if (s.isSaving()) {
		for (Common::HashMap<uint32, int>::const_iterator i = _flags.begin(); i != _flags.end(); ++i) {
			uint32 key = i->_key;
			int32 value = i->_value;
			s.syncAsUint32LE(key);
			s.syncAsSint32LE(value);
		}
	} else {
		_flags.clear();
		for (uint32 k = 0; k < count; ++k) {
			uint32 key = 0;
			int32 value = 0;
			s.syncAsUint32LE(key);
			s.syncAsSint32LE(value);
			_flags[key] = value;
		}
	}

	// the doors
	count = _doors.size();
	s.syncAsUint32LE(count);
	if (s.isSaving()) {
		for (Common::HashMap<uint16, bool>::const_iterator i = _doors.begin(); i != _doors.end(); ++i) {
			uint16 door = i->_key;
			byte open = i->_value ? 1 : 0;
			s.syncAsUint16LE(door);
			s.syncAsByte(open);
		}
	} else {
		_doors.clear();
		for (uint32 k = 0; k < count; ++k) {
			uint16 door = 0;
			byte open = 0;
			s.syncAsUint16LE(door);
			s.syncAsByte(open);
			_doors[door] = open != 0;
		}
	}

	// the inventory
	count = _inventory.size();
	s.syncAsUint32LE(count);
	if (s.isLoading())
		_inventory.resize(count);
	for (uint32 k = 0; k < count; ++k)
		s.syncAsUint16LE(_inventory[k]);
	s.syncAsSint16LE(_inventoryPage);

	// names set by the game (the emptied basket's "kurv", ...)
	_interface->syncNameOverrides(s);
}

Common::Error MiljostrupEngine::saveGameStream(Common::WriteStream *stream, bool isAutosave) {
	Common::Serializer s(nullptr, stream);
	s.setVersion(kSaveVersion);
	byte version = kSaveVersion;
	s.syncAsByte(version);
	_loadedNode = _scene ? _scene->getPlayerNode() : 0;
	syncGame(s);
	return Common::kNoError;
}

Common::Error MiljostrupEngine::loadGameStream(Common::SeekableReadStream *stream) {
	Common::Serializer s(stream, nullptr);
	byte version = 0;
	s.syncAsByte(version);
	if (version == 0 || version > kSaveVersion)
		return Common::Error(Common::kUnknownError, _("Unsupported save game version"));
	s.setVersion(version);
	syncGame(s);
	if (stream->err())
		return Common::kReadingFailed;
	// The room loop ends; the phase loop builds the saved room (run())
	_award = PendingAward();
	_inputOff = false;
	_messageTimer = 0;
	_loadedGame = true;
	_phaseDone = true;
	return Common::kNoError;
}

void MiljostrupEngine::enterLoadedRoom() {
	// After a load: the location's state is the saved one (enterLocation
	// would reset it); the room is built again with the player at his node
	_loadedGame = false;
	_interface->setLocation(_location);
	_interface->setObject(kNoObject);
	_interface->drawInventory();
	// gotoRoom calls the leave handler of the room it leaves; there is none
	// (the shop would put the basket back)
	int room = _room;
	_room = 0;
	gotoRoom(room, _loadedNode);
	_interface->setCursor(Interface::kCursorArrow);
}

} // End of namespace Miljostrup
