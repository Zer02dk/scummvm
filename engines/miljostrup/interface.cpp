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

// The interface bar (notes/interface.md): verb buttons (ae72:0178,
// 28c3:02dd), sentence line (ae72:0a1c) and mouse cursors (28c3:1050).

#include "graphics/cursorman.h"

#include "miljostrup/interface.h"
#include "miljostrup/verbs.h"
#include "miljostrup/interface_data.h"

namespace Miljostrup {

enum {
	kCursorKey = 255,      // not used by the cursor shapes (black 0, white 15)
	kSentenceColor = 3,
	kSentenceY = 147,
	kMaxSentence = 40,
	kVerbNameSize = 11     // String[10]
};

Interface::Interface(Graphics::ManagedSurface *screen, const GameFont *font) :
	_screen(screen), _font(font), _verb(kStartVerb) {
	memset(_cursors, kCursorKey, sizeof(_cursors));
}

Interface::~Interface() {
	for (uint i = 0; i < _verbImages.size(); ++i)
		delete _verbImages[i];
	for (auto &i : _icons)
		delete i._value;
	delete _scrollImages[0];
	delete _scrollImages[1];
}

bool Interface::load(Common::File &exe, Common::File &ovr) {
	// Cursors: AND mask 0 -> black, then the cursor mask turns black into
	// white; AND 1 with cursor 0 leaves the screen (neither shape inverts)
	const uint32 offsets[2] = { kCursorArrowOffset, kCursorWaitOffset };
	for (int c = 0; c < 2; ++c) {
		exe.seek(offsets[c]);
		uint16 masks[32];
		for (int i = 0; i < 32; ++i)
			masks[i] = exe.readUint16LE();
		for (int y = 0; y < 16; ++y) {
			for (int x = 0; x < 16; ++x) {
				bool screenBit = masks[y] & (0x8000 >> x);
				bool cursorBit = masks[16 + y] & (0x8000 >> x);
				byte pixel = kCursorKey;
				if (!screenBit)
					pixel = cursorBit ? 15 : 0;
				_cursors[c][y * 16 + x] = pixel;
			}
		}
	}

	for (int v = kVerbFirst; v <= kVerbLast; ++v) {
		exe.seek(kVerbNameOffset + (v - kVerbFirst) * kVerbNameSize);
		byte len = MIN<byte>(exe.readByte(), kVerbNameSize - 1);
		char buf[kVerbNameSize];
		exe.read(buf, len);
		_verbNames[v - kVerbFirst] = Common::String(buf, len);
	}

	// Button images: u16 width, u16 height, u8 flags, u8 key colour, pixels
	for (uint i = 0; i < ARRAYSIZE(kVerbButtons); ++i)
		_verbImages.push_back(loadImageObject(exe, kVerbButtons[i].image));

	_scrollImages[0] = loadImageObject(exe, kScrollUpImage);
	_scrollImages[1] = loadImageObject(exe, kScrollDownImage);

	_ovr = &ovr;
	_exe = &exe;
	return !exe.err();
}

void Interface::setLocation(int location) {
	// Object IDs are only unique within a location
	_location = location;
	_objectNames.clear();
	for (uint i = 0; i < ARRAYSIZE(kObjectNames); ++i) {
		if (kObjectNames[i].location != location)
			continue;
		_ovr->seek(kObjectNames[i].offset);
		byte len = _ovr->readByte();
		char buf[256];
		_ovr->read(buf, len);
		_objectNames[kObjectNames[i].id] = Common::String(buf, len);
	}
}

Common::String Interface::getObjectName(int object) const {
	if (_nameOverrides.contains(object))
		return _nameOverrides[object];
	return _objectNames.contains(object) ? _objectNames[object] : Common::String();
}

void Interface::syncNameOverrides(Common::Serializer &s) {
	uint32 count = _nameOverrides.size();
	s.syncAsUint32LE(count);
	if (s.isSaving()) {
		for (Common::HashMap<uint16, Common::String>::const_iterator i = _nameOverrides.begin();
				i != _nameOverrides.end(); ++i) {
			uint16 object = i->_key;
			Common::String name = i->_value;
			s.syncAsUint16LE(object);
			s.syncString(name);
		}
	} else {
		_nameOverrides.clear();
		for (uint32 k = 0; k < count; ++k) {
			uint16 object = 0;
			Common::String name;
			s.syncAsUint16LE(object);
			s.syncString(name);
			_nameOverrides[object] = name;
		}
	}
}

void Interface::setNameOverride(int object, const Common::String &name) {
	_nameOverrides[object] = name;
	if (object == _object)
		drawSentence();
}

void Interface::removeNameOverride(int object) {
	_nameOverrides.erase(object);
	if (object == _object)
		drawSentence();
}

void Interface::clearNameOverrides() {
	_nameOverrides.clear();
}

void Interface::setCursor(CursorShape shape) {
	CursorMan.replaceCursor(_cursors[shape], 16, 16, 0, 0, kCursorKey);
}

void Interface::showCursor(bool show) {
	CursorMan.showMouse(show);
}

void Interface::setVerb(int verb) {
	if (verb < kVerbFirst || verb > kVerbLast)
		return;
	_verb = verb;
	_object = kNoObject;
	_first = 0; // ae72:09df: back to state 1
	draw();
}

void Interface::beginSecondObject(int first) {
	_first = first;
	_object = kNoObject;
	drawSentence();
}

void Interface::cancelSecondObject() {
	_first = 0;
	_object = kNoObject;
	drawSentence();
}

int Interface::verbAt(const Common::Point &pos) const {
	for (uint i = 0; i < ARRAYSIZE(kVerbButtons); ++i) {
		int x = (i % 4) * 56 + 3, y = (i / 4) * 14 + 160;
		if (pos.x >= x - 2 && pos.x <= x + 52 && pos.y >= y - 2 && pos.y <= y + 10)
			return kVerbButtons[i].verb;
	}
	return 0;
}

int Interface::verbForKey(int key) const {
	if (key >= 'a' && key <= 'z')
		key -= 'a' - 'A';
	else if (key == 0x86)      // å
		key = 0x8f;
	else if (key == 0x91)      // æ
		key = 0x92;
	for (uint i = 0; i < ARRAYSIZE(kVerbButtons); ++i) {
		if (kVerbButtons[i].hotkey == key)
			return kVerbButtons[i].verb;
	}
	return 0;
}

void Interface::setObject(int object) {
	if (object == _object)
		return;
	_object = object;
	drawSentence();
}

void drawBevelButton(Graphics::ManagedSurface *dst, const Graphics::ManagedSurface &image, int x, int y, bool pressed,
		int frame, int bevel, int light, int corner) {
	// 28c3:0813: image, bevel (raised 28c3:04b4, pressed 28c3:0625), black
	// frame (28c3:043c); bevel ring i runs from the frame inwards
	int b = frame + bevel;
	int x1 = x - b, y1 = y - b, x2 = x + image.w + b - 1, y2 = y + image.h + b - 1;
	if (pressed) {
		dst->transBlitFrom(image, Common::Point(x + bevel, y + bevel), image.getTransparentColor());
		for (int i = frame; i < b; ++i) {
			dst->hLine(x1 + i, y1 + i, x2 - frame, 8);
			dst->vLine(x1 + i, y1 + i + 1, y2 - frame, 8);
			dst->hLine(x1 + i + bevel, y1 + i + bevel, x2 - frame, corner);
			dst->vLine(x1 + i + bevel, y1 + i + bevel + 1, y2 - frame, corner);
		}
	} else {
		dst->transBlitFrom(image, Common::Point(x, y), image.getTransparentColor());
		for (int i = frame; i < b; ++i) {
			dst->hLine(x1 + i, y1 + i, x2 - i - 1, light);
			dst->vLine(x1 + i, y1 + i, y2 - i - 1, light);
			dst->hLine(x1 + i + 1, y2 - i, x2 - i, 8);
			dst->vLine(x2 - i, y1 + i + 1, y2 - i, 8);
			dst->setPixel(x1 + i, y2 - i, corner);
			dst->setPixel(x2 - i, y1 + i, corner);
		}
	}
	for (int i = 0; i < frame; ++i)
		dst->frameRect(Common::Rect(x1 + i, y1 + i, x2 - i + 1, y2 - i + 1), 0);
}

bool isOkKey(const Common::Event &e) {
	return e.kbd.keycode == Common::KEYCODE_RETURN || e.kbd.keycode == Common::KEYCODE_KP_ENTER ||
		e.kbd.keycode == Common::KEYCODE_SPACE;
}

Common::Rect buttonRect(const Graphics::ManagedSurface &image, int x, int y, int border) {
	return Common::Rect(x - border, y - border, x + image.w + border, y + image.h + border);
}

Graphics::ManagedSurface *loadImageObject(Common::File &exe, uint32 offset) {
	// u16 width, u16 height, u8 flags, u8 key colour, pixels
	exe.seek(offset);
	uint16 w = exe.readUint16LE();
	uint16 h = exe.readUint16LE();
	exe.readByte();
	byte key = exe.readByte();
	Graphics::ManagedSurface *image = new Graphics::ManagedSurface(w, h, Graphics::PixelFormat::createFormatCLUT8());
	for (int y = 0; y < h; ++y)
		exe.read(image->getBasePtr(0, y), w);
	image->setTransparentColor(key);
	return image;
}

void Interface::drawButton(int index) {
	int x = (index % 4) * 56 + 3, y = (index / 4) * 14 + 160;
	drawBevelButton(_screen, *_verbImages[index], x, y, kVerbButtons[index].verb == _verb);
}

void Interface::drawSentence() {
	// ae72:0a1c: verb, a space, the object's name; centred on the black strip
	Common::String text = _verbNames[_verb - kVerbFirst] + " ";
	if (_first) {
		// "Giv X til Y" / "Benyt X med Y"
		text += getObjectName(_first);
		text += readPascalString(*_ovr, _verb == kVerbGive ? kTextTil : kTextMed);
		if (_object > kNoObject && _object != _first)
			text += getObjectName(_object);
	} else if (_object > kNoObject) {
		text += getObjectName(_object);
	}
	if (text.size() > kMaxSentence)
		text = Common::String(text.c_str(), kMaxSentence);

	_screen->fillRect(Common::Rect(0, 146, 320, 156), 0);
	_font->drawString(_screen, text, (320 - (int)text.size() * 8) / 2, kSentenceY, kSentenceColor);
}

void Interface::showMessage(const Common::String &text, byte color) {
	// Text longer than 40 characters would run into the next pixel row in
	// the original (2a0c:06d5); cut it at the screen edge instead
	_screen->fillRect(Common::Rect(0, 0, 320, 8), 0);
	Common::String line = text.size() > kMaxSentence ? Common::String(text.c_str(), kMaxSentence) : text;
	_font->drawString(_screen, line, 0, 0, color);
}

Graphics::ManagedSurface *Interface::getIcon(int number) {
	if (number < 0 || number >= (int)ARRAYSIZE(kIconOffsets) || !kIconOffsets[number])
		number = 0; // icon 0: the placeholder
	if (!_icons.contains(number))
		_icons[number] = loadImageObject(*_exe, kIconOffsets[number]);
	return _icons[number];
}

void Interface::drawInventory() {
	if (!_inventory || !_page)
		return;
	// 14a3:00a4: scroll buttons, command 0x3e at (230,160), 0x3f at (230,181)
	drawBevelButton(_screen, *_scrollImages[0], 230, 160, false);
	drawBevelButton(_screen, *_scrollImages[1], 230, 181, false);

	// 14a3:02fd: items (page - 1) * 3 + 1 .. (page + 1) * 3 in two rows of three
	int first = (*_page - 1) * 3;
	for (int slot = 0; slot < 6; ++slot) {
		int col = slot % 3, row = slot / 3;
		int x = 243 + 26 * col, y = 159 + 21 * row;
		_screen->fillRect(Common::Rect(x, y, x + 23, y + 18), 7);
		uint index = first + slot;
		if (index < _inventory->size()) {
			const ObjectInfo *info = findObject(_location, (*_inventory)[index]);
			Graphics::ManagedSurface *icon = getIcon(info ? info->icon : 0);
			if (icon)
				_screen->transBlitFrom(*icon, Common::Point(x, y), icon->getTransparentColor());
		}
	}
}

int Interface::inventoryObjectAt(const Common::Point &pos) const {
	if (!_inventory || !_page)
		return 0;
	for (int slot = 0; slot < 6; ++slot) {
		int col = slot % 3, row = slot / 3;
		Common::Rect r(242 + 26 * col, 158 + 21 * row, 267 + 26 * col, 178 + 21 * row);
		uint index = (*_page - 1) * 3 + slot;
		if (r.contains(pos) && index < _inventory->size())
			return (*_inventory)[index];
	}
	return 0;
}

int Interface::scrollButtonAt(const Common::Point &pos) const {
	if (Common::Rect(228, 158, 241, 178).contains(pos))
		return 0x3e;
	if (Common::Rect(228, 179, 241, 199).contains(pos))
		return 0x3f;
	return 0;
}

void Interface::draw() {
	for (uint i = 0; i < ARRAYSIZE(kVerbButtons); ++i)
		drawButton(i);
	drawSentence();
	drawInventory();
}

} // End of namespace Miljostrup
