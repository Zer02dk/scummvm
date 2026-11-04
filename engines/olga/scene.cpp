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

#include "olga/detection.h"
#include "olga/scene.h"

namespace Olga {

int SceneObject::currentImage() const {
	if (!animated)
		return image;
	if (anim < 1 || anim > (int)def.anims.size())
		return 0;
	const ScnAnim &a = def.anims[anim - 1];
	if (a.steps.empty() || stepPos >= a.steps.size())
		return 0;
	int s = a.steps[stepPos];
	if (s < 1 || s > (int)a.frames.size())
		return 0;
	return a.frames[s - 1] + frameBase;
}

const LbmImage *Scene::image(int n) const {
	if (n >= 1 && n <= (int)_room.size())
		return &_room.image(n);
	n -= _room.size();
	if (n >= 1 && n <= (int)_sprites.size())
		return &_sprites.image(n);
	return nullptr;
}

bool Scene::load(const LocationDef &loc, int entry) {
	_objects.clear();
	_olga = -1;
	_counter = 0;
	_scn = ScnFile();
	if (!_room.load(loc.lbm) || !_scn.load(loc.scn))
		return false;
	bool hasOlga = loc.spriteLbm[0] != 0;
	if (hasOlga && !_sprites.load(loc.spriteLbm))
		return false;

	// Section 1: Scene_AddStatic only keeps sprites that start inside the room
	for (uint i = 0; i < _scn.statics.size(); i++) {
		const ScnStatic &s = _scn.statics[i];
		if (s.x >= kRoomWidth || s.y >= kRoomHeight)
			continue;
		SceneObject o;
		o.x = s.x;
		o.y = s.y;
		o.bias = s.bias;
		o.image = s.image;
		_objects.push_back(o);
	}
	// Section 2: the animated objects. Their handlers pick the animation
	// (notes/situations.md); until those exist every object shows its first.
	for (uint i = 0; i < _scn.objects.size(); i++) {
		SceneObject o;
		o.animated = true;
		o.def = _scn.objects[i];
		o.x = o.def.x;
		o.y = o.def.y;
		o.delay = o.def.delay;
		o.last = o.def.start;
		switchAnim(o, 1);
		_objects.push_back(o);
	}
	// Room_ReadPrs: Olga, at the entry's base position, animation 1 (standing);
	// her frame numbers count from the first image of her sprite file
	if (hasOlga) {
		SceneObject o;
		o.animated = true;
		if (!loadPrs(loc.prs, o.def))
			return false;
		int e = CLIP(entry, 1, loc.numEntries) - 1;
		o.x = loc.entries[e].x;
		o.y = loc.entries[e].y;
		o.delay = o.def.delay;
		o.last = o.def.start;
		o.frameBase = _room.size();
		switchAnim(o, 1);
		_olga = _objects.size();
		_objects.push_back(o);
	}
	debugC(1, kDebugGraphics, "%s: %d sprites, Olga %s", loc.lbm, _objects.size(),
		   hasOlga ? "yes" : "no");
	return true;
}

void Scene::switchAnim(SceneObject &obj, int n) {
	// Scene_SwitchAnim + Scene_ApplyAnimOffset: leave the old animation's
	// offset, take the new one's offset and depth bias, start at its first step
	if (n < 1 || n > (int)obj.def.anims.size())
		return;
	if (obj.anim >= 1) {
		const ScnAnim &old = obj.def.anims[obj.anim - 1];
		obj.x -= old.dx;
		obj.y -= old.dy;
	}
	const ScnAnim &a = obj.def.anims[n - 1];
	obj.anim = n;
	obj.stepPos = 0;
	obj.x += a.dx;
	obj.y += a.dy;
	obj.bias = a.bias;
}

void Scene::step(SceneObject &obj) {
	// Scene_Step (1fab:0d2f): a requested, different animation is switched
	// to; otherwise the step position advances and wraps (Scene_AdvanceStep)
	if (obj.pending) {
		int n = obj.pending;
		obj.pending = 0;
		if (n != obj.anim) {
			switchAnim(obj, n);
			return;
		}
	}
	if (obj.anim >= 1) {
		const ScnAnim &a = obj.def.anims[obj.anim - 1];
		if (!a.steps.empty())
			obj.stepPos = (obj.stepPos + 1) % a.steps.size();
	}
}

void Scene::tick() {
	// Scene_Tick: the frame counter runs mod 1000; an object steps when at
	// least `delay` frames have passed since its last step (Scene_DelayDone)
	_counter = (_counter + 1) % 1000;
	for (int i = (int)_objects.size() - 1; i >= 0; i--) {
		SceneObject &o = _objects[i];
		if (!o.animated)
			continue;
		if ((_counter - o.last + 1000) % 1000 >= o.delay) {
			o.last = _counter;
			step(o);
		}
	}
}

int Scene::sortKey(const SceneObject &obj) const {
	// Scene_SortKey (1fab:089b): the bottom line, moved by the depth bias
	const LbmImage *img = image(obj.currentImage());
	int h = img ? img->surface.h : 0;
	return obj.y + h + obj.bias - 1;
}

void Scene::draw(Graphics::ManagedSurface &dst) const {
	dst.blitFrom(_room.image(1).surface, Common::Point(0, 0));

	// Sorted by depth (Scene_SortList); equal keys keep the list order, which
	// may differ from the original's selection sort (HYPOTHESIS: rare)
	Common::Array<const SceneObject *> order;
	Common::Array<int> keys;
	for (uint i = 0; i < _objects.size(); i++) {
		int k = sortKey(_objects[i]);
		uint j = order.size();
		while (j > 0 && keys[j - 1] > k)
			j--;
		order.insert_at(j, &_objects[i]);
		keys.insert_at(j, k);
	}
	for (uint i = 0; i < order.size(); i++) {
		const LbmImage *img = image(order[i]->currentImage());
		if (!img)
			continue;
		Common::Point p(order[i]->x, order[i]->y);
		if (img->keyed)
			dst.transBlitFrom(img->surface, p, img->key);
		else
			dst.blitFrom(img->surface, p);
	}
}

} // End of namespace Olga
