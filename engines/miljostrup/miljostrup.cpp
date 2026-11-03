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

#include "common/config-manager.h"
#include "common/events.h"
#include "common/file.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/framelimiter.h"
#include "graphics/paletteman.h"

#include "miljostrup/miljostrup.h"
#include "miljostrup/console.h"
#include "miljostrup/rooms.h"
#include "miljostrup/interface.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/verbs_data.h"
#include "miljostrup/sound_data.h"

namespace Miljostrup {

enum {
	kMessageFramesPerChar = 6 // DS:cdea, set by b000:06de
};

// Two 768-byte 6-bit VGA palettes, back to back, in a data segment of the EXE
static const uint32 kPaletteOffset = 0x151F0;

MiljostrupEngine *g_engine;

MiljostrupEngine::MiljostrupEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Miljostrup") {
	g_engine = this;
	memset(_palettes, 0, sizeof(_palettes));
}

MiljostrupEngine::~MiljostrupEngine() {
	delete _scene;
	delete _interface;
	delete _screen;
}

uint32 MiljostrupEngine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String MiljostrupEngine::getGameId() const {
	return _gameDescription->gameId;
}

bool MiljostrupEngine::loadPalettes() {
	Common::File exe;
	if (!exe.open("MLJSTRUP.EXE"))
		return false;

	exe.seek(kPaletteOffset);
	for (int p = 0; p < 2; ++p) {
		for (int i = 0; i < 256 * 3; ++i) {
			byte v = exe.readByte();
			if (v > 63)
				return false;
			_rawPalettes[p][i] = v;
			_palettes[p][i] = v * 255 / 63;
		}
	}
	return !exe.err();
}

void MiljostrupEngine::setPalette(int palette, int dimFactor, int first, int last) {
	if (!dimFactor) {
		g_system->getPaletteManager()->setPalette(_palettes[palette], 0, 256);
		return;
	}
	// 2472:011d on the 6-bit values: red and green * f / 16; blue * f / 12
	// below factor 3, else ((12 * f) / 16) * b / 12 (notes/palette.md)
	byte pal[256 * 3];
	memcpy(pal, _palettes[palette], sizeof(pal));
	const int f = dimFactor;
	for (int i = first; i <= last; ++i) {
		const byte *src = &_rawPalettes[palette][i * 3];
		int r = (src[0] * f >> 4) & 0xff;
		int g = (src[1] * f >> 4) & 0xff;
		int b = (f < 3 ? src[2] * f / 12 : ((12 * f) >> 4) * src[2] / 12) & 0xff;
		pal[i * 3] = r * 255 / 63;
		pal[i * 3 + 1] = g * 255 / 63;
		pal[i * 3 + 2] = b * 255 / 63;
	}
	g_system->getPaletteManager()->setPalette(pal, 0, 256);
}

void MiljostrupEngine::showImage(uint number, int palette) {
	Graphics::ManagedSurface *image = _archive.loadImage(number);
	if (!image)
		return;

	setPalette(palette);
	_screen->clear();
	_screen->blitFrom(*image);
	_screen->update();
	delete image;
}

void MiljostrupEngine::drawInterface() {
	// As drawn by the original (unit ae72): black strips around the room, a
	// white line, the grey bar with the message area on the left and a 3x2
	// grid of inventory slots on the right
	_screen->fillRect(Common::Rect(0, 0, 320, 8), 0);
	_screen->fillRect(Common::Rect(0, 146, 320, 156), 0);
	_screen->hLine(0, 156, 319, 15);
	_screen->fillRect(Common::Rect(0, 157, 320, 200), 7);
	_screen->vLine(225, 157, 199, 8);
	_screen->vLine(226, 157, 199, 15);

	for (int row = 0; row < 2; ++row) {
		for (int col = 0; col < 3; ++col) {
			int x = 242 + col * 26;
			int y = 158 + row * 21;
			_screen->frameRect(Common::Rect(x, y, x + 25, y + 20), 8);
		}
	}
}

void MiljostrupEngine::enterLocation(int location) {
	// Doors keep their state while the player stays in the location
	_location = location;
	_interface->setLocation(location);
	resetObjectState(location);
	// Each location has its own manager (unit b06f): its score starts at 0
	for (int k = 0; k < 4; ++k)
		_score[k] = 0;
	_doors.clear();
	for (uint i = 0; i < getRoomCount(); ++i) {
		const RoomInfo *room = getRoomInfo(i);
		if (room->location != location)
			continue;
		for (int e = 0; e < room->exitCount; ++e) {
			const RoomExit &exit = getExit(room, e);
			if (exit.door)
				_doors[exit.object] = exit.open;
		}
	}
	initExitDoors(location);
}

