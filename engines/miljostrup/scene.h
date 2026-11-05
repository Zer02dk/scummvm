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

#ifndef MILJOSTRUP_SCENE_H
#define MILJOSTRUP_SCENE_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/rect.h"
#include "graphics/managed_surface.h"

#include "miljostrup/archive.h"
#include "miljostrup/rooms.h"

namespace Miljostrup {

enum FigureState {
	kStateNone = 0,
	kStateStand = 1,     // player facing the viewer; most objects: their normal look
	kStateClosed = 2,    // doors
	kStateWalkLeft = 3,
	kStateWalkRight = 4,
	kStateWalkUp = 5,
	kStateWalkDown = 6,
	kStateTalk = 7
};

/** One queued straight walk: state to show, distance and number of steps. */
struct WalkCommand {
	int state;
	int dx, dy;
	int steps;
};

/**
 * An animated figure of a room (units 2381/24d2): a position, a state that
 * selects an animation sequence, and a queue of straight walks. It takes one
 * animation step every `ticks` frames; a step moves one walk step and shows
 * the next image of the sequence.
 */
struct Figure {
	const FigureInfo *info = nullptr;
	int x = 0, y = 0;            // top left of the image, frame offset included
	int state = kStateNone;
	int pendingState = kStateNone;
	const FigureFrame *frame = nullptr;
	uint imageIndex = 0;
	int countdown = 1;

	Common::Array<WalkCommand> queue;
	bool moving = false;
	WalkCommand command = { 0, 0, 0, 0 };
	int remaining = 0;
	int movedX = 0, movedY = 0;
	bool arrived = false;        // set when the last walk has ended
	// A route started again when the last walk ends, in the same step (the
	// walk-end message, 24d2:0f53 VMT 0x4c, sets the position and queues it)
	Common::Array<WalkCommand> route;
	int routeX = 0, routeY = 0;
	bool visible = true;         // 24d2:1168
};

/** A room with a walking player: the part of a room unit built on class b000. */
class Scene {
public:
	Scene(ImageArchive *archive, Common::HashMap<uint16, bool> *doors);
	~Scene();

	/** Set up `room`, with the player at `entryNode` (1-based). */
	bool load(const RoomInfo *room, int entryNode);
	const RoomInfo *getRoom() const { return _room; }

	/** One frame (1/25 s). */
	void update();
	/** Draw background and figures to the room's place on `dst`. */
	void draw(Graphics::ManagedSurface *dst);

	/** The object of the region at a screen position (topmost), or 0. */
	int objectAt(const Common::Point &pos) const;
	/** Walk to the region of `object` that contains `pos`; arrival is reported once. */
	void walkToObject(const Common::Point &pos, int object);
	/** True once when the player has reached the object of walkToObject(). */
	bool takeArrival(int &object);
	bool isWalking() const;
	/** b000:0477: stop the player where he is. */
	void stopWalking();
	/** b000:0295: the player plays the talk animation for `steps` steps. */
	void startTalk(int steps);
	/** The player's figure VMT 0x48: Walk(state, 0, 0, steps) in place. */
	void playPlayer(int state, int steps);
	/** Set the player's state, e.g. 7 talking (b000:03c7) or 1 standing. */
	void setPlayerState(int state);
	/** Figure VMT 0x48 (24d2:0e..): a walk of `steps` animation steps for another figure. */
	void queueFigureWalk(uint16 object, int state, int dx, int dy, int steps);
	/** The node nearest to the player's feet (for save games). */
	int getPlayerNode() const { return nearestNode(getFeet()); }
	/** Sets a route that starts again at (x, y) whenever the figure's walks end. */
	void setFigureRoute(uint16 object, int x, int y, const WalkCommand *walks, int count);
	/** Figure VMT 0x50: stop and forget the queued walks. */
	void stopFigure(uint16 object);
	/** True while the object's figure still walks (2381:0065 sends its message after that). */
	bool isFigureBusy(uint16 object) const;
	/** Figure VMT 0x34: the figure's x. */
	int getFigureX(uint16 object) const;
	/** Figure VMT 0x40 (SetPos): the object's figure to (x, y), room coordinates. */
	void setFigurePos(uint16 object, int x, int y);
	/** Figure VMT 0x3c (SetState): the state from the next animation step on. */
	void setFigureState(uint16 object, int state);
	/** 24d2:1168 (SetVisible): show or hide the object's figure. */
	void setFigureVisible(uint16 object, bool visible);

	void walkTo(int node);
	/** Debugging: the player jumps to the walk's end at once (fast_walk=true). */
	void setFastWalk(bool on) { _fastWalk = on; }
	/** Give the figures bound to `object` the state its class decides. */
	void updateObject(uint16 object);
	/** updateObject for every bound figure that is not walking (room rebuilt). */
	void refreshFigures();

	// Debugging (console "figure"): the room's figures by index
	uint getFigureCount() const { return _figures.size(); }
	const Figure &getFigureAt(uint index) const { return _figures[index]; }
	/** The figure's state from the next animation step on; its walks are dropped. */
	void debugSetState(uint index, int state);
	/** Queue a walk for the figure, as queueFigureWalk does by object. */
	void debugWalk(uint index, int state, int dx, int dy, int steps);

private:
	Graphics::ManagedSurface *getImage(uint16 number);
	void freeImages();

	void setState(Figure &fig, int state);
	/** Apply a pending state now; false if there was none. */
	bool switchState(Figure &fig);
	void showFigure(Figure &fig, bool visible);
	int initialState(Figure &fig);
	void advance(Figure &fig);
	void startCommand(Figure &fig);
	void step(Figure &fig);
	void animate(Figure &fig);
	uint16 currentImage(const Figure &fig) const;

	Common::Point getFeet() const;
	int nearestNode(const Common::Point &p) const;
	int nearestEdge(const Common::Point &p) const;
	bool findPath(int from, int to, Common::Array<int> &path) const;
	void queueSegment(int dx, int dy);
	void arrive();

	ImageArchive *_archive;
	Common::HashMap<uint16, bool> *_doors;
	Common::HashMap<uint16, Graphics::ManagedSurface *> _images;
	Graphics::ManagedSurface _roomSurface;

	const RoomInfo *_room = nullptr;
	Common::Array<Figure> _figures;
	Common::Array<int> _drawOrder;   // indices into _figures, kept sorted between frames
	int _player = -1;

	int regionAt(const Common::Point &pos) const;

	int _targetObject = 0;           // object the player is walking to
	bool _arrived = false;
	bool _fastWalk = false;
};

} // End of namespace Miljostrup

#endif // MILJOSTRUP_SCENE_H
