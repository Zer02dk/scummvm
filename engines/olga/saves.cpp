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

// Saved positions (notes/savegame.md) and the save / load screen
// (locations 25 / 26, overlay unit 50dc). A ScummVM save holds the
// original's file OLGA_GEM.0NN: the name, then the 732-byte record. The
// screen lists ScummVM's slots 1..16; the drive buttons only show the
// program's drive.

#include "common/savefile.h"
#include "common/system.h"
#include "engines/metaengine.h"
#include "graphics/paletteman.h"
#include "graphics/scaler.h"

#include "olga/olga.h"

namespace Olga {

namespace {

const uint16 kSegSaveLoad = 0x50dc;

enum {
	kNumSlots = 16,
	kNameLength = 24,      // string[24]
	kMaxTyped = 23,        // SaveLoad_0a98: appends while the length < 23
	kRecordSize = 0x2dc,
	kCmdYes = 0xe,
	kCmdNo = 0xf
};

const Common::Point kContinuePos(0xe6, 0xab); // Fortsæt, command 0xc
const Common::Point kDrivePos(0x1d, 0x13);    // the first drive button

// The Danish letters a name can have (SaveLoad_0bbd), CP865 and Unicode
const struct {
	byte cp865;
	uint32 unicode;
} kDanish[6] = {
	{ 0x91, 0xe6 }, { 0x9b, 0xf8 }, { 0x86, 0xe5 }, { 0x92, 0xc6 }, { 0x9d, 0xd8 }, { 0x8f, 0xc5 }
};

Common::U32String fromCp865(const Common::String &s) {
	Common::U32String u;
	for (uint i = 0; i < s.size(); i++) {
		uint32 c = (byte)s[i];
		for (int k = 0; k < 6; k++)
			if (kDanish[k].cp865 == c)
				c = kDanish[k].unicode;
		u += (Common::u32char_type_t)c;
	}
	return u;
}

Common::String toCp865(const Common::U32String &u) {
	Common::String s;
	for (uint i = 0; i < u.size(); i++) {
		uint32 c = u[i];
		for (int k = 0; k < 6; k++)
			if (kDanish[k].unicode == c)
				c = kDanish[k].cp865;
		s += c < 0x100 ? (char)c : '?';
	}
	return s;
}

int slotY(int n) {
	return 0x26 + (n - 1) * 9;
}

Common::Rect slotRect(int n) {
	// SaveLoad_0d0a: the slot's control, (30, y)-(221, y + 7)
	return Common::Rect(0x1e, slotY(n), 0xde, slotY(n) + 8);
}

Common::Rect imageButton(const LbmImage &img, const Common::Point &p) {
	return Common::Rect(p.x - 2, p.y - 2, p.x + img.surface.w + 2, p.y + img.surface.h + 2);
}

/** SaveLoad_0bbd: the characters a name may have. */
bool nameChar(byte c) {
	if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' ')
		return true;
	static const char kOthers[] = "!,.:;?/()$";
	for (const char *p = kOthers; *p; p++)
		if (c == (byte)*p)
			return true;
	for (int k = 0; k < 6; k++)
		if (c == kDanish[k].cp865)
			return true;
	return false;
}

} // End of anonymous namespace

// The record (SaveLoad_1743 / 195a)

void OlgaEngine::writeRecord(Common::WriteStream &out, const Common::String &name, int location,
		int x, int y) {
	byte nameBuf[1 + kNameLength];
	memset(nameBuf, 0, sizeof(nameBuf));
	nameBuf[0] = MIN<uint>(name.size(), kNameLength);
	memcpy(nameBuf + 1, name.c_str(), nameBuf[0]);
	out.write(nameBuf, sizeof(nameBuf));
	out.writeSint16LE(location);
	out.writeSint16LE(x);
	out.writeSint16LE(y);
	for (int id = kFirstId; id <= kLastId; id++)
		out.writeUint32LE(_flags[id]);
	out.writeUint16LE(_inventory.size());
	for (int i = 0; i < 100; i++)
		out.writeUint16LE(i < (int)_inventory.size() ? _inventory[i] : 0);
}

