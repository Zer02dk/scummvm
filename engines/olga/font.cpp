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

#include "olga/exe.h"
#include "olga/font.h"

namespace Olga {

GameFont::GameFont() {
	memcpy(_glyphs, Graphics::DosFont::fontData_PCBIOS, sizeof(_glyphs));
}

bool GameFont::loadPatch(const ExeFile &exe) {
	// DanishFont_007b (11c2:007b): the glyphs at DS:0062 + 8k replace the
	// characters Æ (0x92), Ø (0x9d), æ (0x91), ø (0x9b) and å (0x86)
	static const byte chars[5] = { 0x92, 0x9d, 0x91, 0x9b, 0x86 };
	const byte *p = exe.data(kSegData, 0x62, 5 * 8);
	if (!p)
		return false;
	for (int i = 0; i < 5; i++)
		memcpy(_glyphs + chars[i] * 8, p + i * 8, 8);
	return true;
}

void GameFont::drawString(Graphics::ManagedSurface &dst, const Common::String &s, int x, int y, byte color) const {
	for (uint i = 0; i < s.size(); i++, x += 8) {
		const byte *g = _glyphs + (byte)s[i] * 8;
		for (int row = 0; row < 8; row++) {
			for (int col = 0; col < 8; col++) {
				int px = x + col, py = y + row;
				if ((g[row] & (0x80 >> col)) && px >= 0 && py >= 0 && px < dst.w && py < dst.h)
					*(byte *)dst.getBasePtr(px, py) = color;
			}
		}
	}
	dst.addDirtyRect(Common::Rect(MAX(0, x - 8 * (int)s.size()), MAX(0, y), MIN<int>(dst.w, x), MIN<int>(dst.h, y + 8)));
}

void GameFont::drawCentred(Graphics::ManagedSurface &dst, const Common::String &s, int y, byte color) const {
	drawString(dst, s, (320 - 8 * (int)s.size()) / 2, y, color);
}

} // End of namespace Olga
