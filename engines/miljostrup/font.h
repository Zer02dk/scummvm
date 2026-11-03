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

#ifndef MILJOSTRUP_FONT_H
#define MILJOSTRUP_FONT_H

#include "common/file.h"
#include "common/str.h"
#include "graphics/managed_surface.h"

namespace Miljostrup {

/**
 * The game's text font: the 8x8 PC BIOS font with five Danish glyphs
 * replaced by bitmaps from MLJSTRUP.EXE. Characters are code page 865 bytes,
 * drawn 8 pixels apart; only set bits are drawn.
 */
class GameFont {
public:
	GameFont();

	bool loadPatch(Common::File &exe);
	void drawChar(Graphics::ManagedSurface *dst, byte c, int x, int y, byte color) const;
	void drawString(Graphics::ManagedSurface *dst, const Common::String &s, int x, int y, byte color) const;

private:
	byte _glyphs[256 * 8];
};

/** Map a typed Unicode character to code page 865, or 0 if it has none. */
byte unicodeToCp865(uint16 c);

} // End of namespace Miljostrup

#endif // MILJOSTRUP_FONT_H