bool OlgaEngine::readRecord(Common::SeekableReadStream &in) {
	// SaveLoad_1a49: the name, then exactly 0x2dc bytes ("Forkert filformat")
	in.skip(1 + kNameLength);
	if (in.size() - in.pos() < kRecordSize)
		return false;
	int location = in.readSint16LE();
	int x = in.readSint16LE();
	int y = in.readSint16LE();
	if (location < 1 || location > kNumLocations)
		return false;
	for (int id = kFirstId; id <= kLastId; id++)
		_flags[id] = in.readUint32LE();
	int count = MIN<int>(in.readUint16LE(), 100);
	_inventory.clear();
	for (int i = 0; i < 100; i++) {
		uint16 id = in.readUint16LE();
		if (i < count)
			_inventory.push_back(id);
	}
	if (in.err())
		return false;
	// 1000:075b: the saved room, entry 1 = Olga's saved place
	for (int i = 0; i <= kLastId; i++)
		_names[i].clear();
	_inventoryRow = 1;
	_pendingDevil = _pendingPraise = false;
	_restoreLocation = location;
	_restoreEntry = { (int16)x, (int16)y };
	setExit(location, 1);
	return true;
}

// ScummVM's save interface

bool OlgaEngine::canSaveGameStateCurrently(Common::U32String *msg) {
	// In a room waiting for input, Olga standing, nothing about to change
	return _kind == kLocRoom && _location > 0 && _inputEnabled && !_pending && !_nextLocation &&
		_scene.olga() && !_scene.olga()->moving;
}

bool OlgaEngine::canLoadGameStateCurrently(Common::U32String *msg) {
	return _location == 0 || (_kind == kLocRoom && _inputEnabled && !_nextLocation);
}

Common::Error OlgaEngine::saveGameState(int slot, const Common::String &desc, bool isAutosave) {
	_saveName = toCp865(Common::U32String(desc));
	return Engine::saveGameState(slot, desc, isAutosave);
}

Common::Error OlgaEngine::saveGameStream(Common::WriteStream *stream, bool isAutosave) {
	// From the save screen: the room it came from and Olga's place there
	// (its entry 1); from ScummVM's menu: the current room
	int location = _returnLocation, x = 0, y = 0;
	if (_kind == kLocRoom) {
		location = _location;
		SceneObject *olga = _scene.olga();
		if (olga && olga->anim >= 1) {
			const ScnAnim &a = olga->def.anims[olga->anim - 1];
			x = olga->x - a.dx;
			y = olga->y - a.dy;
		}
	} else if (location > 0) {
		x = _entries[location - 1][0].x;
		y = _entries[location - 1][0].y;
	}
	writeRecord(*stream, _saveName, location, x, y);
	return Common::kNoError;
}

Common::Error OlgaEngine::loadGameStream(Common::SeekableReadStream *stream) {
	if (!readRecord(*stream))
		return Common::kReadingFailed;
	return Common::kNoError;
}

bool OlgaEngine::getRoomThumbnail(Graphics::Surface &thumb) {
	// Saved from the save screen: the picture of the room behind it
	if (_kind != kLocSaveLoad || _roomPicture.w != 320)
		return false;
	return createThumbnail(&thumb, (const uint8 *)_roomPicture.getPixels(), _roomPicture.w, _roomPicture.h,
		_roomPalette);
}

// The save / load screen

