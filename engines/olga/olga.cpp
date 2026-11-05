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

#include "audio/softsynth/pcspk.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/cursorman.h"
#include "graphics/paletteman.h"

#include "olga/console.h"
#include "olga/locations_data.h"
#include "olga/olga.h"
#include "olga/states_data.h"

namespace Olga {

OlgaEngine *g_engine;

OlgaEngine::OlgaEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Olga") {
	g_engine = this;
	_scene.setListener(this);
	memset(_controlOff, 0, sizeof(_controlOff));
	resetGame();
	for (int n = 0; n < kNumLocations; n++)
		for (int e = 0; e < kMaxEntries; e++)
			_entries[n][e] = { kLocations[n].entries[e].x, kLocations[n].entries[e].y };
}

void OlgaEngine::resetGame() {
	// Main_NewGame (1000:1854): ObjNames_InitStates (21b9:045a) sets the
	// states, the inventory is empty, DS:2590 = 1 (the devil's first text)
	memset(_flags, 0, sizeof(_flags));
	for (uint i = 0; i < ARRAYSIZE(kStates); i++)
		_flags[kStates[i].id] = kStates[i].flags;
	for (int i = 0; i <= kLastId; i++)
		_names[i].clear();
	_inventory.clear();
	_inventoryRow = 1;
	_devilFirst = true;
	_pendingDevil = _pendingPraise = false;
}

OlgaEngine::~OlgaEngine() {
	delete _speaker;
	delete _screen;
}

uint32 OlgaEngine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String OlgaEngine::getGameId() const {
	return _gameDescription->gameId;
}

bool OlgaEngine::loadExeData() {
	if (!_exe.open() || !_font.loadPatch(_exe))
		return false;
	// Verb buttons: 51x9 images at 19c0:0f84 + 0x1d1 * k (Responses_MakeVerbButtons)
	for (int k = 0; k < kNumVerbs; k++)
		if (!_exe.readBitmap(0x19c0, 0x0f84 + 0x1d1 * k, _verbImages[k]))
			return false;
	// The OK button of KemSkab and BordStol, the image before Skub
	if (!_exe.readBitmap(0x19c0, 0x0db3, _okImage))
		return false;
	// SkiftTid: Kort, Mellem, Lang, OK (19c0:0005 + 0x200 * k); the quit
	// menu: Afslut, Forfra, Fortsæt
	for (int k = 0; k < 4; k++)
		if (!_exe.readBitmap(0x19c0, 0x0005 + 0x200 * k, _speedImages[k]))
			return false;
	if (!_exe.readBitmap(0x19c0, 0x0a27, _diskImages[0]) || !_exe.readBitmap(0x19c0, 0x0bed, _diskImages[1]) ||
		!_exe.readBitmap(0x19c0, 0x2421, _yesImage) || !_exe.readBitmap(0x19c0, 0x263a, _noImage))
		return false;
	if (!_exe.readBitmap(0x19c0, 0x1fdd, _quitImages[0]) || !_exe.readBitmap(0x19c0, 0x21ff, _quitImages[1]) ||
		!_exe.readBitmap(0x19c0, 0x0805, _quitImages[2]))
		return false;
	// Inventory arrows (InvPanel_Init) and the 17 icons of 23x18 (unit Icons)
	if (!_exe.readBitmap(0x1c51, 0x0507, _arrowImages[0]) || !_exe.readBitmap(0x1c51, 0x059d, _arrowImages[1]))
		return false;
	for (int n = 1; n <= 17; n++)
		if (!_exe.readBitmap(0x22d2, 0x0131 + 0x1a4 * (n - 1), _icons[n]))
			return false;
	return true;
}

void OlgaEngine::setCursor(uint16 maskOff) {
	// Gui_1156 copies the 16 AND words and 16 XOR words, Gui_10a5 draws them
	// per pixel (bit 15 = left): AND 0 -> colour 0; XOR 1 -> 15 (or the
	// inverted screen pixel where AND is 1, which these masks do not use);
	// AND 1 and XOR 0 -> transparent. The hot spot is the top-left corner.
	const byte kTransparent = 1;
	const byte *p = _exe.data(kSegData, maskOff, 64);
	if (!p)
		return;
	byte buf[16 * 16];
	for (int y = 0; y < 16; y++) {
		uint16 andMask = READ_LE_UINT16(p + 2 * y);
		uint16 xorMask = READ_LE_UINT16(p + 32 + 2 * y);
		for (int x = 0; x < 16; x++) {
			bool a = andMask & (0x8000 >> x), b = xorMask & (0x8000 >> x);
			buf[y * 16 + x] = b ? 15 : (a ? kTransparent : 0);
		}
	}
	CursorMan.replaceCursor(buf, 16, 16, 0, 0, kTransparent);
	CursorMan.showMouse(true);
}

