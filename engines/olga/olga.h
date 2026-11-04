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

#ifndef OLGA_H
#define OLGA_H

#include "common/array.h"
#include "common/error.h"
#include "common/keyboard.h"
#include "common/random.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "common/system.h"
#include "engines/engine.h"
#include "graphics/screen.h"

#include "olga/detection.h"
#include "olga/exe.h"
#include "olga/font.h"
#include "olga/lbm.h"
#include "olga/locations.h"
#include "olga/scene.h"
#include "olga/states.h"

namespace Olga {

// Screen layout of a room (notes/interface.md)
enum {
	kRoomTop = 8,          // the 320x138 background is drawn at y 8
	kSentenceTop = 146,    // sentence line 146..155, text at y 147
	kPanelTop = 156        // verbs and inventory below
};

// The commands of the verb buttons (Responses_MakeVerbButtons), in button order
enum Verb {
	kVerbSkub = 0x32,
	kVerbTraek,
	kVerbBrug,
	kVerbTag,
	kVerbGiv,
	kVerbSePaa,
	kVerbAaben,
	kVerbLuk,
	kVerbTalTil,
	kNumVerbs = 9
};

enum {
	kCmdInvUp = 0x3b,      // inventory arrows (InvPanel_Init)
	kCmdInvDown = 0x3c,
	kEventArrived = 3      // Olga's walk ended: run the pending verb
};

// One frame of the original's loop: 4 ticks of its 100 Hz timer
static const uint32 kFrameMs = 40;

class OlgaEngine : public Engine {
private:
	const ADGameDescription *_gameDescription;
	Common::RandomSource _randomSource;
	ExeFile _exe;
	GameFont _font;
	Scene _scene;
	Graphics::ManagedSurface _roomView; // 320x138, drawn at y 8
	int _location = 0;
	int _nextLocation = 0, _nextEntry = 0; // Room_SetExit
	struct Entry { int16 x, y; };
	Entry _entries[kNumLocations][kMaxEntries]; // entry 1 is overwritten on leaving

	// Game state: the records' flags (DS:1ca1) and the inventory (DS:3ef0)
	uint32 _flags[kLastId + 1];
	Common::Array<int> _inventory;
	Common::String _names[kLastId + 1]; // a name a refresh wrote, or empty
	int _inventoryRow = 1;  // InvPanel +0x1df

	// The sentence (room view +0xcad..+0xcb5)
	int _verb = kVerbBrug;  // Main_02c6 passes 0x34
	int _stage = 1;
	int _obj1 = kNoObject, _obj2 = kNoObject;
	bool _pending = false;  // walking to the object, verb runs on arrival

	// Message line (Room_Say)
	int _textTimer = 0;
	int _textSpeed = 6;     // DS:27a4, frames per character (SkiftTid)

	// The room's input (App_02e9): off while the devil or the praise plays
	bool _inputEnabled = true;
	bool _fastWalk = false;    // debug setting fast_walk
	// Feedback (notes/situation-logic.md)
	int _devilCount = 0;       // room view +0xcc2
	bool _devilFirst = true;   // DS:2590: "HA!!! Forkert!" the first time
	bool _pendingDevil = false;  // DS:2592: shown when the next room starts
	bool _pendingPraise = false; // DS:2593

	// Pop-ups (location type kLocPopup, overlay 53cd)
	bool _popup = false;
	int _popupAnswers = 0;
	LbmImage _answerImages[4];

	// Images built into OLGA.EXE
	LbmImage _verbImages[kNumVerbs];
	LbmImage _arrowImages[2];
	LbmImage _icons[18];    // 1..17

	/** Main_ShowPicture: a full-screen LBM with its palette, image 1 at 0,0. */
	bool showPicture(const Common::Path &name);
	/** KeyWait_WaitMsOrKey: wait ms milliseconds or until a key (or click). */
	void waitOrKey(uint32 ms);
	bool loadExeData();
	/**
	 * Gui_1156: one of the game's two 16x16 cursors, an AND / XOR mask pair
	 * in the data segment (DS:134e arrow, DS:138e hourglass).
	 */
	void setCursor(uint16 maskOff);

	// Interface (interface.cpp)
	void drawPanel();
	void drawButton(const LbmImage &img, int x, int y, bool pressed, int bevel = 1);
	void drawVerbs();
	void drawInventory();
	void drawSentence(bool running);
	/** The command under the mouse: a hotspot or item id, a button, or 100. */
	int hitTest(const Common::Point &p);
	void onHover(const Common::Point &p);
	void onClick(const Common::Point &p);
	void onKey(const Common::KeyState &ks);
	void selectVerb(int verb);

	// Room logic (room.cpp)
	void resetSentence();
	void startVerb();
	void runVerb();
	void cancelWalk();
	void handleEvent(const SceneEvent &e);
	void say(const Common::String &text, byte color = 0xe, bool talk = true);
	void sayExe(uint16 seg, uint16 off, byte color = 0xe, bool talk = true);
	void clearText();
	void verbBase(int verb);
	/** Things (notes/verbs.md): Tag, Brug / Giv on the home, Se på. */
	bool verbThing(int id, int verb, int other);
	const ThingDef *thing(int id) const;
	/** Thing_Refresh's "control off": carried (bit 0), or bit 0x100. */
	bool hotspotOff(int id) const;
	void addToInventory(int id);
	void removeFromInventory(int id);
	void verbDoor(int id, int verb);
	void goThrough(int id);
	bool isDoor(int id) const;
	void refresh(int id);
	void setExit(int location, int entry) { _nextLocation = location; _nextEntry = entry; }
	void leaveRoom();
	Common::String objectName(int id) const;
	bool carried(int id) const;
	void setInput(bool enabled);

	// The situations (situations.cpp, notes/situation-logic.md)
	/** A situation class's verb; false if the class has none for it. */
	bool verbSituation(int id, int verb, int other);
	/** A situation class's refresh; false if it has its own none. */
	bool refreshSituation(int id, SceneObject &o);
	/** Room events of the situations (Responses_RoomEvent). */
	void situationEvent(const SceneEvent &e);
	/** S[134] = 1 (the devil) or S[190] = 1 (the praise), and refresh it. */
	void showDevil();
	void showPraise();
	/** Responses_19c6: a pop-up's devil or praise when the room starts. */
	void startPendingFeedback();
	/** Lamps (Lamp_Brug 1222:472d). */
	bool verbLamp(int id, const ThingDef &t, int other);

	// Pop-ups (popup.cpp)
	bool gotoPopup(int number);
	void drawPopup();
	int popupHitTest(const Common::Point &p);
	void popupAnswer(int choice);

protected:
	Common::Error run() override;

public:
	Graphics::Screen *_screen = nullptr;

	OlgaEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~OlgaEngine() override;

	uint32 getFeatures() const;
	Common::String getGameId() const;

	uint32 getRandomNumber(uint maxNum) {
		return _randomSource.getRandomNumber(maxNum);
	}

	bool hasFeature(EngineFeature f) const override {
		return f == kSupportsReturnToLauncher;
	}

	/** Load a room location (1..34) with Olga at the given entry. */
	bool gotoLocation(int number, int entry);
	int getLocation() const { return _location; }
};

extern OlgaEngine *g_engine;
#define SHOULD_QUIT ::Olga::g_engine->shouldQuit()

} // End of namespace Olga

#endif // OLGA_H
