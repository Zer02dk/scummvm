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

#ifndef OLGA_LOCATIONS_H
#define OLGA_LOCATIONS_H

#include "common/scummsys.h"

namespace Olga {

// The location types of Main_BuildLocations, one per VMT (notes/rooms.md)
enum LocationKind {
	kLocRoom,       // VMT DS:0002: a room with Olga
	kLocPopup,      // DS:000e: a close-up with a title and answer buttons
	kLocTextSpeed,  // DS:001a: SkiftTid, the text speed (F4)
	kLocKemSkab,    // DS:0026: the chemicals cabinet
	kLocSaveLoad,   // DS:0032: 25 save, 26 load
	kLocQuitMenu,   // DS:003e: F10
	kLocBordStol,   // DS:004a: table and chair
	kLocGameEnd     // DS:0056: the end
};

enum {
	kMaxEntries = 6,
	kStartLocation = 9, // Main_NewGame: GANG5, entry 3
	kStartEntry = 3,
	kNumLocations = 34
};

struct LocationDef {
	int number;
	LocationKind kind;
	const char *lbm;       // room picture (pop-up: its picture)
	const char *scn;
	const char *spriteLbm; // Olga's sprite file, "" for none
	const char *prs;
	const char *title;     // pop-ups only (UTF-8)
	int numEntries;
	struct { int16 x, y; } entries[kMaxEntries]; // Olga's base position, entry 1 = where she was
};

struct DoorExit {
	int hotspot;
	int location;
	int entry;
};

} // End of namespace Olga

#endif // OLGA_LOCATIONS_H