void OlgaEngine::waitOrKey(uint32 ms) {
	uint32 end = g_system->getMillis() + ms;
	while (!shouldQuit() && g_system->getMillis() < end) {
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN || e.type == Common::EVENT_LBUTTONDOWN)
				return;
		}
		_screen->update();
		g_system->delayMillis(10);
	}
}

bool OlgaEngine::showPicture(const Common::Path &name) {
	LbmFile lbm;
	if (!lbm.load(name) || !lbm.hasPalette())
		return false;
	g_system->getPaletteManager()->setPalette(lbm.palette(), 0, 256);
	_screen->clear(0);
	_screen->blitFrom(lbm.image(1).surface, Common::Point(0, 0));
	_screen->update();
	return true;
}

bool OlgaEngine::gotoLocation(int number, int entry) {
	if (number < 1 || number > kNumLocations)
		return false;
	LocationDef loc = kLocations[number - 1];
	// The return set (1000:02a6, tested at 03e3): the room is kept to go
	// back to
	for (uint i = 0; i < ARRAYSIZE(kReturnLocations); i++)
		if (number == kReturnLocations[i] && _kind == kLocRoom && _location)
			_returnLocation = _location;
	if (loc.kind == kLocPopup)
		return gotoPopup(number);
	if (loc.kind == kLocKemSkab)
		return gotoKemSkab();
	if (loc.kind == kLocBordStol)
		return gotoBordStol(number);
	if (loc.kind == kLocTextSpeed)
		return gotoTextSpeed();
	if (loc.kind == kLocQuitMenu)
		return gotoQuitMenu();
	if (loc.kind == kLocGameEnd)
		return gotoGameEnd();
	if (loc.kind == kLocSaveLoad)
		return gotoSaveLoad(number);
	if (loc.kind != kLocRoom)
		return false;
	for (int e = 0; e < kMaxEntries; e++) {
		loc.entries[e].x = _entries[number - 1][e].x;
		loc.entries[e].y = _entries[number - 1][e].y;
	}
	if (!_scene.load(loc, entry))
		return false;
	_location = number;
	_kind = kLocRoom;
	_nextLocation = 0;
	_pending = false;
	_textTimer = 0;
	_verb = kVerbBrug;
	_roomView.create(kRoomWidth, kRoomHeight, Graphics::PixelFormat::createFormatCLUT8());
	setInput(true);
	// Every hotspot's handler is attached and refreshed (method 1 -> 11):
	// objects set their animation, cupboards show or hide their contents
	memset(_controlOff, 0, sizeof(_controlOff));
	for (uint i = 0; i < _scene.scn().objects.size(); i++)
		refresh(_scene.scn().objects[i].hotspot.id);
	for (uint i = 0; i < _scene.scn().hotspots.size(); i++)
		refresh(_scene.scn().hotspots[i].id);
	_scene.settle();
	// Scene_17a4: the room's whole CMAP
	g_system->getPaletteManager()->setPalette(_scene.palette(), 0, 256);
	_screen->clear(0);
	drawPanel();
	resetSentence();
	startPendingFeedback();
	return true;
}

void OlgaEngine::exitGame() {
	// The exit procedure (1000:1901, installed by Main_Body): with a mouse,
	// rultekst.lbm (the credits) for 32 s or a key, the mouse hidden
	CursorMan.showMouse(false);
	if (showPicture("RULTEKST.LBM"))
		waitOrKey(32000);
	quitGame();
}

void OlgaEngine::leaveRoom() {
	// RoomLoc_Run (1000:02c6): Olga's place, without her animation offset,
	// becomes entry 1 of the room she leaves
	SceneObject *olga = _scene.olga();
	if (_kind == kLocRoom && _location > 0 && olga && olga->anim >= 1) {
		const ScnAnim &a = olga->def.anims[olga->anim - 1];
		_entries[_location - 1][0] = { (int16)(olga->x - a.dx), (int16)(olga->y - a.dy) };
	}
	// A loaded save: Olga's saved place becomes its room's entry 1
	if (_restoreLocation) {
		_entries[_restoreLocation - 1][0] = _restoreEntry;
		_restoreLocation = 0;
	}
	int loc = _nextLocation, entry = _nextEntry;
	_nextLocation = 0;
	if (loc >= 100) {
		// Main_ShowEnding (1000:1a84): 100 = politi.lbm (101, faerdig.lbm,
		// is never reached), 32 s or a key, then the game ends
		if (showPicture(loc == 100 ? "POLITI.LBM" : "FAERDIG.LBM"))
			waitOrKey(32000);
		exitGame();
		return;
	}
	if (!gotoLocation(loc, entry))
		warning("Olga: location %d is not implemented yet", loc);
}

