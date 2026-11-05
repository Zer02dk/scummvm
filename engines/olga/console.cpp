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
	registerCmd("flag", WRAP_METHOD(Console, Cmd_flag));
	registerCmd("give", WRAP_METHOD(Console, Cmd_give));
	registerCmd("score", WRAP_METHOD(Console, Cmd_score));
	registerCmd("solve", WRAP_METHOD(Console, Cmd_solve));
}

bool Console::Cmd_score(int argc, const char **argv) {
	debugPrintf("Score %d of 20\n", g_engine->getScore());
	return true;
}

bool Console::Cmd_solve(int argc, const char **argv) {
	// The solved state of each of the 20 situations (notes/situations.md)
	static const struct { int id; uint32 value; } kSolved[] = {
		{ 104, 4 }, { 108, 1 }, { 205, 1 }, { 127, 1 }, { 202, 4 }, { 140, 1 }, { 128, 2 },
		{ 164, 2 }, { 186, 2 }, { 171, 3 }, { 174, 1 }, { 185, 1 }, { 188, 1 }, { 192, 2 },
		{ 200, 1 }, { 217, 2 }, { 218, 2 }, { 219, 1 }, { 225, 1 }, { 226, 1 }
	};
	for (uint i = 0; i < ARRAYSIZE(kSolved); i++)
		g_engine->setFlag(kSolved[i].id, kSolved[i].value);
	debugPrintf("Score %d of 20\n", g_engine->getScore());
	return true;
}

bool Console::Cmd_flag(int argc, const char **argv) {
	int id = argc > 1 ? atoi(argv[1]) : 0;
	if (id <= kNoObject || id > kLastId) {
		debugPrintf("Usage: flag <id 101..231> [value]\n");
		return true;
	}
	if (argc > 2)
		g_engine->setFlag(id, strtoul(argv[2], nullptr, 0));
	debugPrintf("S[%d] = 0x%x\n", id, g_engine->getFlag(id));
	return true;
}

bool Console::Cmd_give(int argc, const char **argv) {
	int id = argc > 1 ? atoi(argv[1]) : 0;
	if (id <= kNoObject || id > kLastId) {
		debugPrintf("Usage: give <id 101..231>\n");
		return true;
	}
	g_engine->giveThing(id);
	return true;
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
