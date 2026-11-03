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

// The supermarket (location 7), written from notes/supermarket.md. Part 1:
// the yard and the back room (rooms 7 and 6): the stack of boxes, the waste
// sorting, the panels with close-ups 8 (temperature) and 9 (light at night).
// Part 2: the shop (room 5): the freezers and their switches, the boxes into
// the freezers, the counter's award, the lamps and the light switches.

#include "common/events.h"
#include "common/system.h"
#include "graphics/framelimiter.h"

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/handlers_data.h"
#include "miljostrup/response_data.h"

namespace Miljostrup {

enum {
	kSupermarket = 7,
	kCounter = 104,           // +0x2f the boxes stored (53fc:066b)
	kChestFirst = 105,        // 105 / 106: +0x2f on, +0x30 fill 0..4
	kCabinetFirst = 112,      // 112..114: +0x2f closed, +0x30 fill 0..2
	kSwitchFirst = 115,       // 115..117: +0x2f on (the cabinet freezer 112 + k)
	kEmptyProxyFirst = 133,   // 133..138: the empty boxes lying in room 5
	kLampFirst = 143,         // 143..145
	kLightSwitchA = 146,      // 146 / 147: +0x2f on, each the other's partner
	kLightSwitchB = 147,
	kPlasticContainer = 101,
	kOtherContainer = 102,
	kPapContainer = 103,
	kBoxFirst = 120,          // the boxes 120..125: +0x333 empty
	kStackProxyFirst = 126,   // 126..131 stand for the boxes in the stack
	kStack = 132,             // +0x47 the count (start 6)
	kTempPanel = 118,         // +0x12f set, +0x132 the temperature (start 26)
	kLightPanel = 119,        // +0x132 the light at night 0..3 (start 1)
	kBoxEmpty = 0x333,
	kStackCount = 0x47,
	kPanelSet = 0x12f,
	kPanelValue = 0x132
};

void MiljostrupEngine::initSupermarket() {
	// 53fc:24b8, 198a / 1f8c, 0941, 0f62, 161f, 2a39 (5326:0449's arguments)
	setField(kStack, kStackCount, 6);
	setField(kTempPanel, kPanelValue, 26);
	setField(kLightPanel, kPanelValue, 1);
	for (int k = 0; k < 2; ++k) {
		setField(kChestFirst + k, 0x2f, 1);
		setField(kChestFirst + k, 0x30, k == 0 ? 0 : 2);
	}
	for (int k = 0; k < 3; ++k) {
		setField(kCabinetFirst + k, 0x2f, 1);
		setField(kCabinetFirst + k, 0x30, 0);
		setField(kSwitchFirst + k, 0x2f, 1);
	}
	setField(kLightSwitchA, 0x2f, 1);
	setField(kLightSwitchB, 0x2f, 1);
	setField(kCounter, 0x2f, 0);
}

static bool isChest(int object) {
	return object == kChestFirst || object == kChestFirst + 1;
}

static bool isCabinet(int object) {
	return object >= kCabinetFirst && object < kCabinetFirst + 3;
}

bool MiljostrupEngine::isCabinetOn(int cabinet) const {
	// 53fc:0fdb: its switch's +0x2f
	return getField(kSwitchFirst + cabinet - kCabinetFirst, 0x2f) != 0;
}

void MiljostrupEngine::storeBox(int box, int freezer) {
	// 53fc:0dcf / 1409: into a freezer; the box's VMT 0x60 = 28d3 (empty, out
	// of the inventory, its empty proxy shown in room 5), the freezer's rule,
	// the counter (07f1)
	if (isChest(freezer)) {
		if (!getField(freezer, 0x2f)) {
			say(handlerText(0x53fc0dac), 14, true);
			return;
		}
		if (getField(freezer, 0x30) == 4) {
			say(handlerText(0x53fc0dbc), 14, true);
			return;
		}
	} else {
		if (getField(freezer, 0x2f)) {
			say(handlerText(0x53fc13d7), 14, true);
			return;
		}
		if (!isCabinetOn(freezer)) {
			say(handlerText(0x53fc13e6), 14, true);
			return;
		}
		if (getField(freezer, 0x30) == 2) {
			say(handlerText(0x53fc13f6), 14, true);
			return;
		}
	}
	setField(freezer, 0x30, getField(freezer, 0x30) + 1);
	setField(box, kBoxEmpty, 1);
	removeObject(box);
	int proxy = kEmptyProxyFirst + box - kBoxFirst;
	setFlag(proxy, 1);
	refreshObject(proxy);
	refreshObject(freezer);
	countStoredBox();
}

void MiljostrupEngine::countStoredBox() {
	// 53fc:07f1: when all six are stored
	setField(kCounter, 0x2f, getField(kCounter, 0x2f) + 1);
	if (getField(kCounter, 0x2f) != 6)
		return;
	bool cabinetsEmpty = true;
	for (int k = 0; k < 3; ++k) {
		if (getField(kCabinetFirst + k, 0x30))
			cabinetsEmpty = false;
	}
	if (cabinetsEmpty) {
		award(10, 10, 0, 0, 0x53fc06c3, 0x53fc07ee);
	} else {
		// both chest freezers empty or full: -10, else -5 (the same text)
		bool extreme = true;
		for (int k = 0; k < 2; ++k) {
			int fill = getField(kChestFirst + k, 0x30);
			if (fill != 0 && fill <= 3)
				extreme = false;
		}
		int v = extreme ? -10 : -5;
		award(v, v, 0, 0, 0x53fc073d, 0x53fc07ee);
	}
	setQuiz(0x53fc07b0, 0x53fc07c9, 0x53fc07dd, 0, 1);
}

bool MiljostrupEngine::isSuperObjectEnabled(int object, bool &enabled) const {
	// 53fc:263f: the stack's region goes when the last box is taken
	if (object == kStack) {
		enabled = getField(kStack, kStackCount) > 0;
		return true;
	}
	return false;
}

void MiljostrupEngine::adjustSuperRegion(int object, Common::Rect &r) const {
	// 53fc:263f (28c3:0233): the region's left edge follows the stack
	if (object == kStack)
		r.left = (6 - getField(kStack, kStackCount)) * 15 + 9;
}

int MiljostrupEngine::getSuperFigureState(uint32 rule, int object, bool &visible) const {
	switch (rule) {
	case 0x53fc263f: { // the stack: one state per box, gone at 0
		int count = getField(kStack, kStackCount);
		visible = count > 0;
		return count > 0 ? count : 1;
	}
	case 0x53fc2433: // the light panel: the setting
		return getField(kLightPanel, kPanelValue) + 1;
	case 0x53fc196b: // the temperature panel (no figure): its award, panelAward
		return 0;
	case 0x53fc0eaa: // a chest freezer: off 1, on 2 + fill
		return getField(object, 0x2f) ? 2 + getField(object, 0x30) : 1;
	case 0x53fc151c: { // a cabinet freezer
		bool closed = getField(object, 0x2f) != 0;
		int fill = getField(object, 0x30);
		if (!isCabinetOn(object))
			return closed ? 3 : 4; // off: closed 3, open 4
		if (closed)
			return fill == 0 ? 1 : fill == 1 ? 5 : 6;
		return fill == 0 ? 2 : fill == 1 ? 7 : 8;
	}
	case 0x53fc2ed9: // the lamps: shown while both switches are on
		visible = getField(kLightSwitchA, 0x2f) && getField(kLightSwitchB, 0x2f);
		return 1;
	default:
		return 0;
	}
}

static bool isBox(int object) {
	return object >= kBoxFirst && object < kBoxFirst + 6;
}

void MiljostrupEngine::sortSuperWaste(int object, int target) {
	// 53fc:026f: "Du har smidt " + the item + " i " + the container + ". " +
	// praise, or scolding + the right container; Award(v, 0, v, 0) or
	// (-v, 0, -v, 0), the quiz for wrong trays / plastic (01aa), then out of
	// the inventory
	const SuperWaste *item = nullptr;
	for (uint i = 0; i < ARRAYSIZE(kSuperWaste); ++i) {
		if (kSuperWaste[i].object == object)
			item = &kSuperWaste[i];
	}
	if (!item || target < kPlasticContainer || target > kPapContainer) {
		sayRootMessage(kVerbUse);
		return;
	}
	auto container = [this](int bin) {
		return handlerText(bin == kOtherContainer ? 0x53fc04f4 : bin == kPlasticContainer ? 0x53fc0512 : 0x53fc0531);
	};
	auto material = [this](int bin) {
		return handlerText(bin == kOtherContainer ? 0x53fc05c1 : bin == kPlasticContainer ? 0x53fc05ce : 0x53fc05dc);
	};
	Common::String definite = readPascalString(_ovr, item->definite);
	bool right = target == item->container;
	Common::String text = handlerText(0x53fc0237) + definite + handlerText(0x53fc0245) + container(target) +
		handlerText(0x53fc0249);
	if (right) {
		text += readPascalString(_exe, kSuperPraise[getRandomNumber(4)]);
	} else {
		text += readPascalString(_exe, kSuperScold[getRandomNumber(3)]) + handlerText(0x53fc024c) +
			capitalize(definite) + handlerText(0x53fc024e) + material(item->container) +
			handlerText(0x53fc0253) + container(item->container) + handlerText(0x53fc026d);
	}
	int v = right ? item->value : -item->value;
	awardText(v, 0, v, 0, text, Common::String());
	if (!right && (object == 139 || object == 141)) {
		if (object == 139)
			setQuiz(0x53fc0133, 0x53fc0166, 0x53fc0169, 0, 2);
		else
			setQuiz(0x53fc016d, 0x53fc0197, 0x53fc01a1, 1, 2);
	}
	removeObject(object);
}

int MiljostrupEngine::runSuperTemperatureScreen(int value) {
	// Close-up 8 (5148, base 8f88:0000 with palette A, origin (0, 0)):
	// picture 1477, the value as digits 1481 + d at (119, 70) / (128, 70), a
	// bar of 26 segments at (206, 44 + 4 k), lit (1492) when 26 - k <= value;
	// down (0x3f) at (110, 116), up (0x3e) at (144, 116), 10..26 with sound
	// 1d26:0178; OK (0x41) at (200, 184), Cancel (0x40) at (259, 184)
	Graphics::ManagedSurface *background = _archive.loadImage(1477);
	Graphics::ManagedSurface *digit[10];
	for (int d = 0; d < 10; ++d)
		digit[d] = _archive.loadImage(1481 + d);
	Graphics::ManagedSurface *segment[2] = { _archive.loadImage(1491), _archive.loadImage(1492) };
	Graphics::ManagedSurface *down = loadImageObject(_exe, kButtonDownImage);
	Graphics::ManagedSurface *up = loadImageObject(_exe, kButtonUpImage);
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *cancel = loadImageObject(_exe, kButtonCancelImage);
	auto blit = [this](Graphics::ManagedSurface *image, int x, int y) {
		if (image)
			_screen->transBlitFrom(*image, Common::Point(x, y), image->getTransparentColor());
	};

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);
	int result = 0;
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!result && !shouldQuit()) {
		if (redraw) {
			if (background)
				_screen->blitFrom(*background, Common::Point(0, 0));
			blit(digit[value / 10], 119, 70);
			blit(digit[value % 10], 128, 70);
			for (int k = 0; k < 26; ++k)
				blit(segment[26 - k <= value], 206, 44 + 4 * k);
			drawBevelButton(_screen, *down, 110, 116, false);
			drawBevelButton(_screen, *up, 144, 116, false);
			drawBevelButton(_screen, *ok, 200, 184, false);
			drawBevelButton(_screen, *cancel, 259, 184, false);
			redraw = false;
		}
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			int step = 0;
			if (e.type == Common::EVENT_KEYDOWN) {
				if (isOkKey(e))
					result = 1;
				else if (e.kbd.keycode == Common::KEYCODE_ESCAPE)
					result = -1;
				else if (e.kbd.keycode == Common::KEYCODE_UP)
					step = 1;
				else if (e.kbd.keycode == Common::KEYCODE_DOWN)
					step = -1;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				const Common::Point &m = e.mouse;
				if (buttonRect(*ok, 200, 184).contains(m)) {
					if (trackButton(*ok, 200, 184))
						result = 1;
				} else if (buttonRect(*cancel, 259, 184).contains(m)) {
					if (trackButton(*cancel, 259, 184))
						result = -1;
				} else if (buttonRect(*up, 144, 116).contains(m)) {
					if (trackButton(*up, 144, 116))
						step = 1;
				} else if (buttonRect(*down, 110, 116).contains(m)) {
					if (trackButton(*down, 110, 116))
						step = -1;
				}
			}
			if ((step > 0 && value < 26) || (step < 0 && value > 10)) {
				value += step;
				playSound(kSoundClicks);
				redraw = true;
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	delete background;
	for (int d = 0; d < 10; ++d)
		delete digit[d];
	delete segment[0];
	delete segment[1];
	delete down;
	delete up;
	delete ok;
	delete cancel;
	return result == 1 ? value : -1;
}

int MiljostrupEngine::runSuperLightScreen(int value) {
	// Close-up 9 (50e3): picture 1478; four lamps at (46 / 109 / 172 / 235,
	// 49), 1493, the chosen one 1494; a radio group of four image buttons
	// (frame and bevel 0: 1495, pressed 1496) at (42 / 105 / 168 / 231, 83),
	// commands 0x42..0x45 = 0..3; OK (0x41) at (132, 184), Cancel (0x40) at
	// (259, 184)
	static const int kLampX[4] = { 46, 109, 172, 235 };
	static const int kButtonX[4] = { 42, 105, 168, 231 };
	Graphics::ManagedSurface *background = _archive.loadImage(1478);
	Graphics::ManagedSurface *lamp[2] = { _archive.loadImage(1493), _archive.loadImage(1494) };
	Graphics::ManagedSurface *button[2] = { _archive.loadImage(1495), _archive.loadImage(1496) };
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *cancel = loadImageObject(_exe, kButtonCancelImage);
	auto blit = [this](Graphics::ManagedSurface *image, int x, int y) {
		if (image)
			_screen->transBlitFrom(*image, Common::Point(x, y), image->getTransparentColor());
	};

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);
	int result = 0;
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!result && !shouldQuit()) {
		if (redraw) {
			if (background)
				_screen->blitFrom(*background, Common::Point(0, 0));
			for (int k = 0; k < 4; ++k) {
				blit(lamp[k == value], kLampX[k], 49);
				blit(button[k == value], kButtonX[k], 83);
			}
			drawBevelButton(_screen, *ok, 132, 184, false);
			drawBevelButton(_screen, *cancel, 259, 184, false);
			redraw = false;
		}
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				if (isOkKey(e))
					result = 1;
				else if (e.kbd.keycode == Common::KEYCODE_ESCAPE)
					result = -1;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				const Common::Point &m = e.mouse;
				if (buttonRect(*ok, 132, 184).contains(m)) {
					if (trackButton(*ok, 132, 184))
						result = 1;
				} else if (buttonRect(*cancel, 259, 184).contains(m)) {
					if (trackButton(*cancel, 259, 184))
						result = -1;
				} else if (button[0]) {
					for (int k = 0; k < 4; ++k) {
						if (Common::Rect(kButtonX[k], 83, kButtonX[k] + button[0]->w, 83 + button[0]->h).contains(m)) {
							value = k;
							redraw = true;
						}
					}
				}
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	delete background;
	for (int k = 0; k < 2; ++k) {
		delete lamp[k];
		delete button[k];
	}
	delete ok;
	delete cancel;
	return result == 1 ? value : -1;
}

