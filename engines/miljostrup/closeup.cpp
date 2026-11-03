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

// Close-ups that the original draws by code, written from
// notes/thermostat.md and notes/kitchen.md.

#include "common/events.h"
#include "common/system.h"
#include "graphics/framelimiter.h"

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/rooms.h"
#include "miljostrup/verbs.h"
#include "miljostrup/response_data.h"

namespace Miljostrup {

enum {
	// 8f88:0000 called with (0, 22): the screen's origin, fill colour 0x1a
	kScreenX = 0,
	kScreenY = 22,
	kScreenFill = 26,
	// 8e40:034a: the four valve dials, levels 0..3 ("0", "2", "4", "6")
	kDialImage = 117,
	// 9000:0451 / 055d: thermometer column for 18, 20, 22, 24 degrees
	kColumnImage = 121,
	// 8af9:0248: the egg water close-up's buttons and water pictures
	kWaterPicture = 286,
	kWaterImage = 287,
	kWaterDialImage = 291,
	// 8d9d:0297: close-ups 18/19, picture 178; TV states 179..181, video 182..184
	kTVPicture = 178,
	// 8a94:028c: the bath close-up's six choices
	kBathImage = 371,
	// Close-ups 27..29: the washing powder, the dosing chart, the dosing scale
	kDosingPicture = 390,
	kPowderPicture = 393,
	kSchedulePicture = 394,
	kHardnessImage = 395,     // 395..397: hard, medium, soft
	kDosingOkImage = 398,
	kDosingMarkerImage = 400,
	kDosingOkX = 181, kDosingOkY = 165,
	kDoseSteps = 24,          // 11f8:010e: at most 0x18
	kOkX = 132, kOkY = 184,     // 8e40 / 8af9: command 0x123; 8ebc: 0x124
	kCancelX = 259, kCancelY = 184
};

// 8e40:04f9 / 8af9:0544: dial k at the screen's origin + this offset (commands 0x125 + k)
static const int kDialPos[4][2] = { { 167, 6 }, { 245, 6 }, { 167, 74 }, { 245, 74 } };

bool MiljostrupEngine::trackButton(const Graphics::ManagedSurface &image, int x, int y, int frame, int bevel,
		int light, int corner) {
	// 28c3:0883 mouse down: pressed; 08a8 mouse move: pressed while inside;
	// 08f8 mouse up: raised, the command (+0x0e) only when inside
	const Common::Rect rect = buttonRect(image, x, y, frame + bevel);
	bool inside = true;
	drawBevelButton(_screen, image, x, y, true, frame, bevel, light, corner);
	_screen->update();
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!shouldQuit()) {
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_MOUSEMOVE && rect.contains(e.mouse) != inside) {
				inside = !inside;
				drawBevelButton(_screen, image, x, y, inside, frame, bevel, light, corner);
			} else if (e.type == Common::EVENT_LBUTTONUP) {
				inside = rect.contains(e.mouse);
				drawBevelButton(_screen, image, x, y, false, frame, bevel, light, corner);
				_screen->update();
				return inside;
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	return false;
}

void MiljostrupEngine::drawScreenTitle(const Common::String &title) {
	// 8f88:0130: black strip at the top, the text centred in colour 14
	_screen->fillRect(Common::Rect(0, 0, 320, 8), 0);
	_font.drawString(_screen, title, (320 - (int)title.size() * 8) / 2, 0, 14);
}

int MiljostrupEngine::runDialScreen(uint16 picture, uint16 firstDial, const Common::String &title,
		int displayX, int displayY, const uint16 displayImages[4], int level) {
	Graphics::ManagedSurface *background = _archive.loadImage(picture);
	Graphics::ManagedSurface *dial[4];
	Graphics::ManagedSurface *display[4];
	for (int k = 0; k < 4; ++k) {
		dial[k] = _archive.loadImage(firstDial + k);
		display[k] = displayImages[k] ? _archive.loadImage(displayImages[k]) : nullptr;
	}
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *cancel = loadImageObject(_exe, kButtonCancelImage);

	setPalette(kPaletteB);
	_interface->setCursor(Interface::kCursorArrow);

	int result = -2; // -2 = still open
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (result == -2 && !shouldQuit()) {
		if (redraw) {
			redraw = false;
			// 8f88:009a: fill, picture at the origin, then the widgets
			_screen->fillRect(Common::Rect(0, 0, 320, 200), kScreenFill);
			if (background)
				_screen->transBlitFrom(*background, Common::Point(kScreenX, kScreenY), background->getTransparentColor());
			// The display shows the chosen level (9000:055d / 8af9:03e7)
			if (display[level])
				_screen->transBlitFrom(*display[level], Common::Point(displayX, displayY), display[level]->getTransparentColor());
			// Dials: a radio group (1ccf), frame 2 and bevel 2 (8e40:0488 / 8af9:04d1)
			for (int k = 0; k < 4; ++k) {
				if (dial[k])
					drawBevelButton(_screen, *dial[k], kScreenX + kDialPos[k][0], kScreenY + kDialPos[k][1], k == level, 2, 2);
			}
			drawBevelButton(_screen, *ok, kOkX, kOkY, false);
			drawBevelButton(_screen, *cancel, kCancelX, kCancelY, false);
			drawScreenTitle(title);
		}

		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				// 8e40:0220 / 8af9:011a: Enter or Space = OK, Esc = cancel
				if (isOkKey(e))
					result = level;
				else if (e.kbd.keycode == Common::KEYCODE_ESCAPE)
					result = -1;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (buttonRect(*ok, kOkX, kOkY, 2).contains(e.mouse)) {
					if (trackButton(*ok, kOkX, kOkY))
						result = level;
				} else if (buttonRect(*cancel, kCancelX, kCancelY, 2).contains(e.mouse)) {
					if (trackButton(*cancel, kCancelX, kCancelY))
						result = -1; // +0x1f5 := 1
				} else {
					for (int k = 0; k < 4; ++k) {
						if (dial[k] && k != level && buttonRect(*dial[k], kScreenX + kDialPos[k][0],
								kScreenY + kDialPos[k][1], 4).contains(e.mouse)) {
							level = k;
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
	for (int k = 0; k < 4; ++k) {
		delete dial[k];
		delete display[k];
	}
	delete ok;
	delete cancel;
	return result == -2 ? -1 : result;
}

int MiljostrupEngine::runThermostat(uint16 picture, int dx, int dy, int level, const int temperatures[4]) {
	// 9000:0451: the thermometer column for the level's temperature, at the
	// display's position + (16, 16)
	uint16 column[4];
	for (int k = 0; k < 4; ++k) {
		int t = temperatures[k];
		column[k] = t >= 18 && t <= 24 && t % 2 == 0 ? kColumnImage + (t - 18) / 2 : 0;
	}
	return runDialScreen(picture, kDialImage, readPascalString(_ovr, kTextThermostatTitle),
		kScreenX + dx + 16, kScreenY + dy + 16, column, level);
}

int MiljostrupEngine::runEggWater() {
	// 8af9:06ac: picture 286, buttons 291..294 for 1..4 dl, the water
	// (287..290) at the origin + (15, 47); the choice starts at 4 dl
	static const uint16 kWater[4] = { kWaterImage, kWaterImage + 1, kWaterImage + 2, kWaterImage + 3 };
	int level = runDialScreen(kWaterPicture, kWaterDialImage, readPascalString(_ovr, kTextEggWaterTitle),
		kScreenX + 15, kScreenY + 47, kWater, 3);
	return level < 0 ? -1 : level + 1;
}

int MiljostrupEngine::runBathChoice() {
	// Close-up 26 (unit 8a94, VMT DS:69ec): no picture, only the fill. 8a94:028c
	// loads images 371..376 (bath, shower under 4 / 4-6 / 6-8 / 8-10 / 10-12
	// minutes); 8a94:0440 makes them image buttons (frame 1, bevel 1, light
	// colour 16) at the origin + (85, 4), (137, 4), (189, 4), (85, 74),
	// (137, 74), (189, 74), commands 0x123..0x128 = choice 1..6, in a radio
	// group with nothing selected. OK (0x12a) ends the screen only once a
	// choice is made (+0x210, 8a94:00de); Cancel (0x129) sets +0x1f5.
	static const int kPos[6][2] = { { 85, 4 }, { 137, 4 }, { 189, 4 }, { 85, 74 }, { 137, 74 }, { 189, 74 } };
	Graphics::ManagedSurface *choice[6];
	for (int k = 0; k < 6; ++k)
		choice[k] = _archive.loadImage(kBathImage + k);
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *cancel = loadImageObject(_exe, kButtonCancelImage);

	setPalette(kPaletteB);
	_interface->setCursor(Interface::kCursorArrow);

	int chosen = 0;
	int result = -2; // -2 = still open
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (result == -2 && !shouldQuit()) {
		if (redraw) {
			redraw = false;
			_screen->fillRect(Common::Rect(0, 0, 320, 200), kScreenFill);
			for (int k = 0; k < 6; ++k) {
				if (choice[k])
					drawBevelButton(_screen, *choice[k], kScreenX + kPos[k][0], kScreenY + kPos[k][1], k + 1 == chosen, 1, 1, 16);
			}
			drawBevelButton(_screen, *ok, kOkX, kOkY, false);
			drawBevelButton(_screen, *cancel, kCancelX, kCancelY, false);
		}

		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				// 8a94:0136: Enter or Space = OK, Esc = cancel
				if (isOkKey(e)) {
					if (chosen)
						result = chosen;
				} else if (e.kbd.keycode == Common::KEYCODE_ESCAPE) {
					result = -1;
				}
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (buttonRect(*ok, kOkX, kOkY, 2).contains(e.mouse)) {
					if (trackButton(*ok, kOkX, kOkY) && chosen)
						result = chosen;
				} else if (buttonRect(*cancel, kCancelX, kCancelY, 2).contains(e.mouse)) {
					if (trackButton(*cancel, kCancelX, kCancelY))
						result = -1;
				} else {
					for (int k = 0; k < 6; ++k) {
						if (choice[k] && k + 1 != chosen && buttonRect(*choice[k], kScreenX + kPos[k][0],
								kScreenY + kPos[k][1], 2).contains(e.mouse)) {
							chosen = k + 1;
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

	for (int k = 0; k < 6; ++k)
		delete choice[k];
	delete ok;
	delete cancel;
	return result == -2 ? -1 : result;
}

// Pascal's Round (2c00:15dd): halves away from zero
static int pascalRound(double v) {
	return (int)(v >= 0 ? v + 0.5 : v - 0.5);
}

// A screen that waits for OK (132, y), Enter, Esc or Space (b1c3, b06f:0d1a)
static void waitForOk(MiljostrupEngine *vm, Graphics::Screen *screen, Graphics::ManagedSurface *ok, int okY) {
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!vm->shouldQuit()) {
		Common::Event e;
		bool done = false;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN && (e.kbd.keycode == Common::KEYCODE_RETURN ||
					e.kbd.keycode == Common::KEYCODE_KP_ENTER || e.kbd.keycode == Common::KEYCODE_SPACE ||
					e.kbd.keycode == Common::KEYCODE_ESCAPE))
				done = true;
			else if (e.type == Common::EVENT_LBUTTONDOWN && ok &&
					buttonRect(*ok, kOkX, okY).contains(e.mouse))
				done = vm->trackButton(*ok, kOkX, okY);
		}
		if (done)
			break;
		limiter.delayBeforeSwap();
		screen->update();
		limiter.startFrame();
	}
}

void MiljostrupEngine::runHelpScreen() {
	// Unit b1c3 (b1c3:03f4): picture 65 with palette A, the title
	// "Oversigt over funktionstasterne" at (37, 12) in colour 8 and (36, 11)
	// in colour 15, five key caps (images 66..70: F1, F2, F5, F6, F10) at
	// (15, 41 + 22 * k) with their lines in colour 189 / 187, OK (132, 185)
	static const struct { int x, y, color; uint32 text; } kLines[] = {
		{ 41, 46, 189, 0xb1c30122 }, { 41, 68, 189, 0xb1c30174 }, { 41, 90, 189, 0xb1c301c7 },
		{ 41, 107, 189, 0xb1c30222 }, { 41, 117, 189, 0xb1c30240 }, { 41, 130, 189, 0xb1c302c5 },
		{ 41, 140, 187, 0xb1c302db }, { 41, 150, 187, 0xb1c302f4 }, { 41, 160, 187, 0xb1c30311 }
	};
	Graphics::ManagedSurface *background = _archive.loadImage(65);
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);
	if (background)
		_screen->blitFrom(background->rawSurface(), Common::Point(0, 0));
	Common::String title = handlerText(0xb1c303d4);
	_font.drawString(_screen, title, 37, 12, 8);
	_font.drawString(_screen, title, 36, 11, 15);
	for (int k = 0; k < 5; ++k) {
		Graphics::ManagedSurface *key = _archive.loadImage(66 + k);
		if (key)
			_screen->transBlitFrom(*key, Common::Point(15, 41 + 22 * k), key->getTransparentColor());
		delete key;
	}
	for (uint i = 0; i < ARRAYSIZE(kLines); ++i)
		_font.drawString(_screen, handlerText(kLines[i].text), kLines[i].x, kLines[i].y, kLines[i].color);
	drawBevelButton(_screen, *ok, kOkX, 185, false);
	waitForOk(this, _screen, ok, 185);
	delete background;
	delete ok;
}

void MiljostrupEngine::runScoreScreen(bool wholeGame) {
	// b06f:0413 / 0be7: picture 43 (this location, F5) or 42 (the whole
	// game, F6) with palette A; on it the four bars (b06f:097f, 086e): rows
	// at y 41, 62, 83, 104 (11 + 30 ...), images 44 / 46 / 48 / 50 (points,
	// energy, waste, water; the next image for a negative value), length
	// Round(value / max * 100), at most +-100 (max 0: 100), from x 167 to
	// the right (negative: to the left, ending at 166); the number in digit
	// images 52..61 from x 294 leftwards at y + 4 and the sign (62 +, 63 -).
	// F5 only: the threshold as image 64 at x 166 + Round(threshold / max *
	// 100) on the points row while the points are below it. The title
	// "Samlet score " + the location (b06f:0a8f) or "Samlet score for hele
	// spillet", centred at y 11 (colour 8, x + 1) and y 10 (colour 15).
	// F6 adds the game's totals (b288:014f) to this location's values.
	const LocationTarget *target = getLocationTarget();
	int values[4], maxima[4];
	for (int k = 0; k < 4; ++k)
		values[k] = _score[k];
	maxima[0] = target ? target->maxPoints : 0;
	maxima[1] = target ? target->maxEnergy : 0;
	maxima[2] = target ? target->maxWaste : 0;
	maxima[3] = target ? target->maxWater : 0;
	if (wholeGame) {
		for (int k = 0; k < 4; ++k) {
			values[k] += _totals[2 * k];
			maxima[k] += _totals[2 * k + 1];
		}
	}

	Graphics::ManagedSurface *background = _archive.loadImage(wholeGame ? 42 : 43);
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);
	if (background)
		_screen->blitFrom(background->rawSurface(), Common::Point(0, 0));

	static const int kRows[4] = { 41, 62, 83, 104 };
	for (int k = 0; k < 4; ++k) {
		int value = values[k], max = maxima[k], y = kRows[k];
		int len;
		if (value > max)
			len = 100;
		else if (-max > value)
			len = -100;
		else if (max == 0)
			len = 100;
		else
			len = pascalRound((double)value / max * 100.0);
		if (len > 0) {
			Graphics::ManagedSurface *bar = _archive.loadImage(44 + 2 * k);
			if (bar)
				_screen->copyRectToSurface(bar->rawSurface(), 167, y, Common::Rect(0, 0, MIN<int>(len, bar->w), bar->h));
			delete bar;
		} else if (len < 0) {
			Graphics::ManagedSurface *bar = _archive.loadImage(45 + 2 * k);
			if (bar) {
				int from = MAX<int>(bar->w + len, 0);
				_screen->copyRectToSurface(bar->rawSurface(), 167 + len, y, Common::Rect(from, 0, bar->w, bar->h));
			}
			delete bar;
		}
		// b06f:0649: the digits from the right, then the sign
		int a = ABS(value), x = 294;
		do {
			Graphics::ManagedSurface *d = _archive.loadImage(52 + a % 10);
			if (d)
				_screen->transBlitFrom(*d, Common::Point(x, y + 4), d->getTransparentColor());
			delete d;
			a /= 10;
			x -= 6;
		} while (a);
		if (value != 0) {
			Graphics::ManagedSurface *s = _archive.loadImage(value > 0 ? 62 : 63);
			if (s)
				_screen->transBlitFrom(*s, Common::Point(x, y + 4), s->getTransparentColor());
			delete s;
		}
		// b06f:08ec: the threshold line
		if (k == 0 && !wholeGame && target && target->maxPoints && values[0] < target->threshold) {
			Graphics::ManagedSurface *line = _archive.loadImage(64);
			int lx = pascalRound((double)target->threshold / target->maxPoints * 100.0) + 166;
			if (line)
				_screen->copyRectToSurface(line->rawSurface(), lx, 41, Common::Rect(0, 0, line->w, MIN<int>(14, line->h)));
			delete line;
		}
	}

	// b06f:0a8f: the title
	Common::String title;
	if (wholeGame) {
		title = handlerText(0xb06f09f3);
	} else {
		static const uint32 kPlaces[8] = { 0xb06f0a11, 0xb06f0a1b, 0xb06f0a25, 0xb06f0a34, 0xb06f0a46,
			0xb06f0a54, 0xb06f0a61, 0xb06f0a71 };
		title = handlerText(0xb06f0a81) + handlerText(kPlaces[_phase >= 1 && _phase <= 7 ? _phase - 1 : 7]);
	}
	int tx = (320 - (int)title.size() * 8) / 2;
	_font.drawString(_screen, title, tx + 1, 11, 8);
	_font.drawString(_screen, title, tx, 10, 15);
	drawBevelButton(_screen, *ok, kOkX, kOkY, false);
	waitForOk(this, _screen, ok, kOkY);
	delete background;
	delete ok;
}

int MiljostrupEngine::runMenu() {
	// b06f:002e, drawn over the screen as it is (b06f:0329): a black frame
	// (37, 35)-(282, 135), the inside (38, 36)-(281, 134) in colour 7, an
	// engraved frame (21e0:0000) at (40, 38) 239 x 94 (colour 8, then 15 one
	// pixel lower right, colour 7 corners), "Jeg ønsker at" at (52, 45) in
	// colour 1; buttons (b06f:02c0) at (53, 59 / 77 / 95 / 113): restart the
	// game (4), replay the location (5), stop (6), continue (3). Esc
	// continues (b06f:01d6).
	static const int kCommands[4] = { kMenuRestartGame, kMenuReplay, kMenuStop, kMenuContinue };
	static const int kY[4] = { 59, 77, 95, 113 };
	Graphics::ManagedSurface *button[4];
	for (int k = 0; k < 4; ++k)
		button[k] = loadImageObject(_exe, kMenuButtonImages[k]);
	_interface->setCursor(Interface::kCursorArrow);
	_screen->frameRect(Common::Rect(37, 35, 283, 136), 0);
	_screen->fillRect(Common::Rect(38, 36, 282, 135), 7);
	_screen->frameRect(Common::Rect(40, 38, 40 + 239, 38 + 94), 8);
	_screen->frameRect(Common::Rect(41, 39, 41 + 239, 39 + 94), 15);
	_screen->setPixel(41, 38 + 94 - 1, 7);
	_screen->setPixel(40 + 239 - 1, 39, 7);
	_font.drawString(_screen, handlerText(0xb06f031b), 52, 45, 1);
	for (int k = 0; k < 4; ++k)
		drawBevelButton(_screen, *button[k], 53, kY[k], false);

	int result = 0;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!result && !shouldQuit()) {
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN && e.kbd.keycode == Common::KEYCODE_ESCAPE)
				result = kMenuContinue;
			else if (e.type == Common::EVENT_LBUTTONDOWN) {
				for (int k = 0; k < 4; ++k) {
					if (Common::Rect(53 - 2, kY[k] - 2, 53 + button[k]->w + 2, kY[k] + button[k]->h + 2).contains(e.mouse) &&
							trackButton(*button[k], 53, kY[k]))
						result = kCommands[k];
				}
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	for (int k = 0; k < 4; ++k)
		delete button[k];
	return result ? result : (int)kMenuContinue;
}

bool MiljostrupEngine::runFunctionKey(int key, bool endOfGame) {
	// ae72 (rooms) / 5042:00a6 (the end): GoTo(1 / 2 / 3 / 4, 0), the room
	// ends; the dispatcher's fallback b06f:0fa4 shows the screen with GoTo
	// back to the room before (+0x17). F5 does not exist at the end.
	switch (key) {
	case kKeyF1:
		runHelpScreen();
		return true;
	case kKeyF5:
		if (endOfGame)
			return false;
		runScoreScreen(false);
		return true;
	case kKeyF6:
		runScoreScreen(true);
		return true;
	case kKeyF10: {
		int action = runMenu();
		if (action != kMenuContinue)
			_menuAction = action;
		return true;
	}
	default:
		return false;
	}
}

int MiljostrupEngine::runTemperatureScreen(int value, int minValue, int maxValue) {
	// Unit 8000 ("Temperaturvalgs_Applikationer", the school's close-up 14):
	// 8000:0354: black strip (0,0)-(319,7), palette A, picture 594 at (0, 8),
	// title "Indstil termostaten:" at (80, 0) in colour 14. 8000:04f3: the
	// sign (606 minus, 607 plus) at (143, 105), the tens digit (595 + n) at
	// (150, 97), the ones digit at (164, 97). 8000:0433: down (EXE
	// 1db8:1a70) at (108, 99) command 0x3f, up (1db8:196e) at (196, 99)
	// command 0x3e, OK (132, 184) command 3. Up / down and the arrow keys
	// step within min..max with sound 1d26:0178; OK, Enter, Space and Esc
	// end and keep the value (VMT 0x3c = 8000:07aa).
	Graphics::ManagedSurface *background = _archive.loadImage(594);
	Graphics::ManagedSurface *digit[10];
	for (int k = 0; k < 10; ++k)
		digit[k] = _archive.loadImage(595 + k);
	Graphics::ManagedSurface *sign[2] = { _archive.loadImage(606), _archive.loadImage(607) };
	Graphics::ManagedSurface *up = loadImageObject(_exe, kButtonUpImage);
	Graphics::ManagedSurface *down = loadImageObject(_exe, kButtonDownImage);
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	static const int kDownX = 108, kUpX = 196, kArrowY = 99;

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);

	bool done = false, redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!done && !shouldQuit()) {
		if (redraw) {
			redraw = false;
			_screen->fillRect(Common::Rect(0, 0, 320, 8), 0);
			if (background)
				_screen->transBlitFrom(*background, Common::Point(0, 8), background->getTransparentColor());
			_font.drawString(_screen, handlerText(0x8000033f), 80, 0, 14);
			Graphics::ManagedSurface *s = sign[value < 0 ? 0 : 1];
			if (s)
				_screen->transBlitFrom(*s, Common::Point(143, 105), s->getTransparentColor());
			int a = ABS(value);
			if (digit[(a / 10) % 10])
				_screen->transBlitFrom(*digit[(a / 10) % 10], Common::Point(150, 97), digit[(a / 10) % 10]->getTransparentColor());
			if (digit[a % 10])
				_screen->transBlitFrom(*digit[a % 10], Common::Point(164, 97), digit[a % 10]->getTransparentColor());
			drawBevelButton(_screen, *down, kDownX, kArrowY, false);
			drawBevelButton(_screen, *up, kUpX, kArrowY, false);
			drawBevelButton(_screen, *ok, kOkX, kOkY, false);
		}

		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			int step = 0;
			if (e.type == Common::EVENT_KEYDOWN) {
				if (isOkKey(e) || e.kbd.keycode == Common::KEYCODE_ESCAPE)
					done = true;
				else if (e.kbd.keycode == Common::KEYCODE_UP)
					step = 1;
				else if (e.kbd.keycode == Common::KEYCODE_DOWN)
					step = -1;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (buttonRect(*ok, kOkX, kOkY, 2).contains(e.mouse))
					done = trackButton(*ok, kOkX, kOkY);
				else if (buttonRect(*up, kUpX, kArrowY, 2).contains(e.mouse))
					step = 1;
				else if (buttonRect(*down, kDownX, kArrowY, 2).contains(e.mouse))
					step = -1;
			}
			if ((step > 0 && value < maxValue) || (step < 0 && value > minValue)) {
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
	for (int k = 0; k < 10; ++k)
		delete digit[k];
	delete sign[0];
	delete sign[1];
	delete up;
	delete down;
	delete ok;
	return value;
}

int MiljostrupEngine::runChoiceScreen(uint16 picture, int pictureX, int pictureY, uint16 firstOption,
		const Common::String &title, bool hasOk, int initial) {
	// Unit 80ab ("Valg_Applikationer", the school's close-ups 12 and 13):
	// 80ab:01bf draws the frame: the title strip (VMT 0x48 = 80ab:0657,
	// black (0,0)-(319,7), text centred in colour 14) and (0,8)-(319,21) in
	// colour 8 (without a title (0,0)-(319,21)), a line in colour 19 at y 22,
	// the panel (161,23)-(319,162) in colour 20, a line at y 163,
	// (0,164)-(319,199) in colour 8. The room's picture (b3d8:0102) at its
	// place, drawn whole. 80ab:032d: the four options (80ab:015a, images
	// firstOption..+3) as image buttons, frame 1, bevel 2, at (166 + 78 *
	// col, 28 + 68 * row), commands 0x1f..0x22. With OK (+0x657): a radio
	// group with `initial` pressed, OK (132,184); a click chooses (80ab:04ae,
	// VMT 0x64), OK / Enter / Space / Esc end with the selection, the preset
	// when nothing was clicked (the user's check in the original,
	// 2026-11-03). Without OK a click chooses and ends. Returns the choice
	// 1..4, or 0 if none was clicked without OK.
	Graphics::ManagedSurface *background = _archive.loadImage(picture);
	Graphics::ManagedSurface *option[4];
	for (int k = 0; k < 4; ++k)
		option[k] = _archive.loadImage(firstOption + k);
	Graphics::ManagedSurface *ok = hasOk ? loadImageObject(_exe, kButtonOkImage) : nullptr;

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);

	int selected = hasOk ? initial : 0;
	int chosen = 0;
	bool done = false, redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!done && !shouldQuit()) {
		if (redraw) {
			redraw = false;
			if (title.empty()) {
				_screen->fillRect(Common::Rect(0, 0, 320, 22), 8);
			} else {
				drawScreenTitle(title);
				_screen->fillRect(Common::Rect(0, 8, 320, 22), 8);
			}
			_screen->hLine(0, 22, 319, 19);
			_screen->fillRect(Common::Rect(161, 23, 320, 163), 20);
			_screen->hLine(0, 163, 319, 19);
			_screen->fillRect(Common::Rect(0, 164, 320, 200), 8);
			if (background)
				_screen->copyRectToSurface(background->rawSurface(), pictureX, pictureY,
					Common::Rect(background->w, background->h));
			for (int k = 0; k < 4; ++k) {
				if (option[k])
					drawBevelButton(_screen, *option[k], 166 + 78 * (k % 2), 28 + 68 * (k / 2), k + 1 == selected, 1, 2);
			}
			if (ok)
				drawBevelButton(_screen, *ok, kOkX, kOkY, false);
		}

		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN && hasOk &&
					(isOkKey(e) || e.kbd.keycode == Common::KEYCODE_ESCAPE)) {
				done = true;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (ok && buttonRect(*ok, kOkX, kOkY, 2).contains(e.mouse)) {
					done = trackButton(*ok, kOkX, kOkY);
					continue;
				}
				for (int k = 0; k < 4; ++k) {
					if (option[k] && buttonRect(*option[k], 166 + 78 * (k % 2), 28 + 68 * (k / 2), 3).contains(e.mouse)) {
						chosen = selected = k + 1;
						redraw = true;
						if (!hasOk)
							done = true;
					}
				}
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	delete background;
	for (int k = 0; k < 4; ++k)
		delete option[k];
	delete ok;
	return hasOk ? selected : chosen;
}

bool MiljostrupEngine::runTVScreen(bool video, int &tv, int &vcr) {
	// Close-ups 18 / 19 (unit 8d9d, VMT DS:6ff0 / DS:704c): 8f88:0000 with
	// origin (0, 22) and palette A, picture 178. The TV's state (0 on, 1
	// stand-by, 2 off) as image 179 + state at the origin + (112, 10)
	// (8d9d:03de), the video's as 182 + state at + (132, 90) (8d9d:044a).
	// The used device's region (TV (112, 10)-(153, 30), video (132, 90)-
	// (200, 109), command 0x123) switches it on -> stand-by -> off -> on
	// with a click (8d9d:00da, 1d26:0155). Title (VMT 0x58 = 8d9d:0647 /
	// 086b): "Fjernsynet er " / "Videoen er " + state + ".". OK (0x124),
	// Enter, Space end; Cancel (0x125) and Esc set +0x1f5.
	static const uint32 kStateTexts[2][3] = {
		{ 0x8d9d061c, 0x8d9d0622, 0x8d9d062e },
		{ 0x8d9d0843, 0x8d9d0849, 0x8d9d0855 }
	};
	Graphics::ManagedSurface *background = _archive.loadImage(kTVPicture);
	Graphics::ManagedSurface *tvImage[3], *vcrImage[3];
	for (int k = 0; k < 3; ++k) {
		tvImage[k] = _archive.loadImage(kTVPicture + 1 + k);
		vcrImage[k] = _archive.loadImage(kTVPicture + 4 + k);
	}
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *cancel = loadImageObject(_exe, kButtonCancelImage);
	const Common::Rect region = video ?
		Common::Rect(kScreenX + 132, kScreenY + 90, kScreenX + 201, kScreenY + 110) :
		Common::Rect(kScreenX + 112, kScreenY + 10, kScreenX + 154, kScreenY + 31);

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);

	int &state = video ? vcr : tv;
	state = CLIP(state, 0, 2);
	tv = CLIP(tv, 0, 2);
	vcr = CLIP(vcr, 0, 2);
	int result = -2; // -2 = still open, -1 cancelled, 1 OK
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (result == -2 && !shouldQuit()) {
		if (redraw) {
			redraw = false;
			_screen->fillRect(Common::Rect(0, 0, 320, 200), kScreenFill);
			if (background)
				_screen->transBlitFrom(*background, Common::Point(kScreenX, kScreenY), background->getTransparentColor());
			if (tvImage[tv])
				_screen->transBlitFrom(*tvImage[tv], Common::Point(kScreenX + 112, kScreenY + 10), tvImage[tv]->getTransparentColor());
			if (vcrImage[vcr])
				_screen->transBlitFrom(*vcrImage[vcr], Common::Point(kScreenX + 132, kScreenY + 90), vcrImage[vcr]->getTransparentColor());
			drawBevelButton(_screen, *ok, kOkX, kOkY, false);
			drawBevelButton(_screen, *cancel, kCancelX, kCancelY, false);
			drawScreenTitle(handlerText(video ? 0x8d9d085d : 0x8d9d0636) + handlerText(kStateTexts[video][state]) +
				handlerText(video ? 0x8d9d0869 : 0x8d9d0645));
		}

		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				if (isOkKey(e))
					result = 1;
				else if (e.kbd.keycode == Common::KEYCODE_ESCAPE)
					result = -1;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (buttonRect(*ok, kOkX, kOkY, 2).contains(e.mouse)) {
					if (trackButton(*ok, kOkX, kOkY))
						result = 1;
				} else if (buttonRect(*cancel, kCancelX, kCancelY, 2).contains(e.mouse)) {
					if (trackButton(*cancel, kCancelX, kCancelY))
						result = -1;
				} else if (region.contains(e.mouse)) {
					state = (state + 1) % 3;
					playSound(kSoundClick);
					redraw = true;
				}
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	delete background;
	for (int k = 0; k < 3; ++k) {
		delete tvImage[k];
		delete vcrImage[k];
	}
	delete ok;
	delete cancel;
	return result == 1;
}

void MiljostrupEngine::runInfoScreen(uint16 picture, const uint16 *images, const Common::Point *pos, int count) {
	// Close-ups 27 (EXE unit 128e, VMT DS:69a0) and 28 (1268, DS:6954):
	// 8f88:0000 with origin (0, 0) and palette B, the picture (VMT 0x40),
	// then OK (command 0x123, the usual OK button at (132, 184)) and for 27
	// the extra images (VMT 0x3c). OK, Enter, Space and Esc close it.
	Graphics::ManagedSurface *background = _archive.loadImage(picture);
	Common::Array<Graphics::ManagedSurface *> extra;
	for (int k = 0; k < count; ++k)
		extra.push_back(_archive.loadImage(images[k]));
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);

	setPalette(kPaletteB);
	_interface->setCursor(Interface::kCursorArrow);
	_screen->fillRect(Common::Rect(0, 0, 320, 200), kScreenFill);
	if (background)
		_screen->transBlitFrom(*background, Common::Point(0, 0), background->getTransparentColor());
	drawBevelButton(_screen, *ok, kOkX, kOkY, false);
	for (int k = 0; k < count; ++k) {
		if (extra[k])
			_screen->transBlitFrom(*extra[k], pos[k], extra[k]->getTransparentColor());
	}

	bool done = false;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!done && !shouldQuit()) {
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN && (isOkKey(e) || e.kbd.keycode == Common::KEYCODE_ESCAPE))
				done = true;
			else if (e.type == Common::EVENT_LBUTTONDOWN && buttonRect(*ok, kOkX, kOkY, 2).contains(e.mouse))
				done = trackButton(*ok, kOkX, kOkY);
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	delete background;
	for (uint k = 0; k < extra.size(); ++k)
		delete extra[k];
	delete ok;
}

void MiljostrupEngine::runPowderBox(const int hardness[6]) {
	// Close-up 27 (128e:03dc): picture 393, the six towns' water hardness
	// (powder +0x2f..+0x34, 0 hard, 1 medium, 2 soft) as images 395 + value
	// at (160, 70 + 10k) (128e:027e)
	uint16 images[6];
	Common::Point pos[6];
	for (int k = 0; k < 6; ++k) {
		images[k] = kHardnessImage + CLIP(hardness[k], 0, 2);
		pos[k] = Common::Point(160, 70 + 10 * k);
	}
	runInfoScreen(kPowderPicture, images, pos, 6);
}

void MiljostrupEngine::runSchedule() {
	// Close-up 28 (1268:0203): the dosing chart, picture 394, and OK
	runInfoScreen(kSchedulePicture, nullptr, nullptr, 0);
}

int MiljostrupEngine::runDosing() {
	// Close-up 29 (EXE unit 11f8, VMT DS:68f8): picture 390 at (0, 0),
	// palette B. The value +0x330 (0..24, starts at 0) is shown by images
	// 401 + v at (95, 100), 426 + v at (170, 15) and 451 + v at (275, 13)
	// (VMT 0x58 = 11f8:0557) and the marker 400 at (240, (24 - v) * 6 + 31)
	// (VMT 0x50; VMT 0x54 draws 399 over its old place). 25 regions at
	// (239, 31 + 6i)-(251, 36 + 6i), commands 0x13c - i, set v; Up/Down add
	// or take one. OK (own image 398 at (181, 165), command 0x123), Enter or
	// Space end it; there is no cancel. Returns the dose, v * 10 (VMT 0x4c).
	Graphics::ManagedSurface *background = _archive.loadImage(kDosingPicture);
	Graphics::ManagedSurface *ok = _archive.loadImage(kDosingOkImage);
	Graphics::ManagedSurface *marker = _archive.loadImage(kDosingMarkerImage);
	Graphics::ManagedSurface *display[3][kDoseSteps + 1];
	static const uint16 kDisplayFirst[3] = { 401, 426, 451 };
	static const int kDisplayPos[3][2] = { { 95, 100 }, { 170, 15 }, { 275, 13 } };
	for (int d = 0; d < 3; ++d) {
		for (int v = 0; v <= kDoseSteps; ++v)
			display[d][v] = _archive.loadImage(kDisplayFirst[d] + v);
	}

	setPalette(kPaletteB);
	_interface->setCursor(Interface::kCursorArrow);

	int value = 0;
	bool done = false;
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!done && !shouldQuit()) {
		if (redraw) {
			redraw = false;
			_screen->fillRect(Common::Rect(0, 0, 320, 200), kScreenFill);
			if (background)
				_screen->transBlitFrom(*background, Common::Point(0, 0), background->getTransparentColor());
			if (marker)
				_screen->transBlitFrom(*marker, Common::Point(240, (kDoseSteps - value) * 6 + 31), marker->getTransparentColor());
			for (int d = 0; d < 3; ++d) {
				if (Graphics::ManagedSurface *img = display[d][value])
					_screen->transBlitFrom(*img, Common::Point(kDisplayPos[d][0], kDisplayPos[d][1]), img->getTransparentColor());
			}
			if (ok)
				drawBevelButton(_screen, *ok, kDosingOkX, kDosingOkY, false);
		}

		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				if (isOkKey(e)) {
					done = true;
				} else if (e.kbd.keycode == Common::KEYCODE_UP && value < kDoseSteps) {
					++value;
					redraw = true;
				} else if (e.kbd.keycode == Common::KEYCODE_DOWN && value > 0) {
					--value;
					redraw = true;
				}
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (ok && buttonRect(*ok, kDosingOkX, kDosingOkY, 2).contains(e.mouse)) {
					done = trackButton(*ok, kDosingOkX, kDosingOkY);
				} else if (e.mouse.x >= 239 && e.mouse.x <= 251 && e.mouse.y >= 31) {
					// the region for value v spans y (24 - v) * 6 + 31 .. + 5
					int i = (e.mouse.y - 31) / 6;
					if (i <= kDoseSteps && e.mouse.y <= 36 + 6 * i && value != kDoseSteps - i) {
						value = kDoseSteps - i;
						redraw = true;
					}
				}
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	delete background;
	delete ok;
	delete marker;
	for (int d = 0; d < 3; ++d) {
		for (int v = 0; v <= kDoseSteps; ++v)
			delete display[d][v];
	}
	return value * 10;
}

bool MiljostrupEngine::runWashProgram(int &program, int &temperature) {
	// Close-up 30 (unit 8a01, VMT DS:68a0): 8f88:0000 with origin (0, 0) and
	// palette A (0d58:0000), picture 476. Temperature +0x21d starts at 40,
	// program +0x21c at 2 (none), chosen +0x223 at 0 (8a01:0000).
	// - Programs: a radio group (117, 56)-(182, 109) of image buttons
	//   without frame or bevel (8a01:058e): command 0x123 = program 0 "uden
	//   forvask", images 479 / 480 (pressed) at (114, 58); 0x124 = program 1
	//   "med forvask", 477 / 478 at (114, 73). Choosing one: sound 1d26:0178
	//   (8a01:0104).
	// - The dial (8a01:07fc): image 483 / 484 / 485 for 40 / 60 / 90 degrees
	//   at (35, 65). Regions (12, 74)-(45, 107) command 0x125 (40), (46, 53)-
	//   (74, 73) 0x126 (60), (75, 75)-(110, 107) 0x127 (90) turn it one step
	//   towards that temperature (8a01:0139); a change clicks (1d26:0155).
	// - START 481 at (129, 88) command 0x129 and FORTRYD 482 at (129, 105)
	//   command 0x128, frame 1, bevel 1, corner colour 18. START, Enter or
	//   Space end it once a program is chosen; FORTRYD or Esc cancel.
	// Returns false when cancelled; program and temperature either way.
	static const uint16 kImages[] = { 476, 477, 478, 479, 480, 481, 482, 483, 484, 485 };
	Graphics::ManagedSurface *img[ARRAYSIZE(kImages)];
	for (uint k = 0; k < ARRAYSIZE(kImages); ++k)
		img[k] = _archive.loadImage(kImages[k]);
	Graphics::ManagedSurface *background = img[0];
	Graphics::ManagedSurface *programImage[2][2] = { { img[3], img[4] }, { img[1], img[2] } };
	Graphics::ManagedSurface *start = img[5], *cancel = img[6];
	static const int kProgramY[2] = { 58, 73 };
	static const int kStartX = 129, kStartY = 88, kCancelButtonY = 105;
	static const Common::Rect kDialRegion[3] = {
		Common::Rect(12, 74, 46, 108), Common::Rect(46, 53, 75, 74), Common::Rect(75, 75, 111, 108)
	};
	static const int kTemperatures[3] = { 40, 60, 90 };

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);

