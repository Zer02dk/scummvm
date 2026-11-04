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

#include "common/events.h"

#include "olga/olga.h"
#include "olga/states_data.h"

namespace Olga {

namespace {

// Verb button k: image at x 3 + 57 * col, y 160 + 14 * row (Responses_1eee)
Common::Point verbPos(int k) {
	return Common::Point(3 + 57 * (k % 3), 160 + 14 * (k / 3));
}

// Inventory arrows (InvPanel_Init): images at 178,160 and 178,181
const Common::Point kArrowPos[2] = { Common::Point(178, 160), Common::Point(178, 181) };

// Inventory slot r, c: icon at 191 + 26c, 159 + 21r (InvPanel_0201), the
// control 190 + 26c .. 214 + 26c, 158 + 21r .. 177 + 21r (InvPanel_005e)
Common::Rect slotRect(int r, int c) {
	return Common::Rect(190 + 26 * c, 158 + 21 * r, 215 + 26 * c, 178 + 21 * r);
}

// An image button's rectangle: the image plus border 1 and bevel 1 (Gui_02d5)
Common::Rect buttonRect(const LbmImage &img, const Common::Point &p) {
	return Common::Rect(p.x - 2, p.y - 2, p.x + img.surface.w + 2, p.y + img.surface.h + 2);
}

} // End of anonymous namespace

void OlgaEngine::drawButton(const LbmImage &img, int x, int y, bool pressed, int bevel) {
	// Gui_0885: the image (opaque) at x, y, pressed `bevel` pixels lower-
	// right; Gui_0455 a black border of 1; for each bevel line i, Gui_04d6
	// (raised) draws light 15 top / left, dark 8 bottom / right and face 7
	// corners, Gui_067b (pressed) dark top / left with a face line inside
	const int b = 1;
	Common::Rect r(x - b - bevel, y - b - bevel, x + img.surface.w + b + bevel, y + img.surface.h + b + bevel);
	int x1 = r.left, y1 = r.top, x2 = r.right - 1, y2 = r.bottom - 1;
	_screen->blitFrom(img.surface, Common::Point(pressed ? x + bevel : x, pressed ? y + bevel : y));
	_screen->frameRect(r, 0);
	for (int i = b; i < b + bevel; i++) {
		if (!pressed) {
			_screen->hLine(x1 + i, y1 + i, x2 - i - 1, 15);
			_screen->vLine(x1 + i, y1 + i, y2 - i - 1, 15);
			_screen->hLine(x1 + i + 1, y2 - i, x2 - i, 8);
			_screen->vLine(x2 - i, y1 + i + 1, y2 - i, 8);
			_screen->setPixel(x1 + i, y2 - i, 7);
			_screen->setPixel(x2 - i, y1 + i, 7);
		} else {
			_screen->hLine(x1 + i, y1 + i, x2 - b, 8);
			_screen->vLine(x1 + i, y1 + i + 1, y2 - b, 8);
			_screen->hLine(x1 + i + bevel, y1 + i + bevel, x2 - b, 7);
			_screen->vLine(x1 + i + bevel, y1 + i + bevel + 1, y2 - b, 7);
		}
	}
}

void OlgaEngine::drawPanel() {
	// Responses_DrawPanel (5427:1e0d): message and sentence lines black, a
	// white line at y 156, the panel grey (7) with the divider at x 172 / 173
	_screen->fillRect(Common::Rect(0, 0, 320, kRoomTop), 0);
	_screen->fillRect(Common::Rect(0, kSentenceTop, 320, kPanelTop), 0);
	_screen->hLine(0, kPanelTop, 319, 15);
	_screen->fillRect(Common::Rect(0, kPanelTop + 1, 320, 200), 7);
	_screen->vLine(172, kPanelTop + 1, 199, 8);
	_screen->vLine(173, kPanelTop + 1, 199, 15);
	// and a dark frame (8) around each inventory slot (5427:1e92)
	for (int r = 0; r < 2; r++)
		for (int c = 0; c < 5; c++)
			_screen->frameRect(slotRect(r, c), 8);
	drawVerbs();
	for (int i = 0; i < 2; i++)
		drawButton(_arrowImages[i], kArrowPos[i].x, kArrowPos[i].y, false);
	drawInventory();
}

void OlgaEngine::drawVerbs() {
	// The chosen verb's button stays pressed (Gui1cb5_02a6)
	for (int k = 0; k < kNumVerbs; k++) {
		Common::Point p = verbPos(k);
		drawButton(_verbImages[k], p.x, p.y, kVerbSkub + k == _verb);
	}
}

void OlgaEngine::drawInventory() {
	// InvPanel_0314: two rows of five from row _inventoryRow; icons keyed,
	// empty slots grey
	int count = _inventory.size();
	if (_inventoryRow > 1 && count < (_inventoryRow - 1) * 5 + 1)
		_inventoryRow = (count - 1) / 5 + 1;
	if (_inventoryRow < 1)
		_inventoryRow = 1;
	int first = (_inventoryRow - 1) * 5 + 1;
	for (int k = 0; k < 10; k++) {
		int r = k / 5, c = k % 5, n = first + k;
		Common::Point p(191 + 26 * c, 159 + 21 * r);
		_screen->fillRect(Common::Rect(p.x, p.y, p.x + 23, p.y + 18), 7);
		if (n <= count) {
			int icon = kStates[_inventory[n - 1] - kFirstId].icon;
			if (icon >= 1 && icon <= 17)
				_screen->transBlitFrom(_icons[icon].surface, p, _icons[icon].key);
		}
	}
}

void OlgaEngine::drawSentence(bool running) {
	// Responses_DrawSentence (5427:1647): verb + " " + object; Brug / Giv on
	// a two-object id + " med " / " til " + the second object (5427:1641 /
	// 163b, in OLGA.OVR). Colour 3, 11 while the action runs.
	Common::String s = _exe.readString(kSegData, 0x0d14 + 11 * _verb);
	if (_obj1 != kNoObject) {
		s += " ";
		s += objectName(_obj1);
		if ((_verb == kVerbGiv || _verb == kVerbBrug) && kStates[_obj1 - kFirstId].twoObjects) {
			s += _exe.readString(0x5427, _verb == kVerbGiv ? 0x163b : 0x1641);
			if (_obj2 != kNoObject)
				s += objectName(_obj2);
		}
	}
	_screen->fillRect(Common::Rect(0, kSentenceTop, 320, kPanelTop), 0);
	_font.drawCentred(*_screen, s, kSentenceTop + 1, running ? 11 : 3);
}

bool OlgaEngine::carried(int id) const {
	for (uint i = 0; i < _inventory.size(); i++)
		if (_inventory[i] == id)
			return true;
	return false;
}

int OlgaEngine::hitTest(const Common::Point &p) {
	// Panel: verb buttons, arrows, inventory slots
	for (int k = 0; k < kNumVerbs; k++)
		if (buttonRect(_verbImages[k], verbPos(k)).contains(p))
			return kVerbSkub + k;
	for (int i = 0; i < 2; i++)
		if (buttonRect(_arrowImages[i], kArrowPos[i]).contains(p))
			return kCmdInvUp + i;
	int first = (_inventoryRow - 1) * 5 + 1;
	for (int k = 0; k < 10; k++)
		if (slotRect(k / 5, k % 5).contains(p))
			return first + k <= (int)_inventory.size() ? _inventory[first + k - 1] : kNoObject;
	// Room: the hotspots in SCN order, the first that contains the point
	// wins: the things in a cupboard come before the shelf and the cupboard
	// that contain them (GANG6: 101 .. 229, then 172, then 163), so the
	// first inserted control must be on top (HYPOTHESIS: Gui's search order;
	// it is the order that makes the cupboards work). Rectangles inclusive.
	if (p.y < kRoomTop || p.y >= kRoomTop + kRoomHeight)
		return kNoObject;
	Common::Point rp(p.x, p.y - kRoomTop);
	const ScnFile &scn = _scene.scn();
	int numObjects = scn.objects.size();
	for (int i = 0; i < numObjects + (int)scn.hotspots.size(); i++) {
		ScnHotspot h;
		int dx = 0, dy = 0;
		if (i >= numObjects) {
			h = scn.hotspots[i - numObjects];
		} else {
			h = scn.objects[i].hotspot;
			SceneObject *o = _scene.findObject(h.id);
			if (o) {
				dx = o->hotDx;
				dy = o->hotDy;
			}
		}
		if (!h.enabled() || h.id < kFirstId || h.id > kLastId)
			continue;
		if (hotspotOff(h.id))
			continue;
		if (rp.x >= h.x1 + dx && rp.x <= h.x2 + dx && rp.y >= h.y1 + dy && rp.y <= h.y2 + dy)
			return h.id;
	}
	return kNoObject;
}

void OlgaEngine::onHover(const Common::Point &p) {
	// Responses_Hover (5427:01d4): not while Olga walks to a target
	if (_pending)
		return;
	int id = hitTest(p);
	if (id < kFirstId || id > kLastId)
		return;
	int &slot = _stage == 1 ? _obj1 : _obj2;
	if (slot != id) {
		slot = id;
		drawSentence(false);
	}
}

void OlgaEngine::selectVerb(int verb) {
	// Responses_SelectVerb (5427:03a3) and Responses_ResetSentence
	_verb = verb;
	drawVerbs();
	resetSentence();
}

void OlgaEngine::onClick(const Common::Point &p) {
	int cmd = hitTest(p);
	bool isVerb = cmd >= kVerbSkub && cmd < kVerbSkub + kNumVerbs;
	bool isObject = cmd > kNoObject && cmd <= kLastId;
	// Responses_Command (5427:03fb): a new command cancels a walk first
	if (_pending && (isVerb || isObject))
		cancelWalk();
	if (isVerb) {
		selectVerb(cmd);
	} else if (isObject) {
		// Responses_ClickHotspot (5427:029c)
		if (_stage == 1) {
			_obj1 = cmd;
			if ((_verb == kVerbGiv || _verb == kVerbBrug) && kStates[cmd - kFirstId].twoObjects) {
				_stage = 2;
				_obj2 = cmd;
				drawSentence(false);
			} else {
				startVerb();
			}
		} else {
			_obj2 = cmd;
			startVerb();
		}
	} else if (cmd == kCmdInvUp || cmd == kCmdInvDown) {
		// InvPanel_049a / 04c6: one row of five up or down (HYPOTHESIS: the
		// exact limits)
		if (cmd == kCmdInvUp && _inventoryRow > 1)
			_inventoryRow--;
		else if (cmd == kCmdInvDown && _inventoryRow * 5 < (int)_inventory.size())
			_inventoryRow++;
		drawInventory();
	}
}

void OlgaEngine::onKey(const Common::KeyState &ks) {
	// Responses_LetterKey (5427:0973): the marked letter of each verb
	static const struct { char key; int verb; } keys[] = {
		{ 'K', kVerbSkub }, { 'R', kVerbTraek }, { 'B', kVerbBrug }, { 'A', kVerbTag },
		{ 'G', kVerbGiv }, { 'S', kVerbSePaa }, { 'L', kVerbLuk }, { 'T', kVerbTalTil }
	};
	if (ks.keycode == Common::KEYCODE_ESCAPE) {
		cancelWalk();
		return;
	}
	if (ks.ascii == 0xe5 || ks.ascii == 0xc5) { // å / Å: Åben
		if (_pending)
			cancelWalk();
		selectVerb(kVerbAaben);
		return;
	}
	char c = toupper(ks.ascii & 0xff);
	for (uint i = 0; i < ARRAYSIZE(keys); i++) {
		if (keys[i].key == c && _verb != keys[i].verb) {
			if (_pending)
				cancelWalk();
			selectVerb(keys[i].verb);
			return;
		}
	}
}

} // End of namespace Olga