bool MiljostrupEngine::gotoRoom(int room, int node) {
	leavingRoom(_room);
	const RoomInfo *info = findRoom(_location, room);
	if (!info || !info->nodeCount) {
		// Close-ups have no walk data: they are screens run by their objects'
		// handlers (closeup.cpp), never rooms to go to
		warning("Room %d of location %d has no walk data", room, _location);
		return false;
	}
	if (!_scene->load(info, node))
		return false;

	_room = room;
	bathroomBuilt();
	schoolBuilt();
	farmBuilt();
	chickenFarmBuilt();
	slaughterRoomEntered();
	factoryRoomEntered();
	setRoomPalette(info->palette);
	_screen->clear();
	drawInterface();
	_interface->setObject(kNoObject);
	_interface->draw();
	_scene->draw(_screen);
	_screen->update();
	return true;
}

void MiljostrupEngine::updateDoor(uint16 object) {
	_scene->updateObject(object);
}

void MiljostrupEngine::say(const Common::String &text, byte color, bool talk) {
	// Shown for 6 frames per character (DS:cdea = 6, b000:06de)
	_interface->showMessage(text, color);
	if (talk)
		_scene->setPlayerState(kStateTalk);
	_messageTimer = text.size() * kMessageFramesPerChar;
}

void MiljostrupEngine::clearMessage() {
	// b000:042e
	_interface->showMessage("", 0);
	_scene->setPlayerState(kStateStand);
	_messageTimer = 0;
}

bool MiljostrupEngine::isCarried(int object) const {
	for (uint i = 0; i < _inventory.size(); ++i) {
		if (_inventory[i] == object)
			return true;
	}
	return false;
}

void MiljostrupEngine::removeObject(int object) {
	// Room VMT 0x54 = 229f:0072: out of the inventory, then the display
	// (14a3:02fd: the page is set again when the list has changed)
	for (uint i = 0; i < _inventory.size(); ++i) {
		if (_inventory[i] == object) {
			_inventory.remove_at(i);
			break;
		}
	}
	int count = _inventory.size();
	_inventoryPage = count < 4 ? 1 : (count - 1) / 3;
	_interface->drawInventory();
}

void MiljostrupEngine::playSound(SoundEffectId effect) {
	// The original busy-waits while the speaker plays (1d26:0000); keep the
	// window alive, but the game does nothing else meanwhile
	_sound.play(effect);
	while (_sound.isPlaying() && !shouldQuit()) {
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
		}
		_screen->update();
		g_system->delayMillis(5);
	}
}

void MiljostrupEngine::executeVerb(int verb, int object, int second) {
	// ae72:0c42 calls the object's verb method (VMT 0x28 + 4 * (verb - 0x32))
	if (runProxyVerb(verb, object, second))
		return;
	const ObjectInfo *info = findObject(_location, object);
	if (!info) {
		debug(1, "Object %d has no class data", object);
		return;
	}
	Common::String pronoun = capitalize(readPascalString(_ovr, info->pronoun));
	bool isDoor = _doors.contains(object);
	bool open = !isDoor || _doors[object];

	switch (getHandlerKind(info, verb)) {
	case kHandlerRoot:
		sayRootMessage(verb);
		break;
	case kHandlerTalk:
		say(readPascalString(_exe, kTextHej) + _interface->getObjectName(object) +
			readPascalString(_exe, kTextExcl), 13, true);
		break;
	case kHandlerDescription:
		say(readPascalString(_ovr, info->description), 14, true);
		break;
	case kHandlerPassage: {
		// 13ec:02fb / 030e -> VMT 0x60: go through (13ec:0321), doors only
		// when open (13ec:05cb)
		if (!open) {
			say(pronoun + readPascalString(_exe, kTextDoorLocked), 14, true);
			break;
		}
		if (!canLeaveBathroom(object) || !canLeaveKitchen(object))
			break;
		const RoomInfo *room = _scene->getRoom();
		for (int i = 0; i < room->exitCount; ++i) {
			const RoomExit &exit = getExit(room, i);
			if (exit.object == object) {
				gotoRoom(exit.room, exit.node);
				return;
			}
		}
		// The exit objects (13ec:06f0 / 0883, target room 0): VMT 0x60 = 13ec:07ce
		if (getObjectClass(info).vmt == 0xbb6e || getObjectClass(info).vmt == 0xbc3a)
			useExit();
		else
			debug(1, "Passage %d leads nowhere", object);
		break;
	}
	case kHandlerDoorLook:
		say(pronoun + readPascalString(_exe, open ? kTextDoorOpen : kTextDoorClosed), 14, true);
		break;
	case kHandlerDoorOpen:
	case kHandlerDoorClose: {
		// 13ec:0484 / 13ec:050b (the sounds 1d26:010f / 0132 are not played yet)
		bool wantOpen = getHandlerKind(info, verb) == kHandlerDoorOpen;
		if (open == wantOpen) {
			say(pronoun + readPascalString(_exe, wantOpen ? kTextDoorAlreadyOpen : kTextDoorAlreadyClosed), 14, true);
		} else {
			_doors[object] = wantOpen;
			updateDoor(object);
			playSound(wantOpen ? kSoundOpen : kSoundClose);
		}
		break;
	}
	default:
		if (!runClassHandler(getHandlerAddress(info, verb), verb, object, second))
			debug(1, "Verb %d on object %d: handler %04x:%04x not implemented yet", verb, object,
				getHandlerAddress(info, verb) >> 16, getHandlerAddress(info, verb) & 0xffff);
		break;
	}
}