bool OlgaEngine::gotoSaveLoad(int number) {
	// SaveLoad_05a2: save (25) or load (26); then 0f7c draws the screen,
	// 0ef2 the buttons and slot controls, 1267 reads the slots, 13b4 shows
	// them
	_location = number;
	_kind = kLocSaveLoad;
	_nextLocation = 0;
	_pending = false;
	_textTimer = 0;
	_slSave = number == 25;
	_slEditing = false;
	_slSlot = 0;
	_slDialog = 0;

	SaveStateList saves = getMetaEngine()->listSaves(_targetName.c_str());
	for (int n = 1; n <= kNumSlots; n++) {
		_slots[n].used = false;
		_slots[n].name.clear();
	}
	for (uint i = 0; i < saves.size(); i++) {
		int n = saves[i].getSaveSlot();
		if (n >= 1 && n <= kNumSlots) {
			_slots[n].used = true;
			_slots[n].name = toCp865(saves[i].getDescription());
			if (_slots[n].name.size() > kNameLength)
				_slots[n].name = Common::String(_slots[n].name.c_str(), kNameLength);
		}
	}

	// The room picture for the save's thumbnail (not in the original)
	_roomPicture.copyFrom(*_screen);
	g_system->getPaletteManager()->grabPalette(_roomPalette, 0, 256);

	_screen->clear(0);
	// SaveLoad_1049: the title, colour 3
	_font.drawCentred(*_screen, _exe.readString(kSegSaveLoad, _slSave ? 0x0f30 : 0x0f55), 1, 3);
	_screen->fillRect(Common::Rect(0x14, 0xd, 0x12c, 0xbf), 7);
	frame3D(0x15, 0xe, 0x115, 0xaf);
	for (int i = 0; i < kNumSlots; i++)
		_screen->frameRect(Common::Rect(0x1d, 0x25 + 9 * i, 0xdf, 0x25 + 9 * (i + 1) + 1), 8);
	drawButton(_quitImages[2], kContinuePos.x, kContinuePos.y, false);
	// SaveLoad_0e27 / 0d64: one button per drive, the selected one lit
	// (19c0:0bed), its letter at x + 29, y 24. Only the program's drive here.
	_screen->blitFrom(_diskImages[1].surface, kDrivePos);
	_font.drawString(*_screen, "C", kDrivePos.x + 0x1d, 0x18, 0);
	drawSlots();
	setInput(true);
	return true;
}

void OlgaEngine::frame3D(int x, int y, int w, int h) {
	// Frame3D_Draw (1212:0000)
	_screen->frameRect(Common::Rect(x, y, x + w, y + h), 8);
	_screen->frameRect(Common::Rect(x + 1, y + 1, x + w + 1, y + h + 1), 15);
	_screen->setPixel(x + 1, y + h - 1, 7);
	_screen->setPixel(x + w - 1, y + 1, 7);
}

void OlgaEngine::drawSlot(int n, bool selected, const Common::String &text) {
	// SaveLoad_12e3: grey (7) or selected (3); an empty slot's blocks in 8,
	// a name in 5
	_screen->fillRect(Common::Rect(0x1e, slotY(n), 0xde, slotY(n) + 8), selected ? 3 : 7);
	if (!text.empty())
		_font.drawString(*_screen, text, 0x1e, slotY(n), (byte)text[0] == 0xb1 ? 8 : 5);
}

void OlgaEngine::drawSlots() {
	// SaveLoad_13b4: an empty slot shows 24 x 0xB1 (50dc:13b2)
	Common::String empty;
	for (int i = 0; i < kNameLength; i++)
		empty += (char)0xb1;
	for (int n = 1; n <= kNumSlots; n++)
		drawSlot(n, false, _slots[n].used ? _slots[n].name : empty);
}

void OlgaEngine::drawEditSlot() {
	// SaveLoad_0690: the name being typed and a block cursor after it
	drawSlot(_slSlot, true, _slText);
	int x = _slText.size() * 8 + 0x1e;
	_screen->fillRect(Common::Rect(x, slotY(_slSlot), x + 8, slotY(_slSlot) + 8), 5);
}

void OlgaEngine::drawDialog(const Common::String &title, const Common::String &text, bool question) {
	// SaveLoad_03de / 0260 (Ja / Nej) and 013d / 0000 (OK): a box as wide
	// as the longer text
	int w = MAX(title.size(), text.size()) * 8 + 0x16;
	int x = (320 - w) / 2;
	_dialogX = x;
	_dialogW = w;
	_screen->frameRect(Common::Rect(x - 1, 0x27, x + w + 2, 0x6a), 0);
	_screen->fillRect(Common::Rect(x, 0x28, x + w + 1, 0x69), 7);
	frame3D(x + 1, 0x29, w - 2, 0x3e);
	_screen->hLine(x + 2, 0x3d, x + w - 2, 8);
	_screen->hLine(x + 2, 0x3e, x + w - 2, 15);
	_font.drawCentred(*_screen, title, 0x30, 1);
	_font.drawCentred(*_screen, text, 0x46, 5);
	if (question) {
		drawButton(_yesImage, x + 0xd, 0x55, false);
		drawButton(_noImage, x + w - 0x4a, 0x55, false);
	} else {
		drawButton(_okImage, 0x87, 0x55, false);
	}
}