void MiljostrupEngine::superPanelAward(int panel) {
	// The panel's VMT 0x60 at the room's next build (rule 196b): 1c9f the
	// temperature, 2309 the light at night
	int v = getField(panel, kPanelValue);
	if (panel == kTempPanel) {
		// Award(p, p, 0, 0), p from DS:11f0[t]
		static const int kPoints[27] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 2, 3, 4, 4, 5, 6, 8, 10, 10, 10, 5, 0, -2, -5, -8, -10 };
		int p = v >= 10 && v <= 26 ? kPoints[v] : 0;
		Common::String n = Common::String::format("%d", v) + readPascalString(_ovr, kTextDegrees);
		Common::String text;
		if (v == 26)
			text = handlerText(0x53fc1a3a);
		else if (v >= 21)
			text = handlerText(0x53fc1aba) + n + handlerText(0x53fc1afa) + handlerText(0x53fc1b06);
		else if (v >= 18)
			text = handlerText(0x53fc1b11) + n + handlerText(0x53fc1b3f) + handlerText(0x53fc1b42) + handlerText(0x53fc1b77);
		else if (v >= 14)
			text = handlerText(0x53fc1b11) + n + handlerText(0x53fc1b8e) + handlerText(0x53fc1ba3) + handlerText(0x53fc1bda);
		else
			text = handlerText(0x53fc1b11) + n + handlerText(0x53fc1bfd) + handlerText(0x53fc1c19) + handlerText(0x53fc1c51);
		awardText(p, p, 0, 0, text, handlerText(0x53fc1c9c));
		setQuiz(0x53fc1c61, 0x53fc1c92, 0x53fc1c97, 1, 1);
	} else {
		static const uint32 kTexts[4] = { 0x53fc203c, 0x53fc2100, 0x53fc2180, 0x53fc220d };
		static const int kPoints[4] = { 0, -10, 10, 0 };
		int p = v >= 0 && v < 4 ? kPoints[v] : 0;
		award(p, p, 0, 0, kTexts[v & 3], 0);
		setQuiz(0x53fc22d3, 0x53fc2300, 0x53fc2304, 1, 1);
	}
}