void MiljostrupEngine::runRoomLoop() {
	Graphics::FrameLimiter limiter(g_system, 25, false);
	int pendingVerb = 0;
	int pendingFirst = 0, pendingSecond = kNoObject;
	while (!shouldQuit() && !_phaseDone) {
		Common::Event e;
		for (;;) {
			// ScummVM's main menu (save / load) opens inside pollEvent
			_canSaveNow = true;
			bool got = g_system->getEventManager()->pollEvent(e);
			_canSaveNow = false;
			if (!got || _loadedGame)
				break;
			if (_award.active || _inputOff)
				continue; // input is off until the award has been shown (or a room turned it off)
			if (e.type == Common::EVENT_MOUSEMOVE) {
				// ae72:022f: the sentence names the object under the mouse,
				// except while the player walks
				if (!_scene->isWalking()) {
					int object = _interface->inventoryObjectAt(e.mouse);
					if (!object)
						object = _scene->objectAt(e.mouse);
					_interface->setObject(object ? object : (int)kNoObject);
				}
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				int verb = _interface->verbAt(e.mouse);
				int scroll = _interface->scrollButtonAt(e.mouse);
				int carried = _interface->inventoryObjectAt(e.mouse);
				int object = _scene->objectAt(e.mouse);
				if (verb) {
					_interface->setVerb(verb);
				} else if (scroll) {
					// 14a3:048d / 04ab: one row up or down
					int last = MAX<int>(1, ((int)_inventory.size() - 1) / 3);
					if (scroll == 0x3e && _inventoryPage > 1)
						--_inventoryPage;
					else if (scroll == 0x3f && _inventoryPage < last)
						++_inventoryPage;
					_interface->drawInventory();
				} else if (carried || object) {
					int clicked = carried ? carried : object;
					int verbNow = _interface->getVerb();
					if (_messageTimer)
						clearMessage(); // b000:060a: a new walk ends the message
					if (!_interface->isAwaitingSecond() && (verbNow == kVerbUse || verbNow == kVerbGive) &&
						takesSecondObject(clicked)) {
						// ae72:02e6: Benyt/Giv with an object whose VMT 0x20
						// (+0x2b flag) is true waits for the second object
						_interface->beginSecondObject(clicked);
						continue;
					}
					int first = _interface->isAwaitingSecond() ? _interface->getFirstObject() : clicked;
					int second = _interface->isAwaitingSecond() ? clicked : (int)kNoObject;
					pendingVerb = verbNow;
					pendingSecond = second;
					pendingFirst = first;
					// ae72:0e21: walk to the first object's node; a carried
					// first object has none, so to the second's
					_interface->setObject(clicked);
					int target = !isCarried(first) ? first : second;
					if (carried || target == (int)kNoObject || isCarried(target)) {
						executeVerb(pendingVerb, first, second);
						_interface->cancelSecondObject();
					} else {
						_scene->walkToObject(e.mouse, target);
					}
				}
			} else if (e.type == Common::EVENT_KEYDOWN && (e.kbd.keycode == Common::KEYCODE_F1 ||
					e.kbd.keycode == Common::KEYCODE_F5 || e.kbd.keycode == Common::KEYCODE_F6 ||
					e.kbd.keycode == Common::KEYCODE_F10)) {
				// ae72: the global screens (rooms 1..4, b06f:0fa4); back in
				// the room before with node 0, or the menu's choice
				static const struct { Common::KeyCode code; int key; } kKeys[] = {
					{ Common::KEYCODE_F1, kKeyF1 }, { Common::KEYCODE_F5, kKeyF5 },
					{ Common::KEYCODE_F6, kKeyF6 }, { Common::KEYCODE_F10, kKeyF10 }
				};
				_scene->stopWalking();
				for (uint i = 0; i < ARRAYSIZE(kKeys); ++i) {
					if (kKeys[i].code == e.kbd.keycode)
						runFunctionKey(kKeys[i].key, false);
				}
				if (_menuAction)
					_phaseDone = true;
				else
					redrawRoom();
			} else if (e.type == Common::EVENT_KEYDOWN && e.kbd.keycode == Common::KEYCODE_F2) {
				// ae72:053e: stop walking, switch the sound, say so
				_scene->stopWalking();
				_sound.setOn(!_sound.isOn());
				say(readPascalString(_ovr, _sound.isOn() ? kTextSoundOn : kTextSoundOff), 14, true);
			} else if (e.type == Common::EVENT_KEYDOWN) {
				int verb = _interface->verbForKey(unicodeToCp865(e.kbd.ascii));
				if (verb)
					_interface->setVerb(verb);
			}
		}
		if (_loadedGame)
			break; // a save was loaded from the main menu: run() builds its room

		_scene->update();
		tickGarden();   // 8c31:0bd1, before the room's own idle tick (b000:0263)
		tickBathroom(); // 90f3:0f14
		tickKitchen();  // 91f4:1265
		tickSchool();   // the bat swing in classroom 1
		tickFarm();     // room 9's bagging (7554:001c)
		tickChickenFarm(); // hosing the dung room (69d2:0011)
		tickFactory();     // the line's walk-end messages (2381:0065)
		tickShop();     // the assistant's walks (message 0x21)
		if (_messageTimer && --_messageTimer == 0)
			clearMessage();
		if (_award.active && !_scene->isWalking())
			showPendingAward();
		int object;
		if (_scene->takeArrival(object)) {
			executeVerb(pendingVerb, pendingFirst ? pendingFirst : object, pendingSecond);
			pendingFirst = 0;
			pendingSecond = kNoObject;
			_interface->cancelSecondObject(); // ae72:09df
		}
		_scene->draw(_screen);

		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
}

Common::Error MiljostrupEngine::run() {
	initGraphics(320, 200);
	_screen = new Graphics::Screen();
	setDebugger(new Console());

	uint32 t0 = g_system->getMillis();
	if (!loadPalettes())
		return Common::Error(Common::kReadingFailed, "MLJSTRUP.EXE");
	Common::File exe;
	if (!exe.open("MLJSTRUP.EXE") || !_font.loadPatch(exe))
		return Common::Error(Common::kReadingFailed, "MLJSTRUP.EXE");
	if (!loadStrings())
		return Common::Error(Common::kReadingFailed, "MLJSTRUP.OVR");
	if (!_archive.open("MLJSTRUP.ARK"))
		return Common::Error(Common::kReadingFailed, "MLJSTRUP.ARK");
	debug(1, "Start-up: files open after %u ms", g_system->getMillis() - t0);
	_sound.init();
	debug(1, "Start-up: sound after %u ms", g_system->getMillis() - t0);
	_interface = new Interface(_screen, &_font);
	_interface->setInventory(&_inventory, &_inventoryPage);
	if (!_ovr.open("MLJSTRUP.OVR") || !_exe.open("MLJSTRUP.EXE") || !_interface->load(exe, _ovr))
		return Common::Error(Common::kReadingFailed, "MLJSTRUP.EXE");

	// As the main program: title and first credits page, licence, login.
	// The cursor is shown on the title pages and hidden for licence and
	// login (28c3:10f4 / 28c3:1110)
	_interface->setCursor(Interface::kCursorArrow);
	_interface->showCursor(true);
	// Debugging: "skip_intro=true" in the game's section of scummvm.ini
	// starts in the first room (no title, licence, login or town map)
	bool skipIntro = ConfMan.hasKey("skip_intro") && ConfMan.getBool("skip_intro");
	int launcherSlot = ConfMan.hasKey("save_slot") ? ConfMan.getInt("save_slot") : -1;
	if (launcherSlot >= 0)
		skipIntro = true;
	if (!skipIntro) {
		if (!runTitlePage(1) || !runTitlePage(2))
			return Common::kNoError;
		_interface->showCursor(false);
		if (!runLicence())
			return Common::kNoError;
		// The game option "original_login" (off by default, as ScummVM's
		// games skip such screens): the login and the user's status file;
		// without it no user, the progress is kept in ScummVM's save games
		if (ConfMan.hasKey("original_login") && ConfMan.getBool("original_login")) {
			if (!runLogin())
				return Common::kNoError;
			debug(1, "Logged in as user %d", _user);
			// Fasekontrol.Start (b288:0068): the user's status file
			if (!startStatus())
				return Common::kNoError;
		}
		_interface->showCursor(true);
	}

	_scene = new Scene(&_archive, &_doors);
	// Debugging: "fast_walk=true" in the game's section of scummvm.ini makes
	// the player jump to where he walks (for testing; not in the original)
	_scene->setFastWalk(ConfMan.hasKey("fast_walk") && ConfMan.getBool("fast_walk"));
	debug(1, "Start-up: interface after %u ms", g_system->getMillis() - t0);

	// Fasekontrol's loop (VMT 0x0c = 5000:0000): phase 1..7 is the town map
	// (5009:02a7) and then that location, until PhaseCompleted (b06f:107e)
	// has moved the phase on and saved the status file.
	// Debugging: "start_location=N" in the game's section of scummvm.ini
	// starts in phase / location N (1 home, 2 school, ... 7, 8 the end of the
	// game; testing aid like the original's command line, 1000:0206, but it
	// also overrides the phase of the status file)
	if (ConfMan.hasKey("start_location")) {
		int start = ConfMan.getInt("start_location");
		if (start >= 1 && start <= 8)
			_phase = _startPhase = start;
	}
	if (launcherSlot >= 0) {
		loadUsers();
		if (loadGameState(launcherSlot).getCode() != Common::kNoError)
			_loadedGame = false;
	}
	for (bool first = true; !shouldQuit(); first = false) {
		const LocationInfo *location = findLocation(_phase);
		if (!location) {
			// any other phase: 5038:0069, the end of the game; it ends only
			// through the F10 menu
			_menuAction = 0;
			runEndOfGame();
			if (!_menuAction)
				break;
		}
		if (location && _loadedGame) {
			// a save game: its room, with the location's state as saved
			enterLoadedRoom();
		} else if (location) {
			if (!(first && skipIntro) && !runMapIntro(location->number))
				return Common::kNoError;
			enterLocation(location->number);
			if (!gotoRoom(location->startRoom, location->startNode))
				return Common::Error(Common::kUnknownError, "Start room");
			// Debugging: "all_items=true" in the game's section of scummvm.ini puts
			// every carriable object of the location into the inventory (testing
			// aid, not in the original; the debugger's "items" does the same)
			if (ConfMan.hasKey("all_items") && ConfMan.getBool("all_items"))
				giveAllItems();
		}
		if (location) {
			debug(1, "Location %d after %u ms", location->number, g_system->getMillis() - t0);
			_phaseDone = false;
			_menuAction = 0;
			runRoomLoop();
			if (_loadedGame)
				continue;   // a save was loaded in the room: build its room
			if (!_phaseDone)
				break;
		}
		// The F10 menu (b06f:0161): replay = the status file is read again
		// (b288:01d5), restart = the start phase and the totals 0, saved
		// (b288:01e5), stop = the game ends (title page 3 at the exit,
		// accd:037d); the file stays as it was saved at the last phase end
		if (_menuAction == kMenuStop) {
			runTitlePage(3);
			break;
		} else if (_menuAction == kMenuRestartGame) {
			resetStatus();
		} else if (_menuAction == kMenuReplay && _user >= 0) {
			if (!loadStatus())
				break;
		}
		_menuAction = 0;
	}

	return Common::kNoError;
}

} // End of namespace Miljostrup
