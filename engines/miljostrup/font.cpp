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

#include "graphics/fonts/dosfont.h"

#include "miljostrup/font.h"
#include "miljostrup/startup.h"
#include "miljostrup/startup_data.h"

namespace Miljostrup {

GameFont::GameFont() {
	memcpy(_glyphs, Graphics::DosFont::fontData_PCBIOS, sizeof(_glyphs));
}

bool GameFont::loadPatch(Common::File &exe) {
	exe.seek(kGlyphOffset);
	for (uint i = 0; i < ARRAYSIZE(kGlyphChars); ++i)
		exe.read(&_glyphs[kGlyphChars[i] * 8], 8);
	return !exe.err();
}

void GameFont::drawChar(Graphics::ManagedSurface *dst, byte c, int x, int y, byte color) const {
	const byte *glyph = &_glyphs[c * 8];
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			if (glyph[row] & (0x80 >> col)) {
				int px = x + col, py = y + row;
				if (px >= 0 && py >= 0 && px < dst->w && py < dst->h)
					*(byte *)dst->getBasePtr(px, py) = color;
			}
		}
	}
	dst->addDirtyRect(Common::Rect(x, y, x + 8, y + 8));
}

void GameFont::drawString(Graphics::ManagedSurface *dst, const Common::String &s, int x, int y, byte color) const {
	for (uint i = 0; i < s.size(); ++i)
		drawChar(dst, (byte)s[i], x + i * 8, y, color);
}

// Code page 865, 0x80..0xa5
static const uint16 kCp865Upper[] = {
	0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
	0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
	0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
	0x00ff, 0x00d6, 0x00dc, 0x00f8, 0x00a3, 0x00d8, 0x20a7, 0x0192,
	0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1
};

byte unicodeToCp865(uint16 c) {
	if (c >= 0x20 && c < 0x7f)
		return c;
	for (uint i = 0; i < ARRAYSIZE(kCp865Upper); ++i) {
		if (kCp865Upper[i] == c)
			return 0x80 + i;
	}
	return 0;
}

} // End of namespace Miljostrup
