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

#include "olga/console.h"
#include "olga/olga.h"

namespace Olga {

Console::Console() : GUI::Debugger() {
	registerCmd("room", WRAP_METHOD(Console, Cmd_room));
}

Console::~Console() {
}

bool Console::Cmd_room(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Location %d\n", g_engine->getLocation());
		debugPrintf("Usage: room <location 1..34> [entry]\n");
		return true;
	}
	int entry = argc > 2 ? atoi(argv[2]) : 2;
	if (!g_engine->gotoLocation(atoi(argv[1]), entry)) {
		debugPrintf("Location %s is not a room\n", argv[1]);
		return true;
	}
	return false;
}

} // End of namespace Olga
