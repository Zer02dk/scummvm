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
#include "olga/walk.h"

namespace Olga {

namespace {

const int kFar = 10000;

// WalkGraph_Distance (1ebf:01ec): Euclidean, rounded
int distance(const Common::Point &a, const Common::Point &b) {
	double dx = a.x - b.x, dy = a.y - b.y;
	return (int)(sqrt(dx * dx + dy * dy) + 0.5);
}

// WalkGraph_EdgeDistance (1ebf:0592): the perpendicular distance if the
// foot of the perpendicular lies on the edge, else 10000
int edgeDistance(const Common::Point &p, const Common::Point &a, const Common::Point &b) {
	long abx = b.x - a.x, aby = b.y - a.y;
	long dot1 = (p.x - a.x) * abx + (p.y - a.y) * aby;
	long dot2 = (p.x - b.x) * -abx + (p.y - b.y) * -aby;
	if (dot1 < 0 || dot2 < 0)
		return kFar;
	double len = sqrt((double)(abx * abx + aby * aby));
	if (len == 0)
		return distance(p, a);
	long cross = (p.x - a.x) * aby - (p.y - a.y) * abx;
	return (int)(ABS(cross) / len + 0.5);
}

// WalkGraph_NearestPoint (1ebf:0296): 1-based, the first of equal ones
int nearestPoint(const ScnFile &scn, const Common::Point &p) {
	int best = 1, bestD = distance(scn.points[0], p);
	for (uint i = 1; i < scn.points.size(); i++) {
		int d = distance(scn.points[i], p);
		if (d < bestD) {
			best = i + 1;
			bestD = d;
		}
	}
	return best;
}

// WalkGraph_NearestEdge (1ebf:06d4): 1-based, 0 if no edge is near
int nearestEdge(const ScnFile &scn, const Common::Point &p) {
	int best = 0, bestD = kFar;
	for (uint i = 0; i < scn.edges.size(); i++) {
		int a = scn.edges[i].x, b = scn.edges[i].y;
		if (a < 1 || b < 1 || a > (int)scn.points.size() || b > (int)scn.points.size())
			continue;
		int d = edgeDistance(p, scn.points[a - 1], scn.points[b - 1]);
		if (d < bestD) {
			best = i + 1;
			bestD = d;
		}
	}
	return best;
}

// WalkGraph_FindPath (1ebf:07ea) is a depth-first search with backtracking;
// all walk graphs are trees (tools/test_scn.py), so any search finds the
// same, only path
bool findPath(const ScnFile &scn, int from, int to, Common::Array<int> &path) {
	int n = scn.points.size();
	Common::Array<int> prev(n + 1, 0);
	Common::Array<int> queue;
	queue.push_back(from);
	prev[from] = from;
	for (uint q = 0; q < queue.size(); q++) {
		int cur = queue[q];
		if (cur == to)
			break;
		for (uint i = 0; i < scn.edges.size(); i++) {
			int a = scn.edges[i].x, b = scn.edges[i].y, next = 0;
			if (a == cur)
				next = b;
			else if (b == cur)
				next = a;
			if (next >= 1 && next <= n && prev[next] == 0) {
				prev[next] = cur;
				queue.push_back(next);
			}
		}
	}
	if (prev[to] == 0)
		return false;
	path.clear();
	for (int v = to; v != from; v = prev[v])
		path.insert_at(0, v);
	path.insert_at(0, from);
	return true;
}

// WalkGraph_AddMove (1ebf:0936): along the larger of dx, dy; animation
// 3 right, 4 left, 5 away, 6 towards the viewer
void addMove(int dx, int dy, int speedX, int speedY, Common::Array<SceneObject::Move> &moves) {
	SceneObject::Move m;
	m.dx = dx;
	m.dy = dy;
	if (ABS(dy) < ABS(dx)) {
		m.steps = ABS(dx) / MAX(1, speedX) + 1;
		m.anim = dx < 1 ? 4 : 3;
	} else {
		m.steps = ABS(dy) / MAX(1, speedY) + 1;
		m.anim = dy < 1 ? 5 : 6;
	}
	moves.push_back(m);
}

} // End of anonymous namespace

bool planWalk(const ScnFile &scn, const Scene &scene, const SceneObject &olga,
			  const Common::Point &target, Common::Array<SceneObject::Move> &moves) {
	moves.clear();
	if (scn.points.empty())
		return false;
	// Responses_WalkPath (5427:1a4d)
	Common::Point feet = scene.feet(olga);
	int to = nearestPoint(scn, target);
	int edge = nearestEdge(scn, feet);
	int from = edge ? scn.edges[edge - 1].x : nearestPoint(scn, feet);
	Common::Array<int> path;
	if (!findPath(scn, from, to, path))
		return false;
	if (edge && path.size() > 1 && path[1] == scn.edges[edge - 1].y)
		path.remove_at(0);
	// WalkGraph_MakeMoves (1ebf:0a0d)
	const Common::Point &first = scn.points[path[0] - 1];
	if (first != feet)
		addMove(first.x - feet.x, first.y - feet.y, olga.def.speedX, olga.def.speedY, moves);
	for (uint i = 0; i + 1 < path.size(); i++) {
		const Common::Point &a = scn.points[path[i] - 1], &b = scn.points[path[i + 1] - 1];
		if (a != b)
			addMove(b.x - a.x, b.y - a.y, olga.def.speedX, olga.def.speedY, moves);
	}
	debugC(1, kDebugGraphics, "walk from point %d to %d: %d moves", from, to, moves.size());
	return !moves.empty();
}

} // End of namespace Olga
