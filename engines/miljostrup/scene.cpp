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

// Walking in a room (notes/walking.md): walk graph, player figure, other
// animated figures, clicks on regions and exits.

#include "common/debug.h"
#include "common/queue.h"

#include "miljostrup/detection.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"

namespace Miljostrup {

enum {
	kNoDistance = 10000 // "not on this edge" in 21e8:05b2
};

// 24d2:0d18: a / n rounded, remainders above n / 2 round up (Pascal div/mod)
static int roundDiv(int a, int n) {
	int q = a / n;
	if (a % n > n / 2)
		++q;
	return q;
}

// Pascal Round: halves away from zero
static int roundReal(double v) {
	return v < 0 ? -(int)(-v + 0.5) : (int)(v + 0.5);
}

Scene::Scene(ImageArchive *archive, Common::HashMap<uint16, bool> *doors) :
	_archive(archive), _doors(doors) {
}

Scene::~Scene() {
	freeImages();
}

void Scene::freeImages() {
	for (auto &i : _images)
		delete i._value;
	_images.clear();
}

Graphics::ManagedSurface *Scene::getImage(uint16 number) {
	if (_images.contains(number))
		return _images[number];
	Graphics::ManagedSurface *image = _archive->loadImage(number);
	_images[number] = image;
	return image;
}

bool Scene::load(const RoomInfo *room, int entryNode) {
	if (!room || !room->image || !room->nodeCount)
		return false;

	freeImages();
	Graphics::ManagedSurface *background = getImage(room->image);
	if (!background)
		return false;

	_room = room;
	_roomSurface.create(background->w, background->h, background->format);
	_figures.clear();
	_drawOrder.clear();
	_player = -1;
	_targetObject = 0;
	_arrived = false;

	for (int i = 0; i < room->figureCount; ++i) {
		Figure fig;
		fig.info = &getFigure(room, i);
		fig.x = fig.info->x;
		fig.y = fig.info->y;
		fig.countdown = MAX<int>(fig.info->delay, 1);
		_figures.push_back(fig);
		_drawOrder.push_back(i);
		if (fig.info->player)
			_player = i;
	}
	if (_player < 0)
		return false;

	// The player stands at the entry node (b000:0034); the bound objects set
	// the other figures' states when they are bound (22a9:03af -> VMT 0x58)
	Figure &player = _figures[_player];
	if (entryNode >= 1 && entryNode <= room->nodeCount) {
		const WalkNode &node = getNode(room, entryNode);
		player.x = node.x - room->footX;
		player.y = node.y - room->footY;
	}
	for (uint i = 0; i < _figures.size(); ++i) {
		Figure &fig = _figures[i];
		setState(fig, initialState(fig));
		animate(fig);
	}
	return true;
}

void Scene::setState(Figure &fig, int state) {
	fig.pendingState = state;
}

uint16 Scene::currentImage(const Figure &fig) const {
	if (!fig.frame)
		return 0;
	uint len = getSequenceLength(_room, fig.frame->sequence);
	if (!len)
		return 0;
	return getSequenceImage(_room, fig.frame->sequence, fig.imageIndex % len);
}

bool Scene::switchState(Figure &fig) {
	// 24d2:090c, the first part: a newly set state takes its frame
	if (fig.pendingState == kStateNone || fig.pendingState == fig.state)
		return false;
	const FigureFrame *frame = nullptr;
	for (int i = 0; i < fig.info->frameCount; ++i) {
		const FigureFrame &f = getFigureFrame(*fig.info, i);
		if (f.state == fig.pendingState)
			frame = &f;
	}
	if (frame) {
		if (fig.frame) {
			fig.x -= fig.frame->dx;
			fig.y -= fig.frame->dy;
		}
		fig.frame = frame;
		fig.x += frame->dx;
		fig.y += frame->dy;
		fig.state = fig.pendingState;
		fig.imageIndex = 0;
	} else {
		// The room has no images for this state; keep the current ones
		debugC(2, kDebugGraphics, "Figure has no state %d", fig.pendingState);
	}
	fig.pendingState = kStateNone;
	return true;
}

void Scene::showFigure(Figure &fig, bool visible) {
	// A hidden figure that is shown again takes its new state at once, so
	// its old image is not seen for a frame (the wagon's load in the farm's
	// room 9, the falling bag)
	if (visible && !fig.visible)
		switchState(fig);
	fig.visible = visible;
}

void Scene::animate(Figure &fig) {
	// 24d2:090c: switch to a newly set state, or show the next image
	if (switchState(fig))
		return;
	if (fig.frame) {
		uint len = getSequenceLength(_room, fig.frame->sequence);
		if (len)
			fig.imageIndex = (fig.imageIndex + 1) % len;
	}
	fig.pendingState = kStateNone;
}

void Scene::startCommand(Figure &fig) {
	// 24d2:0e9f
	fig.command = fig.queue.front();
	fig.queue.remove_at(0);
	setState(fig, fig.command.state);
	fig.remaining = fig.command.steps;
	fig.movedX = fig.movedY = 0;
	fig.moving = true;
	step(fig);
}

void Scene::step(Figure &fig) {
	// 24d2:0d55: position i of n on the straight line
	--fig.remaining;
	int n = fig.command.steps;
	int i = n - fig.remaining;
	int nx = roundDiv(fig.command.dx * i, n);
	int ny = roundReal((double)fig.command.dy * i / n);
	fig.x += nx - fig.movedX;
	fig.y += ny - fig.movedY;
	fig.movedX = nx;
	fig.movedY = ny;
}

void Scene::advance(Figure &fig) {
	// 24d2:0af2: movement (24d2:0f53), then the animation
	if (!fig.moving) {
		if (!fig.queue.empty())
			startCommand(fig);
	} else if (fig.remaining == 0) {
		// 24d2:0f53: the walk-end call may queue new walks; they start at once
		fig.moving = false;
		if (fig.queue.empty() && !fig.route.empty()) {
			fig.x = fig.routeX + (fig.frame ? fig.frame->dx : 0);
			fig.y = fig.routeY + (fig.frame ? fig.frame->dy : 0);
			fig.queue = fig.route;
		}
		if (fig.queue.empty())
			fig.arrived = true;
		else
			startCommand(fig);
	} else {
		step(fig);
	}
	animate(fig);
}

void Scene::update() {
	if (!_room)
		return;

	// 24d2:08ac: one animation step every `ticks` frames
	for (uint i = 0; i < _figures.size(); ++i) {
		Figure &fig = _figures[i];
		if (--fig.countdown <= 0) {
			fig.countdown = MAX<int>(fig.info->ticks, 1);
			advance(fig);
		}
	}

	Figure &player = _figures[_player];
	if (player.arrived) {
		player.arrived = false;
		arrive();
	}
}

void Scene::draw(Graphics::ManagedSurface *dst) {
	if (!_room)
		return;

	// The background is drawn whole: ManagedSurface::blitFrom would skip its
	// key colour (every ARK image has one). Of the 25 backgrounds only 702
	// (the school's corridor) uses its key (18), on the containers' faces,
	// and the original shows them (user, 2026-11-01)
	const Graphics::ManagedSurface *background = getImage(_room->image);
	_roomSurface.copyRectToSurface(background->rawSurface(), 0, 0, Common::Rect(background->w, background->h));

	// 23c9:013c: stable insertion sort by bottom edge + z, then draw in order
	Common::Array<int> keys;
	keys.resize(_figures.size());
	for (uint i = 0; i < _figures.size(); ++i) {
		const Figure &fig = _figures[i];
		Graphics::ManagedSurface *image = currentImage(fig) ? getImage(currentImage(fig)) : nullptr;
		keys[i] = fig.y + (image ? image->h : 0) - 1 + (fig.frame ? fig.frame->z : 0);
	}
	for (uint i = 1; i < _drawOrder.size(); ++i) {
		int item = _drawOrder[i];
		int j = i;
		while (j > 0 && keys[item] < keys[_drawOrder[j - 1]]) {
			_drawOrder[j] = _drawOrder[j - 1];
			--j;
		}
		_drawOrder[j] = item;
	}

	for (uint i = 0; i < _drawOrder.size(); ++i) {
		const Figure &fig = _figures[_drawOrder[i]];
		if (!fig.visible)
			continue;
		uint16 number = currentImage(fig);
		Graphics::ManagedSurface *image = number ? getImage(number) : nullptr;
		if (image)
			_roomSurface.transBlitFrom(*image, Common::Point(fig.x, fig.y), image->getTransparentColor());
	}
	dst->blitFrom(_roomSurface, Common::Point(_room->x, _room->y));
}

Common::Point Scene::getFeet() const {
	const Figure &player = _figures[_player];
	return Common::Point(player.x + _room->footX, player.y + _room->footY);
}

int Scene::nearestNode(const Common::Point &p) const {
	// 21e8:02bd: the first node with the smallest distance
	int best = 1;
	int bestDist = -1;
	for (int i = 1; i <= _room->nodeCount; ++i) {
		const WalkNode &n = getNode(_room, i);
		int dist = (n.x - p.x) * (n.x - p.x) + (n.y - p.y) * (n.y - p.y);
		if (bestDist < 0 || dist < bestDist) {
			best = i;
			bestDist = dist;
		}
	}
	return best;
}

int Scene::nearestEdge(const Common::Point &p) const {
	// 21e8:0702 / 05b2: the edge nearest to the point among those it lies
	// beside (its foot on the edge falls between the two nodes); 0 if none
	int best = 0;
	int bestDist = kNoDistance;
	for (int i = 1; i <= _room->edgeCount; ++i) {
		const WalkEdge &e = getEdge(_room, i);
		const WalkNode &a = getNode(_room, e.a);
		const WalkNode &b = getNode(_room, e.b);
		int ex = b.x - a.x, ey = b.y - a.y;
		long along1 = (long)(p.x - a.x) * ex + (long)(p.y - a.y) * ey;
		long along2 = (long)(p.x - b.x) * -ex + (long)(p.y - b.y) * -ey;
		int dist = kNoDistance;
		if (along1 >= 0 && along2 >= 0 && (ex || ey)) {
			double cross = (double)(p.x - a.x) * ey - (double)(p.y - a.y) * ex;
			dist = roundReal(ABS(cross) / sqrt((double)ex * ex + (double)ey * ey));
		}
		if (dist < bestDist) {
			best = i;
			bestDist = dist;
		}
	}
	return best;
}

bool Scene::findPath(int from, int to, Common::Array<int> &path) const {
	// The walk graphs are trees, so the path is unique (21e8:0806)
	Common::Array<int> parent;
	parent.resize(_room->nodeCount + 1);
	for (uint i = 0; i < parent.size(); ++i)
		parent[i] = -1;
	Common::Queue<int> todo;
	parent[from] = 0;
	todo.push(from);
	while (!todo.empty()) {
		int n = todo.pop();
		for (int i = 1; i <= _room->edgeCount; ++i) {
			const WalkEdge &e = getEdge(_room, i);
			int other = e.a == n ? e.b : (e.b == n ? e.a : 0);
			if (other && parent[other] < 0) {
				parent[other] = n;
				todo.push(other);
			}
		}
	}
	if (parent[to] < 0)
		return false;

	path.clear();
	for (int n = to; n != 0; n = parent[n])
		path.insert_at(0, n);
	return true;
}

void Scene::queueSegment(int dx, int dy) {
	// 21e8:0958: walk sideways if the segment is wider than high, else up or down
	WalkCommand cmd;
	cmd.dx = dx;
	cmd.dy = dy;
	if (ABS(dy) < ABS(dx)) {
		int s = _room->hStep;
		cmd.steps = (ABS(dx) * 2 + s) / (2 * s);
		cmd.state = dx < 1 ? kStateWalkLeft : kStateWalkRight;
	} else {
		int s = _room->vStep;
		cmd.steps = (ABS(dy) * 2 + s) / (2 * s);
		cmd.state = dy < 1 ? kStateWalkUp : kStateWalkDown;
	}
	// The original stops with a run-time error on a walk of 0 steps
	cmd.steps = MAX(cmd.steps, 1);
	_figures[_player].queue.push_back(cmd);
}

void Scene::walkTo(int target) {
	// b000:060a / 04c1, then 21e8:0a13
	Figure &player = _figures[_player];
	player.queue.clear();
	player.moving = false;
	player.arrived = false;

	Common::Point feet = getFeet();
	int edge = nearestEdge(feet);
	int start = edge ? getEdge(_room, edge).a : nearestNode(feet);

	Common::Array<int> path;
	if (!findPath(start, target, path))
		return;
	// Already on the edge towards the second node: skip the first
	if (edge && path.size() > 1 && path[1] == getEdge(_room, edge).b)
		path.remove_at(0);

	if (_fastWalk) {
		// Debugging only: stand on the last node and arrive at once
		const WalkNode &n = getNode(_room, path.back());
		player.x = n.x - _room->footX;
		player.y = n.y - _room->footY;
		arrive();
		return;
	}

	Common::Point from = feet;
	for (uint i = 0; i < path.size(); ++i) {
		const WalkNode &n = getNode(_room, path[i]);
		if (n.x != from.x || n.y != from.y)
			queueSegment(n.x - from.x, n.y - from.y);
		from = Common::Point(n.x, n.y);
	}
	if (player.queue.empty())
		arrive();
}

void Scene::arrive() {
	// b000:0477 (room VMT 0x70): a walk still running (a command for an
	// object without a node, e.g. the dung room's dung or floor, given while
	// walking to another) is stopped (the player's VMT 0x50) before the
	// player stands and the command runs
	Figure &player = _figures[_player];
	player.queue.clear();
	player.moving = false;
	setState(player, kStateStand);
	if (_targetObject) {
		_arrived = true;
	}
}

int Scene::regionAt(const Common::Point &pos) const {
	if (!_room)
		return -1;
	Common::Point p(pos.x - _room->x, pos.y - _room->y);
	// The region added first wins where regions overlap (the user's check in
	// the living room: the TV, added 2nd, keeps the corners that the window
	// and the radiator, added later, overlap). Disabled regions do not react
	// (widget +0xc, 28c3:09ca).
	for (int i = 0; i < _room->regionCount; ++i) {
		const RoomRegion &r = getRegion(_room, i);
		if (!g_engine->isObjectEnabled(r.object))
			continue;
		Common::Rect rect(r.x1, r.y1, r.x2, r.y2);
		g_engine->adjustRegion(r.object, rect);
		if (p.x >= rect.left && p.x <= rect.right && p.y >= rect.top && p.y <= rect.bottom)
			return i;
	}
	return -1;
}

int Scene::objectAt(const Common::Point &pos) const {
	int i = regionAt(pos);
	if (i < 0)
		return 0;
	int object = getRegion(_room, i).object;
	// a disabled region (28c3:014d) does not react
	return g_engine->isObjectEnabled(object) ? object : 0;
}

void Scene::walkToObject(const Common::Point &pos, int object) {
	// ae72:0e21: walk to the object's node, then the verb runs (ae72:0c42).
	// With two objects the click is on the second one, but the walk goes to
	// the first's node (its region, +0x10), e.g. the hose's for "Benyt
	// spuleslange med gulv" in the chicken farm's dung room
	int i = regionAt(pos);
	if (i < 0 || getRegion(_room, i).object != object) {
		for (int k = 0; k < _room->regionCount; ++k) {
			if (getRegion(_room, k).object == object && g_engine->isObjectEnabled(object)) {
				i = k;
				break;
			}
		}
	}
	if (i < 0)
		return;
	const RoomRegion &r = getRegion(_room, i);
	// VMT 0x24 = the object's walk node (root 232c:0214: the region's,
	// +0x10); some classes override it (the factory's trees, 6000:14fa)
	int node = g_engine->getObjectNode(r.object, r.node);
	debugC(1, kDebugGraphics, "Walking to object %d at node %d", object, node);
	_targetObject = object;
	_arrived = false;
	if (node)
		walkTo(node);
	else
		arrive();
}

bool Scene::takeArrival(int &object) {
	if (!_arrived)
		return false;
	_arrived = false;
	object = _targetObject;
	_targetObject = 0;
	return true;
}

void Scene::stopWalking() {
	Figure &player = _figures[_player];
	player.queue.clear();
	player.moving = false;
	_targetObject = 0;
	setState(player, kStateStand);
}

void Scene::queueFigureWalk(uint16 object, int state, int dx, int dy, int steps) {
	for (uint i = 0; i < _figures.size(); ++i) {
		if (_figures[i].info->object == object) {
			WalkCommand cmd = { state, dx, dy, MAX(steps, 1) };
			_figures[i].queue.push_back(cmd);
			return;
		}
	}
}

void Scene::setFigureRoute(uint16 object, int x, int y, const WalkCommand *walks, int count) {
	for (uint i = 0; i < _figures.size(); ++i) {
		Figure &fig = _figures[i];
		if (fig.info->object == object) {
			fig.routeX = x;
			fig.routeY = y;
			fig.route.clear();
			for (int k = 0; k < count; ++k)
				fig.route.push_back(walks[k]);
			return;
		}
	}
}

void Scene::stopFigure(uint16 object) {
	for (uint i = 0; i < _figures.size(); ++i) {
		if (_figures[i].info->object == object) {
			_figures[i].queue.clear();
			_figures[i].moving = false;
		}
	}
}

bool Scene::isFigureBusy(uint16 object) const {
	for (uint i = 0; i < _figures.size(); ++i) {
		if (_figures[i].info->object == object)
			return _figures[i].moving || !_figures[i].queue.empty();
	}
	return false;
}

int Scene::getFigureX(uint16 object) const {
	for (uint i = 0; i < _figures.size(); ++i) {
		if (_figures[i].info->object == object)
			return _figures[i].x;
	}
	return 0;
}

void Scene::setFigurePos(uint16 object, int x, int y) {
	// The figure's x/y here include the frame's offset (animate())
	for (uint i = 0; i < _figures.size(); ++i) {
		Figure &fig = _figures[i];
		if (fig.info->object == object) {
			fig.x = x + (fig.frame ? fig.frame->dx : 0);
			fig.y = y + (fig.frame ? fig.frame->dy : 0);
		}
	}
}

void Scene::setFigureState(uint16 object, int state) {
	for (uint i = 0; i < _figures.size(); ++i) {
		if (_figures[i].info->object == object)
			setState(_figures[i], state);
	}
}

void Scene::debugSetState(uint index, int state) {
	Figure &fig = _figures[index];
	fig.queue.clear();
	fig.route.clear();
	fig.moving = false;
	fig.visible = true;
	setState(fig, state);
}

void Scene::debugWalk(uint index, int state, int dx, int dy, int steps) {
	Figure &fig = _figures[index];
	WalkCommand cmd = { state, dx, dy, MAX(steps, 1) };
	fig.visible = true;
	fig.queue.push_back(cmd);
}

void Scene::setFigureVisible(uint16 object, bool visible) {
	for (uint i = 0; i < _figures.size(); ++i) {
		if (_figures[i].info->object == object)
			showFigure(_figures[i], visible);
	}
}

void Scene::startTalk(int steps) {
	Figure &player = _figures[_player];
	player.queue.clear();
	player.moving = false;
	WalkCommand cmd = { kStateTalk, 0, 0, steps };
	player.queue.push_back(cmd);
}

void Scene::playPlayer(int state, int steps) {
	Figure &player = _figures[_player];
	WalkCommand cmd = { state, 0, 0, MAX(steps, 1) };
	player.queue.push_back(cmd);
}

void Scene::setPlayerState(int state) {
	if (_player >= 0)
		setState(_figures[_player], state);
}

bool Scene::isWalking() const {
	const Figure &player = _figures[_player];
	return player.moving || !player.queue.empty();
}

int Scene::initialState(Figure &fig) {
	// The bound object's class decides (VMT 0x58); else the generated start
	// state; else the root rule, state 1 (232c:0516)
	if (fig.info->player)
		return kStateStand;
	if (fig.info->object) {
		bool visible = true;
		int state = g_engine->getFigureState(fig.info->object, visible);
		fig.visible = visible;
		if (state)
			return state;
	}
	return fig.info->state ? fig.info->state : (int)kStateStand;
}

void Scene::refreshFigures() {
	if (!_room)
		return;
	for (uint i = 0; i < _figures.size(); ++i) {
		Figure &fig = _figures[i];
		if (fig.info->object && !fig.info->player && !fig.moving && fig.queue.empty())
			setState(fig, initialState(fig));
	}
}

void Scene::updateObject(uint16 object) {
	if (!_room)
		return;
	for (uint i = 0; i < _figures.size(); ++i) {
		Figure &fig = _figures[i];
		if (fig.info->object == object) {
			bool wasVisible = fig.visible;
			setState(fig, initialState(fig));
			if (fig.visible && !wasVisible) {
				fig.visible = false;
				showFigure(fig, true);
			}
		}
	}
}

} // End of namespace Miljostrup
