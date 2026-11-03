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

#include "common/util.h"

#include "miljostrup/interface.h"
#include "miljostrup/verbs.h"
#include "miljostrup/verbs_data.h"

namespace Miljostrup {

const ObjectInfo *findObject(int location, int id) {
	for (uint i = 0; i < ARRAYSIZE(kObjects); ++i) {
		if (kObjects[i].location == location && kObjects[i].id == id)
			return &kObjects[i];
	}
	return nullptr;
}

void getCarriableObjects(int location, Common::Array<uint16> &out) {
	for (uint i = 0; i < ARRAYSIZE(kObjects); ++i) {
		if (kObjects[i].location == location && kObjects[i].icon)
			out.push_back(kObjects[i].id);
	}
}

const ObjectClass &getObjectClass(const ObjectInfo *object) {
	return kObjectClasses[object->classIndex];
}

int getHandlerKind(const ObjectInfo *object, int verb) {
	if (!object || verb < kVerbFirst || verb > kVerbLast)
		return kHandlerUnknown;
	return getObjectClass(object).kind[verb - kVerbFirst];
}

uint32 getHandlerAddress(const ObjectInfo *object, int verb) {
	if (!object || verb < kVerbFirst || verb > kVerbLast)
		return 0;
	return getObjectClass(object).handler[verb - kVerbFirst];
}

uint32 getRootMessageOffset(int verb) {
	if (verb < kVerbFirst || verb > kVerbLast)
		return 0;
	return kRootMessageOffsets[verb - kVerbFirst];
}

Common::String readPascalString(Common::File &file, uint32 offset) {
	if (!offset)
		return Common::String();
	file.seek(offset);
	byte len = file.readByte();
	char buf[256];
	file.read(buf, len);
	return Common::String(buf, len);
}

Common::String capitalize(const Common::String &s) {
	if (s.empty())
		return s;
	Common::String out = s;
	byte c = (byte)s[0];
	if (c >= 'a' && c <= 'z')
		c -= 0x20;
	else if (c == 0x91)  // æ
		c = 0x92;
	else if (c == 0x9b)  // ø
		c = 0x9d;
	else if (c == 0x86)  // å
		c = 0x8f;
	out.setChar((char)c, 0);
	return out;
}

} // End of namespace Miljostrup