	program = 2;
	temperature = 40;
	bool chosen = false;
	int result = -2; // -2 = still open, -1 cancelled, 1 started
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (result == -2 && !shouldQuit()) {
		if (redraw) {
			redraw = false;
			_screen->fillRect(Common::Rect(0, 0, 320, 200), kScreenFill);
			if (background)
				_screen->transBlitFrom(*background, Common::Point(0, 0), background->getTransparentColor());
			int dial = temperature == 90 ? 9 : temperature == 60 ? 8 : 7;
			if (img[dial])
				_screen->transBlitFrom(*img[dial], Common::Point(35, 65), img[dial]->getTransparentColor());
			if (start)
				drawBevelButton(_screen, *start, kStartX, kStartY, false, 1, 1, 15, 18);
			if (cancel)
				drawBevelButton(_screen, *cancel, kStartX, kCancelButtonY, false, 1, 1, 15, 18);
			for (int p = 0; p < 2; ++p) {
				if (Graphics::ManagedSurface *b = programImage[p][p == program ? 1 : 0])
					_screen->transBlitFrom(*b, Common::Point(114, kProgramY[p]), b->getTransparentColor());
			}
		}

		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				if (isOkKey(e)) {
					if (chosen)
						result = 1;
				} else if (e.kbd.keycode == Common::KEYCODE_ESCAPE) {
					result = -1;
				}
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (start && buttonRect(*start, kStartX, kStartY, 2).contains(e.mouse)) {
					if (trackButton(*start, kStartX, kStartY, 1, 1, 15, 18) && chosen)
						result = 1;
				} else if (cancel && buttonRect(*cancel, kStartX, kCancelButtonY, 2).contains(e.mouse)) {
					if (trackButton(*cancel, kStartX, kCancelButtonY, 1, 1, 15, 18))
						result = -1;
				} else {
					for (int p = 0; p < 2; ++p) {
						Graphics::ManagedSurface *b = programImage[p][0];
						if (b && p != program && buttonRect(*b, 114, kProgramY[p], 0).contains(e.mouse)) {
							program = p;
							chosen = true;
							playSound(kSoundClicks);
							redraw = true;
						}
					}
					for (int r = 0; r < 3; ++r) {
						if (!kDialRegion[r].contains(e.mouse))
							continue;
						// 8a01:0139: one step towards the clicked temperature
						int old = temperature;
						if (kTemperatures[r] > temperature)
							temperature = temperature == 40 ? 60 : 90;
						else if (kTemperatures[r] < temperature)
							temperature = temperature == 90 ? 60 : 40;
						redraw = true;
						if (temperature != old)
							playSound(kSoundClick);
					}
				}
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	for (uint k = 0; k < ARRAYSIZE(kImages); ++k)
		delete img[k];
	return result == 1;
}

int MiljostrupEngine::runThermostatDisplay(uint16 picture, int value) {
	// 8f1b:0000: base 8f88:0000 at (0, 22), palette A, the picture; a number
	// display (9000:01a7) for -25..20 at the origin; buttons (frame and
	// bevel 0: a normal and a pressed image) down 0x125 at origin + (16, 6),
	// up 0x126 at origin + (234, 6); OK 0x124, Cancel 0x123
	enum { kMin = -25, kMax = 20, kDigit = 238, kDigitOff = 248, kMinusOff = 249, kMinusOn = 250,
		kDownImage = 251, kUpImage = 253 };
	Graphics::ManagedSurface *background = _archive.loadImage(picture);
	Graphics::ManagedSurface *digits[10];
	for (int k = 0; k < 10; ++k)
		digits[k] = _archive.loadImage(kDigit + k);
	Graphics::ManagedSurface *digitOff = _archive.loadImage(kDigitOff);
	Graphics::ManagedSurface *minus[2] = { _archive.loadImage(kMinusOff), _archive.loadImage(kMinusOn) };
	Graphics::ManagedSurface *down[2] = { _archive.loadImage(kDownImage), _archive.loadImage(kDownImage + 1) };
	Graphics::ManagedSurface *up[2] = { _archive.loadImage(kUpImage), _archive.loadImage(kUpImage + 1) };
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *cancel = loadImageObject(_exe, kButtonCancelImage);
	Common::String title = readPascalString(_ovr, kTextThermostatTitle);
	const Common::Point downPos(kScreenX + 16, kScreenY + 6), upPos(kScreenX + 234, kScreenY + 6);

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);

