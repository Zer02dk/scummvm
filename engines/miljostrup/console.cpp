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
#include "miljostrup/scene.h"

namespace Miljostrup {

Console::Console() : GUI::Debugger() {
	registerCmd("test",   WRAP_METHOD(Console, Cmd_test));
	registerCmd("room",   WRAP_METHOD(Console, Cmd_room));
	registerCmd("door",   WRAP_METHOD(Console, Cmd_door));
	registerCmd("items",  WRAP_METHOD(Console, Cmd_items));
	registerCmd("points", WRAP_METHOD(Console, Cmd_points));
	registerCmd("figure", WRAP_METHOD(Console, Cmd_figure));
	registerCmd("pic",    WRAP_METHOD(Console, Cmd_pic));
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

bool Console::Cmd_figure(int argc, const char **argv) {
	// Viewing aid: play any state of a room figure, also the ones the
	// original never selects (the father's 2..4, notes/formats.md)
	Scene *scene = g_engine->getScene();
	if (!scene || !scene->getRoom()) {
		debugPrintf("No room\n");
		return true;
	}
	const RoomInfo *room = scene->getRoom();
	if (argc < 2) {
		for (uint i = 0; i < scene->getFigureCount(); ++i) {
			const Figure &fig = scene->getFigureAt(i);
			debugPrintf("#%d object %d%s: state %d at (%d,%d)%s\n", i, fig.info->object,
				fig.info->player ? " (player)" : "", fig.state, fig.x, fig.y, fig.visible ? "" : " hidden");
			for (int k = 0; k < fig.info->frameCount; ++k) {
				const FigureFrame &f = getFigureFrame(*fig.info, k);
				uint len = getSequenceLength(room, f.sequence);
				Common::String images;
				for (uint n = 0; n < len; ++n)
					images += Common::String::format(" %d", getSequenceImage(room, f.sequence, n));
				debugPrintf("    state %d: sequence %d, offset (%d,%d), images%s\n", f.state, f.sequence,
					f.dx, f.dy, images.c_str());
			}
		}
		debugPrintf("Usage: figure <#> <state>, or figure <#> walk <state> <dx> <dy> <steps>\n");
		return true;
	}
	uint index = atoi(argv[1]);
	if (index >= scene->getFigureCount()) {
		debugPrintf("No figure #%s\n", argv[1]);
		return true;
	}
	if (argc == 3) {
		scene->debugSetState(index, atoi(argv[2]));
		return false;
	}
	if (argc == 7 && !strcmp(argv[2], "walk")) {
		scene->debugWalk(index, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
		return false;
	}
	debugPrintf("Usage: figure <#> <state>, or figure <#> walk <state> <dx> <dy> <steps>\n");
	return true;
}

bool Console::Cmd_pic(int argc, const char **argv) {
	// Viewing aid: an archive picture over the room until the next click or key
	if (argc < 2 || argc == 3) {
		debugPrintf("Usage: pic <1..1512> [x y] (screen coordinates, default 0 8)\n");
		return true;
	}
	int x = argc > 3 ? atoi(argv[2]) : 0;
	int y = argc > 3 ? atoi(argv[3]) : 8;
	g_engine->showDebugPicture(atoi(argv[1]), x, y);
	return false;
}

} // End of namespace Miljostrup
