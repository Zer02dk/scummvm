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

class OlgaEngine : public Engine, public SceneListener {
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
	bool _controlOff[kLastId + 1];      // hotspots a refresh switched off (control +0x20)
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

	// The kind of the current location: a room, a pop-up or a special screen
	LocationKind _kind = kLocRoom;
	// RoomLoc_Run: a room left for a screen of the return set (1000:02a6)
	// is kept in DS:2178; the screen goes back there, entry 1
	int _returnLocation = 0;

	// Pop-ups (location type kLocPopup, overlay 53cd)
	int _popupAnswers = 0;
	LbmImage _answerImages[4];

	// Images built into OLGA.EXE
	LbmImage _verbImages[kNumVerbs];
	LbmImage _arrowImages[2];
	LbmImage _icons[18];    // 1..17
	LbmImage _okImage;      // 19c0:0db3
	LbmImage _speedImages[4]; // Kort, Mellem, Lang, OK
	LbmImage _quitImages[3];  // Afslut, Forfra, Fortsæt

	// KemSkab (overlay 52ee)
	struct KemItem {
		int id;
		Common::Rect hot;   // room view coordinates, inclusive
	};
	Common::Array<KemItem> _kemItems; // in the order their controls were inserted
	uint16 _kemChosen = 0;  // view +0xcae, set of 1..9
	int _kemHover = 100;    // view +0xcac
	// BordStol (overlay 502f)
	LbmFile _bordLbm;
	int _bordId = 0;        // +0x6ab: 225 or 226
	int _bordChoice = 0;    // +0x6a7
	int _bordBase = 0;      // +0x6a9: first picture - 1

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
	/** Room_1632 (method +0x38): the message line, no timer, Olga does not talk. */
	void showText(const Common::String &text, byte color);
	void verbBase(int verb);
	/** Things (notes/verbs.md): Tag, Brug / Giv on the home, Se på. */
	bool verbThing(int id, int verb, int other);
	const ThingDef *thing(int id) const;
	/** Thing_Refresh's "control off": carried (bit 0), or bit 0x100. */
	bool hotspotOff(int id) const;
	/** The control's method +0x20: a refresh switches the hotspot on or off. */
	void setControl(int id, bool on) { _controlOff[id] = !on; }
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

	// Special screens (special.cpp)
	bool gotoKemSkab();
	void kemPlace(int item, int pos);
	void kemText(const Common::String &text, bool chosen);
	int kemHitTest(const Common::Point &p) const;
	void kemHover(const Common::Point &p);
	void kemClick(const Common::Point &p);
	void kemEvent(const SceneEvent &e);
	bool gotoBordStol(int number);
	void bordSelect(int choice);
	void bordClick(const Common::Point &p);
	bool gotoTextSpeed();
	void drawSpeedButtons();
	void textSpeedClick(const Common::Point &p);
	bool gotoQuitMenu();
	void quitMenuClick(const Common::Point &p);
	bool gotoGameEnd();
	void gameEndDone();
	/** Where the current location's room view is on the screen. */
	Common::Point roomViewPos() const;

	/** Main_NewGame's state: ObjNames_InitStates, empty inventory. */
	void resetGame();
	/** The handlers' method 12 summed over 100..231 (20 = all solved). */
	int score() const;
	/** F5, Responses_0590. */
	void showStatus();
	/** Situations_54d6: the ENTRE front door 194. */
	void endCheck();

protected:
	Common::Error run() override;

public:
	Graphics::Screen *_screen = nullptr;

	OlgaEngine(OSystem *syst, const ADGameDescription *gameDesc);
	/** Room events, at once from the object's step (Gui_01d1). */
	void sceneEvent(const SceneEvent &e) override { handleEvent(e); }
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

	// Debugger (console.cpp)
	uint32 getFlag(int id) const { return _flags[id]; }
	int getScore() const { return score(); }
	void setFlag(int id, uint32 value) { _flags[id] = value; refresh(id); }
	/** Thing_Tag without the walk: carried and in the inventory. */
	void giveThing(int id) { _flags[id] |= 1; refresh(id); addToInventory(id); }
};

extern OlgaEngine *g_engine;
#define SHOULD_QUIT ::Olga::g_engine->shouldQuit()

} // End of namespace Olga

#endif // OLGA_H
