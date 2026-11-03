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

#include "common/endian.h"

#include "miljostrup/users.h"

namespace Miljostrup {

static const byte kNameKey = 0xEB;
static const uint kSlotSize = 1 + kMaxNameLength;

UserBase::UserBase() {
	memset(_data, 0, sizeof(_data));
}

bool UserBase::load(Common::SeekableReadStream &stream) {
	if (stream.size() != kUserBaseSize)
		return false;
	return stream.read(_data, kUserBaseSize) == kUserBaseSize;
}

bool UserBase::save(Common::WriteStream &stream) const {
	return stream.write(_data, kUserBaseSize) == kUserBaseSize && !stream.err();
}

uint UserBase::count() const {
	return MIN<uint>(READ_LE_UINT16(_data), kMaxUsers);
}

Common::String UserBase::getName(uint number) const {
	const byte *slot = _data + 2 + number * kSlotSize;
	Common::String name;
	for (uint i = 0; i < MIN<uint>(slot[0], kMaxNameLength); ++i)
		name += (char)(slot[1 + i] ^ kNameKey);
	return name;
}

int UserBase::find(const Common::String &name) const {
	Common::String key(name.c_str(), MIN<uint>(name.size(), kMaxNameLength));
	for (uint i = 0; i < count(); ++i) {
		if (getName(i) == key)
			return i;
	}
	return -1;
}

int UserBase::add(const Common::String &name) {
	uint n = count();
	if (n >= kMaxUsers)
		return -1;

	byte *slot = _data + 2 + n * kSlotSize;
	uint len = MIN<uint>(name.size(), kMaxNameLength);
	slot[0] = len;
	for (uint i = 0; i < len; ++i)
		slot[1 + i] = (byte)name[i] ^ kNameKey;
	WRITE_LE_UINT16(_data, n + 1);
	return n;
}

bool readLicence(Common::SeekableReadStream &stream, Common::String &owner) {
	byte r[128];
	if (stream.read(r, sizeof(r)) != sizeof(r))
		return false;

	// Owner String[32] at 0x5f; character i is XORed with 0x56 and a key byte
	uint len = MIN<uint>(r[0x5f], 32);
	for (uint i = 1; i <= len; ++i)
		r[0x5f + i] ^= r[0x5c - i - len] ^ 0x56;

	uint16 sum = 0;
	for (uint i = 0; i < 0x5d; ++i)
		sum += r[i];
	for (uint i = 0x60; i < 0x80; ++i)
		sum += r[i];
	if (sum != (READ_LE_UINT16(r + 0x5d) ^ 0xCCCC) || len == 0)
		return false;

	owner = Common::String((const char *)r + 0x60, len);
	return true;
}

} // End of namespace Miljostrup
