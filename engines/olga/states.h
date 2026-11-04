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

#ifndef OLGA_STATES_H
#define OLGA_STATES_H

#include "common/scummsys.h"

namespace Olga {

enum {
	kFirstId = 100, // hotspot ids 100..231; 100 = Olga / none
	kLastId = 231,
	kNoObject = 100
};

// The handler classes (VMTs of Situations_BuildHandlers, notes/situations.md)
enum {
	kVmtBase = 0x1032,
	kVmtPassage = 0x038e,
	kVmtDoor = 0x03ca,
	kVmtCupboard = 0x0406,
	kVmtFarvehDoor = 0x08a2,
	kVmtFrontDoor = 0x0c5e  // 194, the end-of-game check
};

/** ObjNames_SetState (21b9:0131): the record at DS:1c80 + 0x26 * id. */
struct StateDef {
	int id;
	int icon;            // inventory icon 1..17, 0 = cannot be carried
	uint16 nameSeg;      // the name's Pascal string in OLGA.EXE
	uint16 nameOff;
	uint32 flags;        // the situation state at a new game
	bool twoObjects;     // Brug / Giv ask for a second object
	uint16 vmt;          // the handler's class
};

} // End of namespace Olga

#endif // OLGA_STATES_H
