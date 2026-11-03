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

#ifndef MILJOSTRUP_INTERFACE_H
#define MILJOSTRUP_INTERFACE_H

#include "common/array.h"
#include "common/events.h"
#include "common/file.h"
#include "common/hashmap.h"
#include "common/serializer.h"
#include "common/rect.h"
#include "common/str.h"
#include "graphics/managed_surface.h"

#include "miljostrup/font.h"

namespace Miljostrup {

enum Verb {
	kVerbPush = 0x32,  // Skub
	kVerbPull,         // Træk
	kVerbUse,          // Benyt
	kVerbTake,         // Tag
	kVerbGive,         // Giv
	kVerbLook,         // Se på
	kVerbOpen,         // Åbn
	kVerbClose,        // Luk
	kVerbTalk,         // Tal til
	kVerbTurnOn,       // Tænd
	kVerbTurnOff,      // Sluk
	kVerbHelp,         // Hjælp
	kVerbFirst = kVerbPush,
	kVerbLast = kVerbHelp
};

enum {
	kNoObject = 100 // "nothing" in the sentence line
};

struct VerbButton {
	byte verb;
	byte hotkey;    // code page 865
	uint32 image;   // EXE file offset of the image object
};

struct ObjectName {
	byte location;
	uint16 id;
	uint32 offset;  // Pascal string in MLJSTRUP.OVR
};

/** A button of the original's widget kit (28c3:02dd): image with a bevel. */
/**
 * 28c3:0813: a button image with its bevel and black frame. `frame` is the
 * frame width (+0x1b), `bevel` the bevel width and the pressed offset (+0x1d);
 * the widget rectangle is the image plus frame + bevel on each side.
 * `light` and `corner` are widget +0x1e and +0x20 (usually 15 and 7; close-up
 * 26 has light 16, close-up 30's START and FORTRYD corner 18).
 */
void drawBevelButton(Graphics::ManagedSurface *dst, const Graphics::ManagedSurface &image, int x, int y, bool pressed,
	int frame = 1, int bevel = 1, int light = 15, int corner = 7);
/** The close-ups' OK keys: Enter, keypad Enter, Space. */
bool isOkKey(const Common::Event &e);
/** An image button's rectangle: the image plus `border` (frame + bevel) on each side. */
Common::Rect buttonRect(const Graphics::ManagedSurface &image, int x, int y, int border = 2);
/** An image object stored in the EXE (verb buttons, icons, buttons). */
Graphics::ManagedSurface *loadImageObject(Common::File &exe, uint32 offset);

/**
 * The interface bar of the room screens (unit ae72): verb buttons, the
 * sentence line above the bar, and the mouse cursors.
 */
class Interface {
public:
	Interface(Graphics::ManagedSurface *screen, const GameFont *font);
	~Interface();

	bool load(Common::File &exe, Common::File &ovr);
	void setLocation(int location);

	enum CursorShape { kCursorArrow, kCursorWait };
	void setCursor(CursorShape shape);
	void showCursor(bool show);

	int getVerb() const { return _verb; }
	void setVerb(int verb);
	/** The verb button at a screen position, or 0. */
	int verbAt(const Common::Point &pos) const;
	/** The verb with this hotkey (code page 865, either case), or 0. */
	int verbForKey(int key) const;

	/** The object the sentence names (kNoObject for none). */
	void setObject(int object);
	/** ae72:02e6: Benyt/Giv with a first object waits for a second one. */
	void beginSecondObject(int first);
	bool isAwaitingSecond() const { return _first != 0; }
	int getFirstObject() const { return _first; }
	void cancelSecondObject();
	Common::String getObjectName(int object) const;
	/** A name that differs from the start name after a state change. */
	void setNameOverride(int object, const Common::String &name);
	/** The name overrides into / out of a save game (saves.cpp). */
	void syncNameOverrides(Common::Serializer &s);
	void clearNameOverrides();
	void removeNameOverride(int object);

	void draw();

	/** ae72:0da9: the message line at the top (y 0..7); empty text clears it. */
	void showMessage(const Common::String &text, byte color);

	/** The inventory list (DS:d100) and its first shown row (DS:ccc0). */
	void setInventory(const Common::Array<uint16> *items, int *page) { _inventory = items; _page = page; }
	/** 14a3:02fd: scroll buttons and the six slots. */
	void drawInventory();
	/** The object in the inventory slot at a screen position, or 0. */
	int inventoryObjectAt(const Common::Point &pos) const;
	/** The scroll button at a screen position: 0x3e up, 0x3f down, or 0. */
	int scrollButtonAt(const Common::Point &pos) const;

private:
	void drawButton(int index);
	void drawSentence();

	Graphics::ManagedSurface *_screen;
	const GameFont *_font;
	Common::Array<Graphics::ManagedSurface *> _verbImages;
	Common::String _verbNames[kVerbLast - kVerbFirst + 1];
	byte _cursors[2][16 * 16];
	Common::File *_ovr = nullptr;
	Common::HashMap<uint16, Common::String> _objectNames;
	Common::HashMap<uint16, Common::String> _nameOverrides;
	int _location = 0;

	int _verb;
	int _object = kNoObject;
	int _first = 0;  // first object while waiting for the second (+0x718)

	Graphics::ManagedSurface *getIcon(int number);
	Common::File *_exe = nullptr;
	Common::HashMap<int, Graphics::ManagedSurface *> _icons;
	Graphics::ManagedSurface *_scrollImages[2] = { nullptr, nullptr };
	const Common::Array<uint16> *_inventory = nullptr;
	int *_page = nullptr;
};

} // End of namespace Miljostrup

#endif // MILJOSTRUP_INTERFACE_H
