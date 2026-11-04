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

#ifndef OLGA_SCN_H
#define OLGA_SCN_H

#include "common/array.h"
#include "common/path.h"
#include "common/rect.h"

namespace Olga {

/** An animation of an object (notes/scn.md, notes/animation.md). */
struct ScnAnim {
	int dx = 0, dy = 0;          // p1, p2: offset from the object's position
	int bias = 0;                // p3: depth bias while it is shown
	Common::Array<int> frames;   // image numbers
	Common::Array<byte> steps;   // 1-based indices into frames, looping
};

struct ScnHotspot {
	int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	int id = 0;
	/** 1cf0:08ce: id 100 or an empty rectangle switches the control off. */
	bool enabled() const { return !(id == 100 || (x1 == x2 && y1 == y2)); }
};

struct ScnObject {
	int x = 0, y = 0;
	int delay = 0;               // frames between steps
	int start = 0;               // first "last step" frame
	int flag = 0;
	int speedX = 0, speedY = 0;  // PRS only: pixels per walk step
	ScnHotspot hotspot;
	Common::Array<ScnAnim> anims;
};

struct ScnStatic {
	int x = 0, y = 0;
	int bias = 0;
	int image = 0;
};

/**
 * A room file (.SCN): five lists of 16-bit words ended by -1, read by
 * Room_ReadScn (1cf0:1199). A .PRS file is one object (Room_ReadPrs).
 */
struct ScnFile {
	Common::Array<ScnStatic> statics;
	Common::Array<ScnObject> objects;
	Common::Array<ScnHotspot> hotspots;
	Common::Array<Common::Point> points; // walk graph
	Common::Array<Common::Point> edges;  // 1-based point numbers

	bool load(const Common::Path &name);
};

/** Olga's object from a .PRS file; frames count from her sprite file. */
bool loadPrs(const Common::Path &name, ScnObject &obj);

} // End of namespace Olga

#endif // OLGA_SCN_H
