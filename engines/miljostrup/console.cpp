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

#include "miljostrup/console.h"
#include "miljostrup/miljostrup.h"

namespace Miljostrup {

Console::Console() : GUI::Debugger() {
	registerCmd("test",   WRAP_METHOD(Console, Cmd_test));
	registerCmd("room",   WRAP_METHOD(Console, Cmd_room));
	registerCmd("door",   WRAP_METHOD(Console, Cmd_door));
	registerCmd("items",  WRAP_METHOD(Console, Cmd_items));
	registerCmd("points", WRAP_METHOD(Console, Cmd_points));
}

Console::~Console() {
}

bool Console::Cmd_test(int argc, const char **argv) {
	debugPrintf("Test\n");
	return true;
}

bool Console::Cmd_room(int argc, const char **argv) {
	if (argc < 2) {
		debugPrintf("Location %d, room %d\n", g_engine->getLocation(), g_engine->getRoom());
		debugPrintf("Usage: room <number> [entry node]\n");
		return true;
	}
	int node = argc > 2 ? atoi(argv[2]) : 1;
	if (!g_engine->gotoRoom(atoi(argv[1]), node))
		debugPrintf("Cannot go to room %s\n", argv[1]);
	return false;
}

bool Console::Cmd_door(int argc, const char **argv) {
	// Until the verbs exist: open or close doors by object ID
	Common::HashMap<uint16, bool> &doors = g_engine->getDoors();
	if (argc < 2) {
		for (auto &d : doors)
			debugPrintf("door %d: %s\n", d._key, d._value ? "open" : "closed");
		debugPrintf("Usage: door <id> open|close, or door all open\n");
		return true;
	}
	bool open = argc < 3 || !strcmp(argv[2], "open");
	for (auto &d : doors) {
		if (!strcmp(argv[1], "all") || d._key == atoi(argv[1])) {
			d._value = open;
			g_engine->updateDoor(d._key);
		}
	}
	return true;
}

bool Console::Cmd_points(int argc, const char **argv) {
	// Testing aid: show or set the location's points (the exit needs the threshold)
	if (argc > 1)
		g_engine->setPoints(atoi(argv[1]));
	debugPrintf("Location %d: %d points\n", g_engine->getLocation(), g_engine->getPoints());
	return true;
}

bool Console::Cmd_items(int argc, const char **argv) {
	// Testing aid: all carriable objects of the location into the inventory
	g_engine->giveAllItems();
	debugPrintf("All items of location %d are in the inventory\n", g_engine->getLocation());
	return true;
}

} // End of namespace Miljostrup
