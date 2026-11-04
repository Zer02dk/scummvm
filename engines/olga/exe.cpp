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
#include "common/file.h"
#include "common/textconsole.h"

#include "olga/exe.h"

namespace Olga {

static const uint16 kGhidraBase = 0x1000;

bool ExeFile::open() {
	Common::File f;
	if (!f.open("OLGA.EXE"))
		return false;
	_data.resize(f.size());
	if (f.read(_data.data(), _data.size()) != _data.size() || _data.size() < 0x20)
		return false;
	_header = READ_LE_UINT16(&_data[8]) * 16; // MZ header size in paragraphs
	return true;
}

uint32 ExeFile::offset(uint16 seg, uint16 off) const {
	return _header + (uint32)(seg - kGhidraBase) * 16 + off;
}

const byte *ExeFile::data(uint16 seg, uint16 off, uint32 size) const {
	uint32 o = offset(seg, off);
	if (seg < kGhidraBase || o + size > _data.size())
		return nullptr;
	return &_data[o];
}

Common::String ExeFile::readString(uint16 seg, uint16 off) const {
	const byte *p = data(seg, off, 1);
	if (!p || !data(seg, off, 1 + p[0]))
		return Common::String();
	return Common::String((const char *)p + 1, p[0]);
}

bool ExeFile::readBitmap(uint16 seg, uint16 off, LbmImage &img) const {
	const byte *p = data(seg, off, 6);
	if (!p)
		return false;
	uint16 w = READ_LE_UINT16(p), h = READ_LE_UINT16(p + 2);
	const byte *pix = data(seg, off + 6, (uint32)w * h);
	if (!pix || w == 0 || h == 0)
		return false;
	img.surface.create(w, h, Graphics::PixelFormat::createFormatCLUT8());
	for (uint y = 0; y < h; y++)
		memcpy(img.surface.getBasePtr(0, y), pix + y * w, w);
	img.keyed = (p[4] & 1) != 0;
	img.key = p[5];
	return true;
}

} // End of namespace Olga
