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

#ifndef OLGA_EXE_H
#define OLGA_EXE_H

#include "common/array.h"
#include "common/str.h"

#include "olga/lbm.h"

namespace Olga {

enum {
	kSegData = 0x2bad // the data segment (Ghidra numbering)
};

/**
 * OLGA.EXE, for the texts and images built into the code. Addresses are
 * segment:offset in the Ghidra numbering of the research notes (the EXE
 * image loaded at segment 0x1000).
 */
class ExeFile {
public:
	bool open();

	/** A Pascal string (code page 865 bytes). */
	Common::String readString(uint16 seg, uint16 off) const;
	/** Raw bytes at an address. */
	const byte *data(uint16 seg, uint16 off, uint32 size) const;
	/**
	 * A Bitmap block (unit Bitmap 276f): u16 width, u16 height, u8 flags,
	 * u8 key colour, then the pixels. Flag bit 0: the key is transparent.
	 */
	bool readBitmap(uint16 seg, uint16 off, LbmImage &img) const;

private:
	uint32 offset(uint16 seg, uint16 off) const;

	Common::Array<byte> _data;
	uint32 _header = 0;
};

} // End of namespace Olga

#endif // OLGA_EXE_H