void OlgaEngine::saveLoadDone() {
	// 1000:075b: back to the room (or to the loaded one, set by readRecord)
	if (!_nextLocation)
		setExit(_returnLocation, 1);
}

void OlgaEngine::saveLoadError(uint16 off) {
	// SaveLoad_1593: "Gem/hent fejl:" and the message with "."
	_slDialog = 2;
	drawDialog(_exe.readString(kSegSaveLoad, 0x012e), _exe.readString(kSegSaveLoad, off) + ".", false);
}

void OlgaEngine::saveSlot() {
	// SaveLoad_182b: write the file (the overwrite question comes first)
	if (saveGameState(_slSlot, fromCp865(_slText).encode()).getCode() != Common::kNoError) {
		saveLoadError(0x180b);
		return;
	}
	saveLoadDone();
}

void OlgaEngine::saveLoadClick(const Common::Point &p) {
	if (_slDialog == 1) {
		// SaveLoad_050f: Ja (0xe) or Nej (0xf); either way the screen ends
		if (imageButton(_yesImage, Common::Point(_dialogX + 0xd, 0x55)).contains(p)) {
			_slDialog = 0;
			saveSlot();
		} else if (imageButton(_noImage, Common::Point(_dialogX + _dialogW - 0x4a, 0x55)).contains(p)) {
			_slDialog = 0;
			saveLoadDone();
		}
		return;
	}
	if (_slDialog == 2) {
		if (imageButton(_okImage, Common::Point(0x87, 0x55)).contains(p)) {
			_slDialog = 0;
			saveLoadDone();
		}
		return;
	}
	if (imageButton(_quitImages[2], kContinuePos).contains(p)) {
		saveLoadDone(); // SaveLoad_09dc: 0xc
		return;
	}
	for (int n = 1; n <= kNumSlots; n++) {
		if (!slotRect(n).contains(p))
			continue;
		if (!_slSave) {
			// SaveLoad_08e8: a used slot loads at once
			if (!_slots[n].used)
				return;
			drawSlot(n, true, _slots[n].name);
			if (loadGameState(n).getCode() != Common::kNoError)
				saveLoadError(0x1a37);
			else
				saveLoadDone();
		} else {
			// SaveLoad_07c6: start typing in that slot, from its old name
			if (_slEditing)
				drawSlots();
			_slEditing = true;
			_slSlot = n;
			_slText = _slots[n].used ? _slots[n].name : Common::String();
			drawEditSlot();
		}
		return;
	}
}

void OlgaEngine::saveLoadKey(const Common::KeyState &ks) {
	if (ks.keycode == Common::KEYCODE_ESCAPE && _slDialog == 0) {
		saveLoadDone(); // not in the original: Esc is Fortsæt
		return;
	}
	if (!_slSave || !_slEditing || _slDialog)
		return;
	// SaveLoad_0bbd: a name character, Backspace or Enter
	if (ks.keycode == Common::KEYCODE_BACKSPACE) {
		if (!_slText.empty()) {
			_slText.deleteLastChar();
			drawEditSlot();
		}
		return;
	}
	if (ks.keycode == Common::KEYCODE_RETURN || ks.keycode == Common::KEYCODE_KP_ENTER) {
		// SaveLoad_162e: a used slot asks first
		if (_slots[_slSlot].used) {
			_slDialog = 1;
			drawDialog(_exe.readString(kSegSaveLoad, 0x15ec), _exe.readString(kSegSaveLoad, 0x160a) + "?", true);
		} else {
			saveSlot();
		}
		return;
	}
	byte c = 0;
	if (ks.ascii > 0 && ks.ascii < 0x80) {
		c = ks.ascii;
	} else {
		for (int k = 0; k < 6; k++)
			if (ks.ascii == kDanish[k].unicode)
				c = kDanish[k].cp865;
	}
	if (c && nameChar(c) && _slText.size() < kMaxTyped) {
		_slText += (char)c;
		drawEditSlot();
	}
}

} // End of namespace Olga