	int result = -1000, pressed = 0; // pressed: -1 down, 1 up
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	auto blit = [this](Graphics::ManagedSurface *image, int x, int y) {
		if (image)
			_screen->transBlitFrom(*image, Common::Point(x, y), image->getTransparentColor());
	};
	while (result == -1000 && !shouldQuit()) {
		if (redraw) {
			redraw = false;
			_screen->fillRect(Common::Rect(0, 0, 320, 200), kScreenFill);
			blit(background, kScreenX, kScreenY);
			// 9000:0367: the sign (lit for negative values), the tens (off
			// when 0) and the ones, at the origin + (100, 27), (121, 10), (150, 10)
			int v = ABS(value);
			blit(minus[value < 0 ? 1 : 0], kScreenX + 100, kScreenY + 27);
			blit((v / 10) % 10 ? digits[(v / 10) % 10] : digitOff, kScreenX + 121, kScreenY + 10);
			blit(digits[v % 10], kScreenX + 150, kScreenY + 10);
			blit(down[pressed == -1 ? 1 : 0], downPos.x, downPos.y);
			blit(up[pressed == 1 ? 1 : 0], upPos.x, upPos.y);
			drawBevelButton(_screen, *ok, kOkX, kOkY, false);
			drawBevelButton(_screen, *cancel, kCancelX, kCancelY, false);
			drawScreenTitle(title);
		}

		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				if (isOkKey(e))
					result = value;
				else if (e.kbd.keycode == Common::KEYCODE_ESCAPE)
					result = -999;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (buttonRect(*ok, kOkX, kOkY, 2).contains(e.mouse)) {
					if (trackButton(*ok, kOkX, kOkY))
						result = value;  // 8f1b:01ad
				} else if (buttonRect(*cancel, kCancelX, kCancelY, 2).contains(e.mouse)) {
					if (trackButton(*cancel, kCancelX, kCancelY))
						result = -999;   // 8f1b:0177: +0x1f5 := 1
				} else if (down[0] && Common::Rect(downPos.x, downPos.y, downPos.x + down[0]->w,
						downPos.y + down[0]->h).contains(e.mouse)) {
					// 8f1b:00b5: the click only when it can still go down
					if (value > kMin) {
						playSound(kSoundClicks);
						--value;
					}
					pressed = -1;
					redraw = true;
				} else if (up[0] && Common::Rect(upPos.x, upPos.y, upPos.x + up[0]->w,
						upPos.y + up[0]->h).contains(e.mouse)) {
					if (value < kMax) { // 8f1b:0116
						playSound(kSoundClicks);
						++value;
					}
					pressed = 1;
					redraw = true;
				}
			} else if (e.type == Common::EVENT_LBUTTONUP && pressed) {
				pressed = 0;
				redraw = true;
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	delete background;
	for (int k = 0; k < 10; ++k)
		delete digits[k];
	delete digitOff;
	for (int k = 0; k < 2; ++k) {
		delete minus[k];
		delete down[k];
		delete up[k];
	}
	delete ok;
	delete cancel;
	return result;
}

int MiljostrupEngine::runPotatoPeeling() {
	// 8ebc:056a: palette A, origin (0, 0), no picture; three animated
	// pictures (8f88:0378) with frame buttons around them (12fe:013e)
	static const uint16 kImages[7] = { 256, 257, 258, 259, 260, 261, 262 }; // DS:70f8
	struct Animation {
		int x, y;
		int count;
		byte frames[6]; // 1-based into kImages (DS:710e, 710c, 7106)
	};
	static const Animation kAnimations[3] = {
		{ 23, 25, 2, { 7, 6 } },               // choice 0: under running water
		{ 121, 25, 2, { 5, 4 } },              // choice 1: in a bowl
		{ 219, 25, 6, { 2, 1, 2, 1, 3, 3 } }   // choice 2: peel, then rinse
	};
	static const Common::Rect kButtons[3] = {
		Common::Rect(23, 25, 98, 90), Common::Rect(121, 25, 196, 90), Common::Rect(219, 25, 294, 90)
	};

	Graphics::ManagedSurface *images[7];
	for (int k = 0; k < 7; ++k)
		images[k] = _archive.loadImage(kImages[k]);
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *cancel = loadImageObject(_exe, kButtonCancelImage);
	Common::String title = readPascalString(_ovr, kTextPotatoTitle);

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);

