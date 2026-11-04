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

#include "common/debug.h"
#include "common/file.h"
#include "common/textconsole.h"

#include "olga/detection.h"
#include "olga/scn.h"

namespace Olga {

namespace {

enum {
	kMaxWords = 1023, // 1cf0:03c8: FileSize in words > 0x3ff is an error
	kMaxAnims = 10,   // Scene_AddAnim
	kMaxSteps = 120   // Scene_AddStep: per object, all animations
};

/** The game's word reader (1cf0:0548): next() past the end gives -1. */
class Words {
public:
	bool load(const Common::Path &name) {
		Common::File f;
		if (!f.open(name))
			return false;
		uint n = f.size() / 2;
		if (n > kMaxWords)
			return false;
		for (uint i = 0; i < n; i++)
			_w.push_back(f.readSint16LE());
		return true;
	}
	int next() {
		if (_pos + 1 < (int)_w.size())
			_cur = _w[++_pos];
		else
			_cur = -1;
		return _cur;
	}
	int take() {
		int v = _cur;
		next();
		return v;
	}
	int cur() const { return _cur; }

private:
	Common::Array<int16> _w;
	int _pos = -1;
	int _cur = -1;
};

void readHotspot(Words &r, ScnHotspot &h) {
	h.x1 = r.take();
	h.y1 = r.take();
	h.x2 = r.take();
	h.y2 = r.take();
	h.id = r.take();
}

// Room_ScnAnim (1cf0:06f2): a word that is read and dropped, p1..p3, the
// frames until -1, the steps until -1
void readAnim(Words &r, ScnObject &obj) {
	ScnAnim a;
	r.next();
	a.dx = r.take();
	a.dy = r.take();
	a.bias = r.take();
	do {
		a.frames.push_back(r.take());
	} while (r.cur() != -1);
	r.next();
	do {
		a.steps.push_back((byte)r.take());
	} while (r.cur() != -1);
	r.next();
	obj.anims.push_back(a);
}

bool checkObject(const ScnObject &obj) {
	uint steps = 0;
	for (uint i = 0; i < obj.anims.size(); i++)
		steps += obj.anims[i].steps.size();
	return obj.anims.size() <= kMaxAnims && steps <= kMaxSteps;
}

} // End of anonymous namespace

bool ScnFile::load(const Common::Path &name) {
	Words r;
	if (!r.load(name)) {
		warning("Olga: cannot read %s", name.toString().c_str());
		return false;
	}
	r.next();
	// 1. static sprites: x, y, depth bias, image
	while (r.cur() != -1) {
		ScnStatic s;
		s.x = r.take();
		s.y = r.take();
		s.bias = r.take();
		s.image = r.take();
		statics.push_back(s);
	}
	r.next();
	// 2. objects: header, hotspot, animations until -1
	while (r.cur() != -1) {
		ScnObject o;
		o.x = r.take();
		o.y = r.take();
		o.delay = r.take();
		o.start = r.take();
		o.flag = r.take();
		readHotspot(r, o.hotspot);
		do {
			readAnim(r, o);
		} while (r.cur() != -1);
		r.next();
		if (!checkObject(o))
			return false;
		objects.push_back(o);
	}
	r.next();
	// 3. hotspots
	while (r.cur() != -1) {
		ScnHotspot h;
		readHotspot(r, h);
		hotspots.push_back(h);
	}
	r.next();
	// 4. walk graph: points, then edges
	while (r.cur() != -1) {
		int x = r.take();
		points.push_back(Common::Point(x, r.take()));
	}
	r.next();
	while (r.cur() != -1) {
		int a = r.take();
		edges.push_back(Common::Point(a, r.take()));
	}
	debugC(1, kDebugResources, "%s: %d statics, %d objects, %d hotspots, %d points",
		   name.toString().c_str(), statics.size(), objects.size(), hotspots.size(), points.size());
	return true;
}

bool loadPrs(const Common::Path &name, ScnObject &obj) {
	Words r;
	if (!r.load(name)) {
		warning("Olga: cannot read %s", name.toString().c_str());
		return false;
	}
	// Room_PrsHeader (1cf0:104c): delay, start, speed x, speed y; then one
	// unused word (1cf0:0ddb starts with a next)
	r.next();
	obj = ScnObject();
	obj.delay = r.take();
	obj.start = r.take();
	obj.speedX = r.take();
	obj.speedY = r.take();
	r.next();
	readHotspot(r, obj.hotspot);
	while (r.cur() != -1)
		readAnim(r, obj);
	return checkObject(obj);
}

} // End of namespace Olga
