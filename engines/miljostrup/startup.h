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

#ifndef MILJOSTRUP_STARTUP_H
#define MILJOSTRUP_STARTUP_H

#include "common/str.h"
#include "graphics/managed_surface.h"

#include "miljostrup/font.h"

namespace Miljostrup {

struct MapMarker {
	int16 x, y;
	uint16 label; // image with the location's name
	uint16 blank; // image shown between blinks
};

/**
 * The text screen of the login dialogue: 39 columns x 19 lines of 8x10
 * cells starting at (4, 5). Each character first fills its cell with the
 * background colour. Lines wrap around to the top.
 */
class TextConsole {
public:
	TextConsole(Graphics::ManagedSurface *screen, const GameFont *font);

	void setForeground(byte color) { _fg = color; }
	void setBackground(byte color) { _bg = color; }

	void clear();
	void putChar(byte c);
	void write(const Common::String &s);
	void writeLine(const Common::String &s);
	void newLine();
	/** Move back one cell, blank it and stay there. */
	void erase();

private:
	void back();

	Graphics::ManagedSurface *_screen;
	const GameFont *_font;
	int _col = 0;
	int _line = 0;
	byte _fg = 8;
	byte _bg = 15;
};

} // End of namespace Miljostrup

#endif // MILJOSTRUP_STARTUP_H