	int frame[3] = { 0, 0, 0 };
	int choice = 0, result = -2;
	int countdown = 1; // DS:7200: every 15th message 0x41 advances the animations
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (result == -2 && !shouldQuit()) {
		if (--countdown == 0) {
			// HYPOTHESIS: one message 0x41 per frame
			countdown = 15;
			for (int k = 0; k < 3; ++k)
				frame[k] = (frame[k] + 1) % kAnimations[k].count; // 8f88:03d9
			redraw = true;
		}
		if (redraw) {
			redraw = false;
			_screen->fillRect(Common::Rect(0, 0, 320, 200), kScreenFill);
			for (int k = 0; k < 3; ++k) {
				Graphics::ManagedSurface *image = images[kAnimations[k].frames[frame[k]] - 1];
				if (image)
					_screen->transBlitFrom(*image, Common::Point(kAnimations[k].x, kAnimations[k].y), image->getTransparentColor());
				// 12fe:018f: rings 4..6 outside the button, colour 4 when chosen
				// (+0x17), else the background colour 26 (+0x18)
				const Common::Rect &r = kButtons[k];
				for (int i = 4; i < 7; ++i)
					_screen->frameRect(Common::Rect(r.left - i, r.top - i, r.right + i + 1, r.bottom + i + 1),
						k == choice ? 4 : kScreenFill);
			}
			drawBevelButton(_screen, *ok, kOkX, kOkY, false);
			drawBevelButton(_screen, *cancel, kCancelX, kCancelY, false);
			drawScreenTitle(title);
		}

		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				if (isOkKey(e))
					result = choice;
				else if (e.kbd.keycode == Common::KEYCODE_ESCAPE)
					result = -1;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (buttonRect(*ok, kOkX, kOkY, 2).contains(e.mouse)) {
					if (trackButton(*ok, kOkX, kOkY))
						result = choice; // command 0x124 (8ebc:00da)
				} else if (buttonRect(*cancel, kCancelX, kCancelY, 2).contains(e.mouse)) {
					if (trackButton(*cancel, kCancelX, kCancelY))
						result = -1;     // command 0x123 (8ebc:00a4)
				} else {
					for (int k = 0; k < 3; ++k) {
						const Common::Rect &r = kButtons[k];
						if (e.mouse.x >= r.left && e.mouse.x <= r.right && e.mouse.y >= r.top &&
								e.mouse.y <= r.bottom && k != choice) {
							choice = k; // 8ebc:007a: +0x234 := k
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

	for (int k = 0; k < 7; ++k)
		delete images[k];
	delete ok;
	delete cancel;
	return result == -2 ? -1 : result;
}

} // End of namespace Miljostrup
