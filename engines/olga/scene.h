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

#ifndef OLGA_SCENE_H
#define OLGA_SCENE_H

#include "common/array.h"
#include "graphics/managed_surface.h"

#include "olga/lbm.h"
#include "olga/locations.h"
#include "olga/scn.h"

namespace Olga {

enum {
	kRoomWidth = 320,
	kRoomHeight = 138
};

/**
 * A sprite of the scene: a static sprite of the SCN, an animated object, or
 * Olga. Objects follow the original's scene objects (unit Scene 1fab,
 * notes/animation.md): a delay in frames, a requested animation that is
 * switched to at the next step, step lists that loop.
 */
struct SceneObject {
	bool animated = false;
	int x = 0, y = 0;      // top-left, room coordinates, animation offset included
	int bias = 0;          // depth bias: the static's, or the animation's p3
	int image = 0;         // static sprites: the image number

	ScnObject def;         // animated objects
	int delay = 0;
	int last = 0;          // frame of the last step
	int anim = 0;          // current animation, 1-based, 0 = none yet
	int pending = 0;       // Scene_SetAnim's request, 0 = none
	uint stepPos = 0;      // index into the animation's steps
	int frameBase = 0;     // added to frame numbers (Olga: her sprite file)

	// Script (Scene_AddMove ... Scene_MoveStep): a list of moves, the event
	// posted to the room when the list is done
	struct Move {
		int dx, dy, anim, steps;
	};
	Common::Array<Move> moves;
	bool moving = false;   // +0x12c0
	uint moveIndex = 0;    // +0x131f, 1-based
	Move cur = { 0, 0, 0, 0 };
	int stepsLeft = 0;     // +0x131d
	int startX = 0, startY = 0; // +0x1321 / +0x1323
	int event = 2;         // +0x1329; 2 = none
	int hotDx = 0, hotDy = 0; // the hotspot moves with the object's moves (1e9c:0167)

	int currentImage() const;
};

/** A room event posted by an object's finished script (Responses_RoomEvent). */
struct SceneEvent {
	int code;
	int object;
};

/**
 * Receives the room events. Gui_01d1 calls the control's event method at
 * once, so the event is handled inside the object's step, before its
 * animation advances (a new script's animation replaces the advance).
 */
class SceneListener {
public:
	virtual ~SceneListener() {}
	virtual void sceneEvent(const SceneEvent &e) = 0;
};

class Scene {
public:
	/** Room_Init: the room's pictures, Olga's sprite file, the SCN and the PRS. */
	bool load(const LocationDef &loc, int entry);

	/** One frame of Scene_Tick (1fab:1aef). */
	void tick();
	/** Background and sprites sorted by depth (Scene_DrawAll). */
	void draw(Graphics::ManagedSurface &dst) const;

	const byte *palette() const { return _room.palette(); }
	/** Image n of the combined list: the room's images, then Olga's. */
	const LbmImage *image(int n) const;

	/** Scene_SetAnim: switch to animation n at the object's next step. */
	static void setAnim(SceneObject &obj, int n) { obj.pending = n; }
	/** Scene_StartScript: run the object's moves; event when they are done. */
	void startScript(SceneObject &obj, int event);
	/** Stop a script (Scene_12d3) without its event. */
	static void stopScript(SceneObject &obj);
	/** Scene1e9c_0087 (method 0): place the object; its hotspot moves along. */
	static void setPosition(SceneObject &obj, int x, int y) {
		obj.hotDx += x - obj.x;
		obj.hotDy += y - obj.y;
		obj.x = x;
		obj.y = y;
	}
	/** Apply the requested animations at once (after the handlers' refresh). */
	void settle();
	void setListener(SceneListener *l) { _listener = l; }

	/** Olga's feet: centre x and the sort key (Responses_1a4d). */
	Common::Point feet(const SceneObject &obj) const;
	int imageWidth(const SceneObject &obj) const;

	SceneObject *olga() { return _olga < 0 ? nullptr : &_objects[_olga]; }
	int olgaIndex() const { return _olga; }
	/** The SCN object whose hotspot has this id, or nullptr. */
	SceneObject *findObject(int hotspotId);
	const ScnFile &scn() const { return _scn; }

private:
	void switchAnim(SceneObject &obj, int n);
	void step(SceneObject &obj);
	void loadMove(SceneObject &obj);
	void scriptStep(SceneObject &obj, int index);
	int sortKey(const SceneObject &obj) const;

	LbmFile _room;
	LbmFile _sprites;
	ScnFile _scn;
	Common::Array<SceneObject> _objects;
	int _olga = -1;
	int _counter = 0; // frames, mod 1000
	SceneListener *_listener = nullptr;
	void post(int code, int object) {
		if (_listener)
			_listener->sceneEvent({ code, object });
	}
};

} // End of namespace Olga

#endif // OLGA_SCENE_H
