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

// The special screens KemSkab (location 23, overlay unit 52ee) and BordStol
// (32 / 33, overlay unit 502f), notes/special-screens.md.

#include "graphics/paletteman.h"

#include "olga/locations_data.h"
#include "olga/olga.h"

namespace Olga {

namespace {

const uint16 kSegKemSkab = 0x52ee;
const uint16 kSegBordStol = 0x502f;

enum {
	kCmdOk = 10,        // the OK button of both screens
	kCmdNone = 100,
	kIdBin = 195,       // the bin of Kommune Kemi (KEM_SKAB.SCN)
	kIdFirstProduct = 176,
	kIdLastProduct = 184,
	kIdPaintCupboard = 185,
	kEventLidDown = 0xb // KemSkab's own room event
};

// KemSkab_0398: the nine places on the shelves (x, y); 4, 5, 8 and 9 are at
// the back (the object flag)
const struct {
	int16 x, y;
	bool back;
} kKemPlaces[9] = {
	{ 99, 26, false }, { 13, 23, false }, { 58, 22, false }, { 95, 19, true }, { 22, 17, true },
	{ 42, 94, false }, { 91, 92, false }, { 20, 80, true }, { 71, 81, true }
};

// DS:025a: per product, the drawn part of its 44x58 picture (x1, y1, x2, y2)
const struct {
	int16 x1, y1, x2, y2;
} kKemShapes[9] = {
	{ 8, 0, 35, 57 }, { 6, 7, 36, 57 }, { 3, 10, 41, 57 }, { 3, 22, 41, 57 }, { 3, 10, 41, 57 },
	{ 3, 10, 41, 57 }, { 3, 10, 41, 57 }, { 10, 8, 32, 57 }, { 3, 10, 41, 57 }
};

// 52ee:0b2b: the dangerous ones, {1, 2, 4, 7, 8, 9}
const uint16 kKemRight = 0x0396;

const Common::Point kKemOk(0x86, 0xbc);
const Common::Point kBordOk(0x86, 0xb2);
const Common::Point kBordPicture(0xa7, 0x15);
const int kBordBevel = 3;

/**
 * KemSkab_004a / 0000: the n-th member of a set of 1..9, counting on from
 * 1 and wrapping with x mod 9 + 1. The count is raised once before the
 * wrap test, so after 9 it goes on from 2.
 */
int nthMember(uint16 set, int n) {
	int x = 1;
	while (!(set & (1 << x)))
		x = x % 9 + 1;
	for (int i = 2; i <= n; i++) {
		x++;
		while (!(set & (1 << x)))
			x = x % 9 + 1;
	}
	return x;
}

Common::Rect buttonArea(const LbmImage &img, const Common::Point &p, int bevel) {
	int e = 1 + bevel;
	return Common::Rect(p.x - e, p.y - e, p.x + img.surface.w + e, p.y + img.surface.h + e);
}

Common::Point bordButtonPos(int row, int col) {
	return Common::Point(col * 0x4f + 8, row * 0x49 + 0x18);
}

} // End of anonymous namespace

bool OlgaEngine::gotoKemSkab() {
	// KemSkab_06f7: a room view (0, 8)-(319, 173) with kem_skab.lbm / .scn
	// and no Olga
	LocationDef def = kLocations[23 - 1];
	def.lbm = "KEM_SKAB.LBM";
	def.scn = "KEM_SKAB.SCN";
	if (!_scene.load(def, 0))
		return false;
	_location = 23;
	_kind = kLocKemSkab;
	_nextLocation = 0;
	_pending = false;
	_textTimer = 0;
	memset(_controlOff, 0, sizeof(_controlOff));
	for (uint i = 0; i < _scene.scn().objects.size(); i++)
		refresh(_scene.scn().objects[i].hotspot.id);
	_kemItems.clear();
	_kemChosen = 0;
	_kemHover = kCmdNone;

	// KemSkab_05bc: the spirit (1) goes to one of the places 1, 2, 3, 6, 7
	// (52ee:053c); then every free place gets one of the others, at random
	int item = nthMember(1 << 1, getRandomNumber(8) + 1);
	int place = nthMember(0x00ce, getRandomNumber(8) + 1);
	kemPlace(item, place);
	uint16 places = (0x00ce & ~(1 << place)) | 0x0330; // + 52ee:057c
	uint16 items = 0x03fc;                              // 52ee:059c, 2..9
	for (int p = 1; p <= 9; p++) {
		if (!(places & (1 << p)))
			continue;
		item = nthMember(items, getRandomNumber(8) + 1);
		getRandomNumber(8); // the place: the only member of [p]
		kemPlace(item, p);
		items &= ~(1 << item);
		places &= ~(1 << p);
	}
	_scene.settle();

	g_system->getPaletteManager()->setPalette(_scene.palette(), 0, 256);
	_roomView.create(320, 166, Graphics::PixelFormat::createFormatCLUT8());
	_screen->clear(0);
	showText(_exe.readString(kSegKemSkab, 0x06cf), 14);
	_screen->hLine(0, 0xb8, 319, 15);
	_screen->fillRect(Common::Rect(0, 0xb9, 320, 200), 7);
	drawButton(_okImage, kKemOk.x, kKemOk.y, false);
	setInput(true);
	return true;
}

void OlgaEngine::kemPlace(int item, int place) {
	// KemSkab_01b6: product `item` (id 175 + item, picture item + 1, empty
	// picture 11) at the place, delay 4; its control covers the drawn part
	// of the picture, from the picture's top for the back places (008f)
	int id = kIdFirstProduct - 1 + item;
	_flags[id] = 0;
	const auto &p = kKemPlaces[place - 1];
	const auto &s = kKemShapes[item - 1];
	ScnObject def;
	def.x = p.x;
	def.y = p.y;
	def.delay = 4;
	def.start = 1;
	def.flag = p.back;
	def.hotspot.id = id;
	ScnAnim a;
	a.frames.push_back(item + 1);
	a.steps.push_back(1);
	def.anims.push_back(a);
	a.frames[0] = 11;
	def.anims.push_back(a);
	_scene.addObject(def);
	KemItem k;
	k.id = id;
	k.hot = Common::Rect(p.x + s.x1, p.y + (p.back ? 0 : s.y1), p.x + s.x2, p.y + s.y2);
	_kemItems.push_back(k);
	refresh(id);
}

void OlgaEngine::kemText(const Common::String &text, bool chosen) {
	// KemSkab_0d57: the line at y 175, colour 3 (pointed at) or 11 (chosen)
	_screen->fillRect(Common::Rect(0, 0xaf, 320, 0xb7), 0);
	if (!text.empty())
		_font.drawCentred(*_screen, text, 0xaf, chosen ? 0xb : 3);
}

int OlgaEngine::kemHitTest(const Common::Point &p) const {
	if (buttonArea(_okImage, kKemOk, 1).contains(p))
		return kCmdOk;
	Common::Point rp(p.x, p.y - kRoomTop);
	for (uint i = 0; i < _kemItems.size(); i++) {
		const KemItem &k = _kemItems[i];
		if (_controlOff[k.id])
			continue;
		if (rp.x >= k.hot.left && rp.x <= k.hot.right && rp.y >= k.hot.top && rp.y <= k.hot.bottom)
			return k.id;
	}
	return kCmdNone;
}

void OlgaEngine::kemHover(const Common::Point &p) {
	// KemSkab_08bb: a product's name, or nothing, when the command changes
	int id = kemHitTest(p);
	if (id == _kemHover)
		return;
	if (id >= kIdFirstProduct && id <= kIdLastProduct)
		kemText(objectName(id), false);
	else
		kemText(Common::String(), false);
	_kemHover = id;
}

void OlgaEngine::kemClick(const Common::Point &p) {
	int id = kemHitTest(p);
	if (id >= kIdFirstProduct && id <= kIdLastProduct) {
		// KemSkab_09f9: into the set, the name in colour 11, the product
		// gone (bit 0); 0976: the bin's lid up for 2 steps, event 0xb
		_kemChosen |= 1 << (id - kIdFirstProduct + 1);
		kemText(objectName(id), true);
		_kemHover = id;
		_flags[id] |= 1;
		refresh(id);
		setInput(false);
		SceneObject *bin = _scene.findObject(kIdBin);
		if (bin) {
			Scene::stopScript(*bin);
			SceneObject::Move m = { 0, 0, 2, 2 };
			bin->moves.push_back(m);
			_scene.startScript(*bin, kEventLidDown);
		}
	} else if (id == kCmdOk) {
		// KemSkab_0b4b: right iff the set is exactly the dangerous ones
		if (_kemChosen == kKemRight) {
			_flags[kIdPaintCupboard] |= 1;
			_pendingPraise = true;
		} else {
			_flags[kIdPaintCupboard] &= ~1;
			_pendingDevil = true;
		}
		setExit(_returnLocation, 1);
	}
}

void OlgaEngine::kemEvent(const SceneEvent &e) {
	// KemSkab_0cad / 0c65: the lid down, input again
	if (e.code != kEventLidDown)
		return;
	SceneObject *bin = _scene.findObject(kIdBin);
	if (bin)
		Scene::setAnim(*bin, 1);
	setInput(true);
}

bool OlgaEngine::gotoBordStol(int number) {
	// BordStol_03d1: 32 for pupil 225 (pictures 5..8, starts at 1), 33 for
	// 226 (9..12, starts at 3). BordStol_01c8 loads bordstol.lbm without
	// setting its palette: the room's stays (HYPOTHESIS)
	if (!_bordLbm.load("BORDSTOL.LBM"))
		return false;
	_location = number;
	_kind = kLocBordStol;
	_nextLocation = 0;
	_pending = false;
	_textTimer = 0;
	_bordId = number == 32 ? 225 : 226;
	_bordChoice = 0;
	_bordBase = _bordId == 225 ? 5 : 9;

	// BordStol_0027 / 04bf: the title line, stripes and the frame of the
	// picture
	_screen->fillRect(Common::Rect(0, 0, 320, 10), 0);
	_font.drawCentred(*_screen, _exe.readString(kSegBordStol, 0x0000), 1, 14);
	_screen->fillRect(Common::Rect(0, 10, 320, 12), 7);
	_screen->hLine(0, 12, 319, 0);
	_screen->fillRect(Common::Rect(0, 13, 320, 16), 8);
	_screen->hLine(0, 16, 319, 15);
	_screen->fillRect(Common::Rect(0, 17, 320, 168), 7);
	_screen->frameRect(Common::Rect(0xa6, 0x14, 0x13b, 0xa5), 0);
	_screen->hLine(0, 168, 319, 0);
	_screen->fillRect(Common::Rect(0, 169, 320, 172), 8);
	_screen->hLine(0, 172, 319, 15);
	_screen->fillRect(Common::Rect(0, 173, 320, 192), 7);
	_screen->hLine(0, 192, 319, 0);
	_screen->fillRect(Common::Rect(0, 193, 320, 196), 8);
	_screen->hLine(0, 196, 319, 15);
	_screen->fillRect(Common::Rect(0, 197, 320, 200), 7);
	// BordStol_034b / 02b6: the four choices (pictures 1..4, bevel 3,
	// commands 11..14) and OK
	for (int row = 0; row < 2; row++)
		for (int col = 0; col < 2; col++) {
			Common::Point p = bordButtonPos(row, col);
			drawButton(_bordLbm.image(row * 2 + col + 1), p.x, p.y, false, kBordBevel);
		}
	drawButton(_okImage, kBordOk.x, kBordOk.y, false);
	bordSelect(_bordId == 225 ? 1 : 3);
	setInput(true);
	return true;
}

void OlgaEngine::bordSelect(int choice) {
	// BordStol_0703: the pupil at that table and chair
	if (choice == _bordChoice)
		return;
	_bordChoice = choice;
	const LbmImage &img = _bordLbm.image(choice + _bordBase - 1);
	if (img.keyed)
		_screen->transBlitFrom(img.surface, kBordPicture, img.key);
	else
		_screen->blitFrom(img.surface, kBordPicture);
}

void OlgaEngine::bordClick(const Common::Point &p) {
	for (int row = 0; row < 2; row++)
		for (int col = 0; col < 2; col++)
			if (buttonArea(_bordLbm.image(row * 2 + col + 1), bordButtonPos(row, col), kBordBevel).contains(p)) {
				bordSelect(row * 2 + col + 1);
				return;
			}
	if (!buttonArea(_okImage, kBordOk, 1).contains(p))
		return;
	// BordStol_0546: 225 sits right at 3, 226 at 2
	bool right = (_bordId == 225 && _bordChoice == 3) || (_bordId == 226 && _bordChoice == 2);
	_flags[_bordId] = right ? 1 : 0;
	if (right)
		_pendingPraise = true;
	else
		_pendingDevil = true;
	setExit(_returnLocation, 1);
}

} // End of namespace Olga
