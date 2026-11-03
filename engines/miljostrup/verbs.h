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

#ifndef MILJOSTRUP_VERBS_H
#define MILJOSTRUP_VERBS_H

#include "common/file.h"
#include "common/str.h"

namespace Miljostrup {

enum {
	kVerbCount = 12
};

/** A class of game objects: its verb handlers (VMT slots 0x28..0x54). */
struct ObjectClass {
	uint16 vmt;
	byte kind[kVerbCount];       // HandlerKind
	uint32 handler[kVerbCount];  // Ghidra seg << 16 | off
	uint32 stateRule;            // VMT 0x58: sets the figure's state
};

struct HandlerText {
	uint32 address;  // Ghidra seg << 16 | off of the literal
	uint32 offset;   // OVR file offset
};

struct SuperWaste {
	int object;
	uint32 definite;  // OVR offset
	int container;
	int value;
};

struct ExitDoor {
	int location;
	int object;
	bool open;
};

struct StartFlag {
	uint16 id;
	byte field;      // offset in the object (+0x2f the main state)
	int16 value;     // at the start (a word for the lamps)
};

struct FarmPile {
	uint16 id;       // the pile (784c:0911)
	uint16 item;     // the object you take from it
	byte count;      // at the start
	uint32 texts[9]; // pronoun, name, text (OVR offsets) for 0, 1 and more
};

struct ProxyObject {
	byte location;
	uint16 id;
	uint16 other;    // the object it stands for
	byte shown;      // +0x2f at the start: figure and region shown
};

/** A location's points needed to leave it (manager +0) and maxima (+4..+10). */
struct LocationTarget {
	byte location;
	int16 threshold, maxPoints, maxEnergy, maxWaste, maxWater;
};

/** A text record of the washing machine's award (9909:0184). */
struct WashingRecord {
	uint32 good, bad;   // EXE offsets of the two texts (Pascal strings)
	int16 points, energy, water;
};

struct ShopItem {
	byte location;
	uint16 id;       // the goods on the shelf (a proxy of `goods`)
	uint16 goods;    // what the player gets
	uint16 counter;  // the proxy shown on the counter once fetched
	int16 price;     // +0x36
	int16 x;         // +0x3a: where the assistant walks to
	byte pose;       // +0x3c: his figure state when he reaches for it
	uint32 text;     // +0x3e: OVR offset of the description
	byte bulb;       // bulbs: the shelf keeps them, the counter counts them
};

struct WasteItem {
	byte location;
	uint16 id;
	uint16 bin;          // correct container (+0x12f)
	uint32 definite;     // OVR offset of the definite name (+0x131)
	uint32 note;         // OVR offset of the extra note (+0x231), 0 for none
	uint32 page;         // OVR offset of the page (+0x331, school), 0 for none
};

/** A location's praise and scolding for the waste sorting (EXE offsets). */
struct WasteTexts {
	byte location;
	uint32 praise[5];
	uint32 scolding[4];
};

struct WasteQuiz {
	byte location;
	uint16 item;
	uint32 question, answer1, answer2; // OVR offsets
	byte correct;                      // 1 = the first answer, 0 = the second
};

struct SimpleHandler {
	uint32 address;  // Ghidra seg << 16 | off of the handler
	uint32 text;     // OVR offset of the text it says
	byte color;
	byte talk;
};

struct StartItem {
	byte location;
	uint16 id;
};

struct ObjectInfo {
	byte location;
	uint16 id;
	uint16 classIndex;
	uint32 description;  // OVR offset of the object's text, 0 for none
	uint32 pronoun;      // OVR offset of "den", "det", ...
	byte icon;           // inventory icon (root +0x0e), 0 = none
};

struct ObjectRef {
	byte location;
	uint16 id;
};

const ObjectInfo *findObject(int location, int id);
/** The location's objects with an inventory icon (root +0x0e != 0). */
void getCarriableObjects(int location, Common::Array<uint16> &out);
const ObjectClass &getObjectClass(const ObjectInfo *object);
/** Kind of handler (HandlerKind) that runs verb 0x32..0x3d on the object. */
int getHandlerKind(const ObjectInfo *object, int verb);
uint32 getHandlerAddress(const ObjectInfo *object, int verb);

/** Text of the root class's default for a verb; empty for Tal til. */
uint32 getRootMessageOffset(int verb);

/** Pascal string at a file offset. */
Common::String readPascalString(Common::File &file, uint32 offset);
/** 232c:0000: first letter in upper case, also æ ø å (code page 865). */
Common::String capitalize(const Common::String &s);

} // End of namespace Miljostrup

#endif // MILJOSTRUP_VERBS_H