bool MiljostrupEngine::runSuperHandler(uint32 handler, int verb, int object, int second) {
	if (_location != kSupermarket)
		return false;
	switch (handler) {
	// The stack 132: Tag (2548) the top box, Se på (25a1)
	case 0x53fc2548: {
		int count = getField(kStack, kStackCount);
		if (count <= 0)
			break;
		setFlag(kStackProxyFirst + count - 1, 0);
		takeObject(kBoxFirst + count - 1);
		setField(kStack, kStackCount, count - 1);
		refreshObject(kStack);
		break;
	}
	case 0x53fc25a1: {
		int count = getField(kStack, kStackCount);
		say(handlerText(0x53fc257b) + Common::String::format("%d", count) + handlerText(0x53fc2583) +
			(count == 1 ? Common::String() : handlerText(0x53fc2579)) + handlerText(0x53fc258a), 14, true);
		break;
	}

	// The boxes 120..125: Se på (295c), Benyt (29bf)
	case 0x53fc295c:
		say(handlerText(getField(object, kBoxEmpty) ? 0x53fc2915 : 0x53fc2934), 14, true);
		break;
	case 0x53fc29bf:
		if (getField(object, kBoxEmpty))
			sortSuperWaste(object, second);
		else if (isChest(second) || isCabinet(second))
			storeBox(object, second); // the freezer's Benyt (VMT 0x30) with the box
		else
			sayRootMessage(kVerbUse);
		break;

	// The chest freezers 105 / 106
	case 0x53fc0c91: { // Se på
		static const uint32 kTexts[5] = { 0x53fc0bf0, 0x53fc0c06, 0x53fc0c23, 0x53fc0c40, 0x53fc0c60 };
		if (!getField(object, 0x2f))
			say(handlerText(0x53fc0c77), 14, true);
		else
			say(handlerText(kTexts[CLIP(getField(object, 0x30), 0, 4)]), 14, true);
		break;
	}
	case 0x53fc0b8e: // Tænd: it cannot be switched on again
		say(handlerText(getField(object, 0x2f) ? 0x53fc0b77 : 0x53fc0b5f), 14, true);
		break;
	case 0x53fc0a88: // Sluk: only when empty
		if (!getField(object, 0x2f)) {
			say(handlerText(0x53fc0a6f), 14, true);
		} else if (getField(object, 0x30)) {
			say(handlerText(0x53fc0a5c), 14, true);
		} else {
			setField(object, 0x2f, 0);
			refreshObject(object);
			award(5, 5, 0, 0, 0x53fc09b3, 0x53fc0a59);
		}
		break;
	case 0x53fc0dcf: // Benyt with a box
		if (isBox(second) && !getField(second, kBoxEmpty))
			storeBox(second, object);
		else
			sayRootMessage(kVerbUse);
		break;

	// The cabinet freezers 112..114
	case 0x53fc1161: // Åbn
		if (!getField(object, 0x2f)) {
			say(handlerText(0x53fc114b), 14, true);
		} else {
			setField(object, 0x2f, 0);
			refreshObject(object);
		}
		break;
	case 0x53fc11c4: // Luk
		if (getField(object, 0x2f)) {
			say(handlerText(0x53fc11ac), 14, true);
		} else {
			setField(object, 0x2f, 1);
			refreshObject(object);
		}
		break;
	case 0x53fc1273: { // Se på
		static const uint32 kTexts[3] = { 0x53fc120f, 0x53fc1225, 0x53fc1242 };
		if (!isCabinetOn(object))
			say(handlerText(0x53fc1259), 14, true);
		else
			say(handlerText(kTexts[CLIP(getField(object, 0x30), 0, 2)]), 14, true);
		break;
	}
	case 0x53fc1409: // Benyt with a box
		if (isBox(second) && !getField(second, kBoxEmpty))
			storeBox(second, object);
		else
			sayRootMessage(kVerbUse);
		break;

	// The cabinet freezers' switches 115..117
	case 0x53fc16b1: // Se på
		say(handlerText(getField(object, 0x2f) ? 0x53fc1693 : 0x53fc16a1), 14, true);
		break;
	case 0x53fc1742: // Tænd
		say(handlerText(getField(object, 0x2f) ? 0x53fc1713 : 0x53fc172a), 14, true);
		break;
	case 0x53fc17bd: { // Sluk: the freezer's VMT 0x64 (101e) may refuse
		int cabinet = kCabinetFirst + object - kSwitchFirst;
		if (!getField(object, 0x2f)) {
			say(handlerText(0x53fc17a4), 14, true);
		} else if (getField(cabinet, 0x30)) {
			say(handlerText(0x53fc100b), 14, true);
		} else {
			setField(object, 0x2f, 0);
			refreshObject(cabinet);
			playSound(kSoundClick);
			award(5, 5, 0, 0, 0x53fc106c, 0x53fc10e3); // its VMT 0x68 (10e6)
		}
		break;
	}

	// The lamps 143..145: Se på (2e1f); Sluk calls Tænd ("Benyt kontakten!")
	case 0x53fc2e1f:
		say(handlerText(getField(kLightSwitchA, 0x2f) && getField(kLightSwitchB, 0x2f) ?
			0x53fc2deb : 0x53fc2e02), 14, true);
		break;
	case 0x53fc2ec6:
		say(handlerText(0x53fc2e84), 14, true);
		break;

	// The light switches 146 / 147
	case 0x53fc2afb: // Se på
		say(handlerText(getField(object, 0x2f) ? 0x53fc2add : 0x53fc2aeb), 14, true);
		break;
	case 0x53fc2b8c: // Tænd
		say(handlerText(getField(object, 0x2f) ? 0x53fc2b5d : 0x53fc2b74), 14, true);
		break;
	case 0x53fc2c8d: { // Sluk: only one of the two
		int other = object == kLightSwitchA ? kLightSwitchB : kLightSwitchA;
		if (!getField(object, 0x2f)) {
			say(handlerText(0x53fc2c74), 14, true);
		} else if (!getField(other, 0x2f)) {
			say(handlerText(0x53fc2c5c), 14, true);
		} else {
			setField(object, 0x2f, 0);
			award(10, 10, 0, 0, 0x53fc2bee, 0);
			for (int k = 0; k < 3; ++k)
				refreshObject(kLampFirst + k);
			playSound(kSoundClick);
		}
		break;
	}

	// The trays 139 and the plastic 141: Benyt with a container (026f)
	case 0x53fc026f:
		sortSuperWaste(object, second);
		break;

	// The panels 118 / 119: Benyt (1903) opens the close-up once
	case 0x53fc1903: {
		if (getField(object, kPanelSet)) {
			say(handlerText(0x53fc18e7), 14, true);
			break;
		}
		int v = getField(object, kPanelValue);
		v = object == kTempPanel ? runSuperTemperatureScreen(v) : runSuperLightScreen(v);
		if (v >= 0) {
			// VMT 0x64(value) (1a08 / 200a): the value, set; the award at
			// the room's next build (back to room 6 node 0)
			setField(object, kPanelValue, v);
			setField(object, kPanelSet, 1);
			redrawRoom();
			superPanelAward(object);
		} else {
			redrawRoom();
		}
		break;
	}

	default:
		return false;
	}
	return true;
}

} // End of namespace Miljostrup
