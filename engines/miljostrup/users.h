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

#ifndef MILJOSTRUP_USERS_H
#define MILJOSTRUP_USERS_H

#include "common/str.h"
#include "common/stream.h"

namespace Miljostrup {

enum {
	kMaxUsers = 1000,
	kMaxNameLength = 30,
	kUserBaseSize = 2 + kMaxUsers * (1 + kMaxNameLength) // 31002
};

/**
 * The user base, MLJSTRUP.BAS: a u16 count and 1000 slots of String[30]
 * whose characters are XORed with 0xEB. Kept as one raw block, as the
 * original does, so that unused slots are written back unchanged.
 * Names are code page 865 byte strings.
 */
class UserBase {
public:
	UserBase();

	bool load(Common::SeekableReadStream &stream);
	bool save(Common::WriteStream &stream) const;

	uint count() const;
	Common::String getName(uint number) const;
	/** Exact, case-sensitive lookup of the name cut to 30 characters; -1 if absent. */
	int find(const Common::String &name) const;
	/** Append a user; returns its number, or -1 if the base is full. */
	int add(const Common::String &name);

private:
	byte _data[kUserBaseSize];
};

/** Read MLJSTRUP.LIC. Returns false if the record is invalid or the owner is empty. */
bool readLicence(Common::SeekableReadStream &stream, Common::String &owner);

} // End of namespace Miljostrup

#endif // MILJOSTRUP_USERS_H