Common::Error OlgaEngine::run() {
	initGraphics(320, 200);
	_screen = new Graphics::Screen();
	setDebugger(new Console());
	if (!loadExeData())
		return Common::kReadingFailed;
	if (!initSound())
		warning("Olga: no PC speaker");

	// Main_Body: the hourglass, two title pictures of 6 s each (1000:198b,
	// 1000:1994)
	// Debugging: "skip_intro=true" in the game's section of scummvm.ini
	// starts in the first room
	setCursor(0x138e);
	bool skipIntro = ConfMan.hasKey("skip_intro") && ConfMan.getBool("skip_intro");
	if (!skipIntro) {
		if (!showPicture("OLGA.LBM"))
			return Common::kReadingFailed;
		waitOrKey(6000);
		if (!shouldQuit() && showPicture("REKLAME.LBM"))
			waitOrKey(6000);
	}
	// Debugging: "fast_walk=true" makes Olga jump to where she walks (for
	// testing; not in the original)
	_fastWalk = ConfMan.hasKey("fast_walk") && ConfMan.getBool("fast_walk");

	// The room view is enabled: the arrow (App_02e9)
	setCursor(0x134e);

	// Main_NewGame: location 9 (GANG5), entry 3
	_roomView.create(kRoomWidth, kRoomHeight, Graphics::PixelFormat::createFormatCLUT8());
	// or a save chosen in the launcher
	int launcherSlot = ConfMan.hasKey("save_slot") ? ConfMan.getInt("save_slot") : -1;
	if (launcherSlot >= 0 && loadGameState(launcherSlot).getCode() == Common::kNoError) {
		leaveRoom();
	} else if (!shouldQuit() && !gotoLocation(kStartLocation, kStartEntry)) {
		return Common::kReadingFailed;
	}

	while (!shouldQuit()) {
		uint32 start = g_system->getMillis();
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			switch (e.type) {
			case Common::EVENT_MOUSEMOVE:
				if (_inputEnabled && _kind == kLocRoom)
					onHover(e.mouse);
				else if (_inputEnabled && _kind == kLocKemSkab)
					kemHover(e.mouse);
				break;
			case Common::EVENT_LBUTTONDOWN:
				if (!_inputEnabled)
					break;
				if (_kind == kLocPopup) {
					int k = popupHitTest(e.mouse);
					if (k)
						popupAnswer(k);
				} else if (_kind == kLocKemSkab) {
					kemClick(e.mouse);
				} else if (_kind == kLocBordStol) {
					bordClick(e.mouse);
				} else if (_kind == kLocTextSpeed) {
					textSpeedClick(e.mouse);
				} else if (_kind == kLocQuitMenu) {
					quitMenuClick(e.mouse);
				} else if (_kind == kLocSaveLoad) {
					saveLoadClick(e.mouse);
				} else if (_kind == kLocRoom) {
					onClick(e.mouse);
				}
				break;
			case Common::EVENT_KEYDOWN:
				if (_kind == kLocGameEnd)
					gameEndDone(); // GameEnd_02a9: any key
				else if (_inputEnabled && _kind == kLocRoom)
					onKey(e.kbd);
				else if (_inputEnabled && _kind == kLocSaveLoad)
					saveLoadKey(e.kbd);
				else if (_inputEnabled && (_kind == kLocTextSpeed || _kind == kLocQuitMenu) &&
						 e.kbd.keycode == Common::KEYCODE_ESCAPE)
					// Not in the original (SkiftTid_04f8 only ends the text, the
					// quit menu has no Esc): Esc is OK / Fortsæt
					setExit(_returnLocation, 1);
				break;
			default:
				break;
			}
		}
		// One frame of the room view's idle method (Room_021e)
		if (_textTimer > 0 && --_textTimer == 0)
			clearText();
		// BordStol and the quit menu have no scene: they draw on the screen
		// once
		bool scene = _kind != kLocBordStol && _kind != kLocQuitMenu && _kind != kLocSaveLoad;
		if (scene)
			_scene.tick();
		if (_nextLocation)
			leaveRoom();
		scene = _kind != kLocBordStol && _kind != kLocQuitMenu && _kind != kLocSaveLoad;
		if (scene) {
			_scene.draw(_roomView);
			_screen->blitFrom(_roomView, roomViewPos());
		}
		_screen->update();
		uint32 used = g_system->getMillis() - start;
		if (used < kFrameMs)
			g_system->delayMillis(kFrameMs - used);
	}

	return Common::kNoError;
}

} // End of namespace Olga
