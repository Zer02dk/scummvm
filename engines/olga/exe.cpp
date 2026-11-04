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
static const uint16 kFirstOverlay = 0x5000;

// Where each overlay unit's code starts in OLGA.OVR, by its Ghidra segment
// (the FBOV unit headers, tools/ovr.py list)
static const struct {
	uint16 seg;
	uint32 ovrOffset;
	uint32 size;
} kOverlays[] = {
	{ 0x5000, 0x000008, 0x02ec }, { 0x502f, 0x000326, 0x0773 }, { 0x50a7, 0x000b15, 0x034a },
	{ 0x50dc, 0x000e99, 0x1b2c }, { 0x528f, 0x002b25, 0x05e5 }, { 0x52ee, 0x00316a, 0x0de6 },
	{ 0x53cd, 0x00400e, 0x059b }, { 0x5427, 0x004601, 0x2168 }, { 0x563e, 0x0068b1, 0x00e1 }
};

static bool readAll(const char *name, Common::Array<byte> &buf) {
	Common::File f;
	if (!f.open(name))
		return false;
	buf.resize(f.size());
	return f.read(buf.data(), buf.size()) == buf.size();
}

bool ExeFile::open() {
	if (!readAll("OLGA.EXE", _data) || _data.size() < 0x20 || !readAll("OLGA.OVR", _ovr))
		return false;
	_header = READ_LE_UINT16(&_data[8]) * 16; // MZ header size in paragraphs
	return true;
}

const byte *ExeFile::at(uint16 seg, uint16 off, uint32 size) const {
	if (seg >= kFirstOverlay) {
		for (uint i = 0; i < ARRAYSIZE(kOverlays); i++) {
			if (kOverlays[i].seg != seg)
				continue;
			uint32 o = kOverlays[i].ovrOffset + off;
			if (off + size > kOverlays[i].size || o + size > _ovr.size())
				return nullptr;
			return &_ovr[o];
		}
		return nullptr;
	}
	uint32 o = _header + (uint32)(seg - kGhidraBase) * 16 + off;
	if (seg < kGhidraBase || o + size > _data.size())
		return nullptr;
	return &_data[o];
}

const byte *ExeFile::data(uint16 seg, uint16 off, uint32 size) const {
	return at(seg, off, size);
}

Common::String ExeFile::readString(uint16 seg, uint16 off) const {
	const byte *p = at(seg, off, 1);
	if (!p || !at(seg, off, 1 + p[0]))
		return Common::String();
	return Common::String((const char *)p + 1, p[0]);
}

bool ExeFile::readBitmap(uint16 seg, uint16 off, LbmImage &img) const {
	const byte *p = at(seg, off, 6);
	if (!p)
		return false;
	uint16 w = READ_LE_UINT16(p), h = READ_LE_UINT16(p + 2);
	const byte *pix = at(seg, off + 6, (uint32)w * h);
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
