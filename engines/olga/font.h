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

#ifndef OLGA_FONT_H
#define OLGA_FONT_H

#include "common/str.h"
#include "graphics/managed_surface.h"

namespace Olga {

class ExeFile;

/**
 * The game's text font: the 8x8 PC BIOS font, with five Danish glyphs
 * replaced by bitmaps from OLGA.EXE (unit DanishFont 11c2). Characters are
 * code page 865 bytes, 8 pixels apart; only the set bits are drawn.
 */
class GameFont {
public:
	GameFont();

	bool loadPatch(const ExeFile &exe);
	void drawString(Graphics::ManagedSurface &dst, const Common::String &s, int x, int y, byte color) const;
	/** Centred on the 320-pixel screen as the game does: (320 - 8 * len) / 2. */
	void drawCentred(Graphics::ManagedSurface &dst, const Common::String &s, int y, byte color) const;

private:
	byte _glyphs[256 * 8];
};

} // End of namespace Olga

#endif // OLGA_FONT_H
