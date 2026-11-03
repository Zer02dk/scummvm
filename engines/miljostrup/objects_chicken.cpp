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

// The chicken farm (location 4), written from notes/chickenfarm.md. So far
// the run (room 7: feed belt, door, chickens) and the chicken house's water
// (room 8: the dispenser switch, the hole in the pipe and the tape); the
// panels' start values and the chickens' well-being.

#include "common/events.h"
#include "common/system.h"
#include "graphics/framelimiter.h"

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/response_data.h"

namespace Miljostrup {

enum {
	kChickenFarm = 4,
	kRun = 7,
	kHouse = 8,
	kFeedBelt = 115,        // DS:c6d4, +0x2f on (start off)
	kBeltFigure = 116,      // the belt's figure, 0b20
	kRunDoor = 117,         // DS:c6dc, +0x2f open (start closed)
	kChickens = 118,
	kPipeHole = 121,        // DS:c6ec, +0x2f patched
	kTape = 122,
	kWaterSwitch = 123,     // DS:c6f4, +0x2f on (start on)
	kCalendar = 124,        // DS:c6f8, +0x2f the chickens' age in days
	kHumidityPanel = 126,   // DS:c700, +0x31 ventilation on, +0x32 humidity (%)
	kTemperaturePanel = 129, // DS:c70c, +0x31 temperature (degrees)
	kLightPanel = 130,      // DS:c710, +0x31 light on, +0x32 automatic
	kLorry = 105,
	kRoom9Door = 103,       // DS:c6a0, the yard's door to the dung room
	kDeadContainer = 107,
	kWasteContainer = 108,
	kField = 109,
	kCompost = 110,
	kBarrowOutside = 111,   // the barrow by the field (room 6)
	kDeadChickens = 132,
	kDung = 133,            // DS:c720, +0x2f cleared away
	kBarrow = 134,          // DS:c72c, the barrow in the dung room: +0x2f there, +0x30 full
	kTankDrain = 135,       // DS:c724, +0x2f open
	kStreamDrain = 136,     // DS:c728
	kFloor = 137,           // DS:c730, +0x2f clean
	kHose = 138,            // DS:c734, +0x2f hung up (start 1)
	kDeadChickensProxy = 139,
	kDungRoom = 9,
	kHosingState = 12       // the player hosing (69d2:0557)
};

// 7000:1813 / 1890 (humidity) and 1e64 / 1ee1 (temperature): the right
// range by the chickens' age (the calendar): under 8, 8..14, 15..21,
// 22..28, 29..35, 36..42 days
static int ageBand(int age) {
	if (age < 8)
		return 0;
	return MIN((age - 1) / 7, 5);
}

static const int kHumidityMin[6] = { 50, 55, 60, 66, 74, 75 };
static const int kHumidityMax[6] = { 55, 60, 66, 74, 75, 75 };
static const int kTemperatureMin[6] = { 31, 29, 26, 24, 22, 21 };
static const int kTemperatureMax[6] = { 35, 31, 29, 26, 24, 22 };

void MiljostrupEngine::initChickenFarm() {
	// 7000:11c0: the age, Random(26) + 17
	setFlag(kCalendar, getRandomNumber(25) + 17);
	int band = ageBand(getFlag(kCalendar));
	// 7000:1316: ventilation on, the humidity set wrong: below the range
	// from 50..59 -> Random(26) + 60, 60..69 -> Random(10) + 50, 70..79 ->
	// Random(20) + 50
	int low = kHumidityMin[band];
	int humidity = low < 60 ? getRandomNumber(25) + 60 : (low < 70 ? getRandomNumber(9) + 50 : getRandomNumber(19) + 50);
	setField(kHumidityPanel, 0x31, 1);
	setField(kHumidityPanel, 0x32, humidity);
	// 7000:199d: the temperature set wrong: by the range's minimum 15..24 ->
	// Random(13) + 27, 25..29 -> Random(8) + 32, 30..39 -> Random(14) + 15
	low = kTemperatureMin[band];
	int temperature = low < 25 ? getRandomNumber(12) + 27 : (low < 30 ? getRandomNumber(7) + 32 : getRandomNumber(13) + 15);
	setField(kTemperaturePanel, 0x31, temperature);
	// 7000:1f6b: the light on, not automatic
	setField(kLightPanel, 0x31, 1);
	setField(kLightPanel, 0x32, 0);
	// 7000:259e: the water dispenser on
	setFlag(kWaterSwitch, 1);
	// 7000:3c98: the hose hung up
	setFlag(kHose, 1);
	_hosing = false;
}

bool MiljostrupEngine::isChickenFarmObjectEnabled(int object, bool &enabled) const {
	// the barrows' regions go with their figures (06e2 / 3a80), the dung's
	// (355d) while it lies there
	switch (object) {
	case kBarrowOutside:
		enabled = getFlag(kBarrow) == 0;
		return true;
	case kBarrow:
		enabled = getFlag(kBarrow) != 0;
		return true;
	case kDung:
		enabled = getFlag(kDung) == 0;
		return true;
	default:
		return false;
	}
}

void MiljostrupEngine::emptyBarrow(int value, uint32 text) {
	// 7000:0106: Award(value, 0, value, 0), the barrow empty again
	award(value, 0, value, 0, text, 0);
	setField(kBarrow, 0x30, 0);
	refreshObject(kBarrowOutside);
}

void MiljostrupEngine::dropDeadChickens(int value, uint32 text) {
	// 7000:2d8a: Award(value, 0, value, 0) with page "36", out of the inventory
	award(value, 0, value, 0, text, 0x70002d87);
	removeObject(kDeadChickens);
}

void MiljostrupEngine::moveBarrow(bool toDungRoom) {
	// 7000:063a / 39dd: the barrow and the player go to the other room; the
	// yard's door to the dung room is open (+0x33) afterwards
	setFlag(kBarrow, toDungRoom ? 1 : 0);
	_doors[kRoom9Door] = true;
	gotoRoom(toDungRoom ? kDungRoom : 6, toDungRoom ? 5 : 1);
}

void MiljostrupEngine::tickChickenFarm() {
	// 69d2:0011: the player hoses for 50 steps (state 12), message 0x20 ->
	// 032b: the hose hung up again, the player stands, input on, the floor
	// clean, the award 0245 (the water into the stream -10, into the tank +10)
	if (_location != kChickenFarm || !_hosing || _scene->isWalking())
		return;
	_hosing = false;
	setFlag(kHose, 1);
	refreshObject(kHose);
	_scene->setPlayerState(1);
	setInputOff(false);
	setFlag(kFloor, 1);
	if (getFlag(kStreamDrain)) {
		award(-10, 0, 0, -10, 0x69d2008b, 0x69d20165);
		// "Hvem bruger mest vand?" Industrien / Landbruget: 0, category 3
		setQuiz(0x69d20168, 0x69d2017f, 0x69d2018a, 0, 3);
	} else {
		award(10, 0, 0, 10, 0x69d20195, 0);
	}
}

bool MiljostrupEngine::isHumidityRight() const {
	// 7000:190d: ventilation on and the humidity in the range
	int band = ageBand(getFlag(kCalendar));
	int h = getField(kHumidityPanel, 0x32);
	return getField(kHumidityPanel, 0x31) && h >= kHumidityMin[band] && h <= kHumidityMax[band];
}

bool MiljostrupEngine::isTemperatureRight() const {
	// 7000:1e2b
	int band = ageBand(getFlag(kCalendar));
	int t = getField(kTemperaturePanel, 0x31);
	return t >= kTemperatureMin[band] && t <= kTemperatureMax[band];
}

bool MiljostrupEngine::areChickensWell() const {
	// 7000:1119: the door closed, the feed belt on, the hole patched, the
	// water on, and the three panels right (light on and automatic, 2569)
	return !getFlag(kRunDoor) && getFlag(kFeedBelt) && getFlag(kPipeHole) && getFlag(kWaterSwitch) &&
		isHumidityRight() && isTemperatureRight() && getField(kLightPanel, 0x31) && getField(kLightPanel, 0x32);
}

int MiljostrupEngine::getChickenDimFactor() const {
	// 7000:0eef (the door's rule): the run is dark while the door is closed
	// and the light is off (the light panel's +0x31); the room inserts the
	// interval (16, 240, 0, 1, 1) with factor 4 (6b06:072a / 073c)
	// The chicken house: dark while the light is off (the light panel's rule
	// 7000:20ac; the room's interval (16, 240, 0, 1, 1), factor 4, 6a6d:07e0)
	if (_location != kChickenFarm)
		return 0;
	if (_room == kHouse)
		return getField(kLightPanel, 0x31) ? 0 : 4;
	if (_room != kRun)
		return 0;
	return !getField(kLightPanel, 0x31) && !getFlag(kRunDoor) ? 4 : 0;
}

void MiljostrupEngine::chickenFarmBuilt() {
	if (_location != kChickenFarm || (_room != kRun && _room != kHouse))
		return;
	// The panels' rules (7000:14b7 / 1bde / 20ac, bound in the chicken
	// house): a setting from the close-up (+0x2f) is done (+0x30) and its
	// award follows (VMT 0x5c)
	if (_room == kHouse) {
		// (the light panel's rule also sets the house's dim, getChickenDimFactor)
		static const int kPanels[3] = { kHumidityPanel, kTemperaturePanel, kLightPanel };
		for (int i = 0; i < 3; ++i) {
			if (getFlag(kPanels[i])) {
				setFlag(kPanels[i], 0);
				setField(kPanels[i], 0x30, 1);
				panelAward(kPanels[i]);
			}
		}
	}
	// 6b06:0411 / 6a6d:0489: the room's five chickens are made at the build,
	// lively ones (at random places, Random(200) + 37, Random(5) + 102) when
	// the chickens are well, else the others at fixed places; the run's are
	// 911..920, the house's 921..930
	int first = _room == kRun ? 911 : 921;
	bool well = areChickensWell();
	for (int k = 0; k < 10; ++k)
		_scene->setFigureVisible(first + k, (k < 5) == well);
	if (well) {
		for (int k = 0; k < 5; ++k)
			_scene->setFigurePos(first + k, getRandomNumber(199) + 37, getRandomNumber(4) + 102);
	}
}

void MiljostrupEngine::panelAward(int panel) {
	switch (panel) {
	case kHumidityPanel: // 7000:176e
		if (!getField(panel, 0x31))
			award(-10, 0, -10, 0, 0x7000167c, 0);
		else if (isHumidityRight())
			award(10, 0, 10, 0, 0x700014db, 0);
		else
			award(-5, 0, -5, 0, 0x7000159d, 0);
		break;
	case kTemperaturePanel: // 1d95
		if (isTemperatureRight())
			award(10, 0, 10, 0, 0x70001c02, 0x70001d92);
		else
			award(-10, 0, -10, 0, 0x70001cc3, 0x70001d92);
		break;
	case kLightPanel: // 24db: "Hvad er 1 kilowatt?" 500 watt / 1000 watt: 0
		if (!getField(panel, 0x31)) {
			award(-10, 0, -10, 0, 0x700023aa, 0);
			setQuiz(0x7000243c, 0x70002450, 0x70002459, 0, 2);
		} else if (!getField(panel, 0x32)) {
			award(-10, -10, 0, 0, 0x70002212, 0);
			setQuiz(0x7000230b, 0x7000231f, 0x70002328, 0, 1);
		} else {
			award(10, 10, 0, 0, 0x700020ec, 0);
		}
		break;
	default:
		break;
	}
}

// The close-ups' common parts: the picture over the whole screen (palette
// A, 2472:0000), image buttons (28c3:02dd) with OK at (132, 184) and Cancel
// at (259, 184); Enter / Space are OK, Esc is Cancel
static const int kPanelOkX = 132, kPanelOkY = 184, kPanelCancelX = 259, kPanelCancelY = 184;

void MiljostrupEngine::runPictureScreen(uint16 picture, int number) {
	// The calendar (6994, picture 979: the age as digits 980 + d at (142,
	// 106) and (150, 106)) and the tables (68df, pictures 993 / 994): only
	// OK, Enter, Esc or Space close them
	Graphics::ManagedSurface *background = _archive.loadImage(picture);
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);
	if (background)
		_screen->blitFrom(*background, Common::Point(0, 0));
	if (number >= 0) {
		for (int k = 0; k < 2; ++k) {
			Graphics::ManagedSurface *d = _archive.loadImage(980 + (k ? number % 10 : (number / 10) % 10));
			if (d)
				_screen->transBlitFrom(*d, Common::Point(142 + 8 * k, 106), d->getTransparentColor());
			delete d;
		}
	}
	if (ok)
		drawBevelButton(_screen, *ok, kPanelOkX, kPanelOkY, false);
	bool done = false;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!done && !shouldQuit()) {
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN && (isOkKey(e) || e.kbd.keycode == Common::KEYCODE_ESCAPE))
				done = true;
			else if (e.type == Common::EVENT_LBUTTONDOWN && ok && buttonRect(*ok, kPanelOkX, kPanelOkY).contains(e.mouse))
				done = trackButton(*ok, kPanelOkX, kPanelOkY);
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	delete background;
	delete ok;
}

bool MiljostrupEngine::runPanelScreen(int panel) {
	// Humidity (6903, close-up 11, picture 990), temperature (6876, 14,
	// 995), light (6804, 15, 996); true for OK, with the result in the
	// panel's fields
	const bool humidity = panel == kHumidityPanel, temperature = panel == kTemperaturePanel;
	Graphics::ManagedSurface *background = _archive.loadImage(humidity ? 990 : (temperature ? 995 : 996));
	Graphics::ManagedSurface *digit[11];
	for (int k = 0; k < 11; ++k)
		digit[k] = _archive.loadImage(595 + k); // 605: blank
	Graphics::ManagedSurface *lamp[2] = { _archive.loadImage(992), _archive.loadImage(991) }; // off, on
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *cancel = loadImageObject(_exe, kButtonCancelImage);
	Graphics::ManagedSurface *up = loadImageObject(_exe, kButtonUpImage);
	Graphics::ManagedSurface *down = loadImageObject(_exe, kButtonDownImage);
	Graphics::ManagedSurface *on = loadImageObject(_exe, kButtonOnImage);
	Graphics::ManagedSurface *off = loadImageObject(_exe, kButtonOffImage);

	// the values: humidity +0x31 ventilation, +0x32 setting (50..80);
	// temperature +0x31 (15..39); light +0x31 on, +0x32 automatic. The
	// gauge shows the value the room has now (+0x1e9 = the value at the
	// start)
	int value = humidity ? getField(panel, 0x32) : getField(panel, 0x31);
	const int reading = value;
	bool switchA = humidity ? getField(panel, 0x31) != 0 : getField(panel, 0x31) != 0; // ventilation / light
	bool switchB = getField(panel, 0x32) != 0;                                         // automatic
	const int minValue = humidity ? 50 : 15, maxValue = humidity ? 80 : 39;
	// positions: up / down, digits, the switch pairs (on above off), lamps
	const int downX = humidity ? 199 : 105, upX = humidity ? 235 : 140, arrowY = humidity ? 73 : 75;
	const int digitX = humidity ? 215 : 121, digitY = humidity ? 46 : 49;

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);
	int result = 0; // 1 OK, -1 Cancel
	bool redraw = true;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!result && !shouldQuit()) {
		if (redraw) {
			redraw = false;
			if (background)
				_screen->blitFrom(*background, Common::Point(0, 0));
			if (panel != kLightPanel) {
				// 6903:0445 / 6876:0315: the setting's digits, blank while
				// the ventilation is off; 0546 / 0399: the gauge
				bool shown = temperature || switchA;
				for (int k = 0; k < 2; ++k) {
					int d = shown ? (k ? value % 10 : (value / 10) % 10) : 10;
					if (digit[d])
						_screen->transBlitFrom(*digit[d], Common::Point(digitX + 14 * k, digitY), digit[d]->getTransparentColor());
				}
				if (humidity) {
					if (lamp[switchA])
						_screen->transBlitFrom(*lamp[switchA], Common::Point(84, 54), lamp[switchA]->getTransparentColor());
					if (!switchA) {
						_screen->fillRect(Common::Rect(165, 46, 172, 158), 47);
					} else {
						int h = (95 - reading) * 12 / 5;
						_screen->fillRect(Common::Rect(165, 46, 172, 47 + h), 47);
						_screen->fillRect(Common::Rect(165, 47 + h, 172, 158), 32);
					}
				} else {
					int h = (70 - reading) * 12 / 10;
					_screen->fillRect(Common::Rect(219, 37, 226, 38 + h), 47);
					_screen->fillRect(Common::Rect(219, 38 + h, 226, 149), 32);
				}
				drawBevelButton(_screen, *down, downX, arrowY, false);
				drawBevelButton(_screen, *up, upX, arrowY, false);
			} else {
				// 6804:0363: the light's lamp, the automatic's only while on
				if (lamp[switchA])
					_screen->transBlitFrom(*lamp[switchA], Common::Point(108, 61), lamp[switchA]->getTransparentColor());
				bool autoLamp = switchA && switchB;
				if (lamp[autoLamp])
					_screen->transBlitFrom(*lamp[autoLamp], Common::Point(193, 61), lamp[autoLamp]->getTransparentColor());
			}
			// the switch pairs (1ccf radio groups): on at (x + 3, y + 3), off
			// at (x + 3, y + 23), the chosen one pressed
			if (humidity) {
				drawBevelButton(_screen, *on, 60, 65, switchA, 1, 1, 16);
				drawBevelButton(_screen, *off, 60, 85, !switchA, 1, 1, 16);
			} else if (!temperature) {
				drawBevelButton(_screen, *on, 85, 74, switchA, 1, 1, 16);
				drawBevelButton(_screen, *off, 85, 94, !switchA, 1, 1, 16);
				drawBevelButton(_screen, *on, 171, 74, switchB, 1, 1, 16);
				drawBevelButton(_screen, *off, 171, 94, !switchB, 1, 1, 16);
			}
			drawBevelButton(_screen, *ok, kPanelOkX, kPanelOkY, false);
			drawBevelButton(_screen, *cancel, kPanelCancelX, kPanelCancelY, false);
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
				if (buttonRect(*ok, kPanelOkX, kPanelOkY).contains(m)) {
					if (trackButton(*ok, kPanelOkX, kPanelOkY))
						result = 1;
				} else if (buttonRect(*cancel, kPanelCancelX, kPanelCancelY).contains(m)) {
					if (trackButton(*cancel, kPanelCancelX, kPanelCancelY))
						result = -1;
				} else if (panel != kLightPanel && buttonRect(*up, upX, arrowY).contains(m)) {
					step = 1;
				} else if (panel != kLightPanel && buttonRect(*down, downX, arrowY).contains(m)) {
					step = -1;
				} else if (humidity || !temperature) {
					// the switches (sound 1d26:0155)
					int ax = humidity ? 60 : 85, ay = humidity ? 65 : 74;
					bool hit = true;
					if (buttonRect(*on, ax, ay).contains(m))
						switchA = true;
					else if (buttonRect(*off, ax, ay + 20).contains(m))
						switchA = false;
					else if (!humidity && buttonRect(*on, 171, 74).contains(m))
						switchB = true;
					else if (!humidity && buttonRect(*off, 171, 94).contains(m))
						switchB = false;
					else
						hit = false;
					if (hit) {
						playSound(kSoundClick);
						redraw = true;
					}
				}
			}
			// 6903:0666 / 06af, 6876:0421 / 0462: one step within the range
			// (the humidity only with the ventilation on), sound 1d26:0178
			if (step && (temperature || switchA) && value + step >= minValue && value + step <= maxValue) {
				value += step;
				playSound(kSoundClicks);
				redraw = true;
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	if (result == 1) {
		// 7000:17c4 / 1dfc / 251a: the values and the award pending (+0x2f)
		if (humidity) {
			setField(panel, 0x31, switchA ? 1 : 0);
			setField(panel, 0x32, value);
		} else if (temperature) {
			setField(panel, 0x31, value);
		} else {
			setField(panel, 0x31, switchA ? 1 : 0);
			setField(panel, 0x32, switchB ? 1 : 0);
		}
		setFlag(panel, 1);
	}
	delete background;
	for (int k = 0; k < 11; ++k)
		delete digit[k];
	delete lamp[0];
	delete lamp[1];
	delete ok;
	delete cancel;
	delete up;
	delete down;
	delete on;
	delete off;
	return result == 1;
}

int MiljostrupEngine::getChickenFigureState(uint32 rule, int object, bool &visible) const {
	switch (rule) {
	case 0x70000b20: // the feed belt: still 1, running 2
		return getFlag(kFeedBelt) ? 2 : 1;
	case 0x70000eef: // the door: the open door shown while it is open
		visible = getFlag(object) != 0;
		return 1;
	case 0x700028ed: // the dispenser switch: on 1, off 2
		return getFlag(object) ? 1 : 2;
	case 0x700006e2: // the barrow by the field: there while not in the dung room, full 2
		visible = getFlag(kBarrow) == 0;
		return getField(kBarrow, 0x30) ? 2 : 1;
	case 0x70003a80: // the barrow in the dung room: there while +0x2f, full 2
		visible = getFlag(object) != 0;
		return getField(object, 0x30) ? 2 : 1;
	case 0x7000355d: // the dung: until it is cleared away
		visible = getFlag(object) == 0;
		return 1;
	case 0x700037de: // the drains: open 1, closed 2
		return getFlag(object) ? 1 : 2;
	case 0x70003eda: // the hose: hung up 1, in use 2
		return getFlag(object) ? 1 : 2;
	case 0x70002a31: // the hole: leaking (1) while the water is on, patched 2
		if (getFlag(object))
			return 2;
		visible = getFlag(kWaterSwitch) != 0;
		return 1;
	default:
		return 0;
	}
}

bool MiljostrupEngine::runChickenHandler(uint32 handler, int verb, int object, int second) {
	if (_location != kChickenFarm)
		return false;
	switch (handler) {
	// The feed belt 115 (VMT 3b06)
	case 0x70000811: // Se på
		say(handlerText(getFlag(object) ? 0x700007e3 : 0x700007f9), 14, true);
		break;
	case 0x70000976: // Tænd
		if (getFlag(object)) {
			say(handlerText(0x70000873), 14, true);
		} else {
			setFlag(object, 1);
			refreshObject(kBeltFigure);
			playSound(kSoundClick);
			award(10, 0, 10, 0, 0x70000892, 0);
		}
		break;
	case 0x70000a48: // Sluk: it stays on
		say(handlerText(getFlag(object) ? 0x70000a01 : 0x70000a27), 14, true);
		break;

	// The door 117 (VMT 3bc6)
	case 0x70000bf4: // Se på
		say(handlerText(getFlag(object) ? 0x70000bcc : 0x70000be3), 14, true);
		break;
	case 0x70000d21: // Åbn
		if (getFlag(object)) {
			say(handlerText(0x70000c56), 14, true);
		} else {
			setFlag(object, 1);
			refreshObject(object);
			setRoomPalette(_scene->getRoom()->palette);
			award(-10, 0, -10, 0, 0x70000c6e, 0);
			playSound(kSoundOpen);
		}
		break;
	case 0x70000e5e: // Luk
		if (!getFlag(object)) {
			say(handlerText(0x70000e44), 14, true);
		} else {
			setFlag(object, 0);
			refreshObject(object);
			setRoomPalette(_scene->getRoom()->palette);
			award(10, 0, 10, 0, 0x70000db5, 0);
			playSound(kSoundClose);
		}
		break;

	// The calendar 124 (7000:1223: GoTo(10)) and the tables 127 / 128 (12d9:
	// GoTo(+0x2f), rooms 12 / 13 with pictures 993 / 994)
	case 0x70001223:
		runPictureScreen(979, getFlag(kCalendar));
		redrawRoom();
		break;
	case 0x700012d9:
		runPictureScreen(object == 127 ? 993 : 994, -1);
		redrawRoom();
		break;

	// The panels 126 / 129 / 130: Benyt once (+0x30), the close-up
	case 0x70001451: // humidity: GoTo(11)
	case 0x70001b78: // temperature: GoTo(14)
	case 0x70002046: // light: GoTo(15)
		if (getField(object, 0x30)) {
			say(handlerText(handler == 0x70001451 ? 0x7000142d : (handler == 0x70001b78 ? 0x70001b54 : 0x70002022)), 14, true);
		} else {
			runPanelScreen(object);
			redrawRoom();
		}
		break;

	// The barrow: by the field 111 (VMT 3aa6), in the dung room 134 (VMT 40a6)
	case 0x700000a3: // Se på
		say(handlerText(getField(kBarrow, 0x30) ? 0x7000005e : 0x70000084), 14, true);
		break;
	case 0x700038f6:
		say(handlerText(getField(kBarrow, 0x30) ? 0x700038b1 : 0x700038d7), 14, true);
		break;
	case 0x7000063a: // Skub / Træk (0681 = Skub): into the dung room
	case 0x70000681:
		moveBarrow(true);
		break;
	case 0x700039dd: // out to the field
	case 0x70003a1f:
		moveBarrow(false);
		break;
	case 0x700004d1: // Benyt the full barrow with ...
		if (!getField(kBarrow, 0x30)) {
			say(handlerText(0x700004b5), 14, true);
		} else if (second == kDeadContainer) {
			emptyBarrow(-10, 0x7000017c);
			// "Hvis man gøder marken for meget, forurener det?" luften / vandet: 0
			setQuiz(0x70000222, 0x70000252, 0x70000259, 0, 2);
		} else if (second == kWasteContainer) {
			emptyBarrow(-10, 0x70000260);
			// "Kan man lave kunstgødning?" Ja / Nej: 1
			setQuiz(0x7000032a, 0x70000345, 0x70000348, 1, 2);
		} else if (second == kField) {
			emptyBarrow(10, 0x7000034c);
		} else if (second == kCompost) {
			emptyBarrow(10, 0x700003f4);
		} else {
			say(handlerText(0x70000497), 14, true);
		}
		break;
	case 0x70003976: // the barrow in the dung room with the dung: the dung's Benyt
		if (second == kDung)
			executeVerb(kVerbUse, kDung, object);
		else if (getField(object, 0x30))
			say(handlerText(0x70003958), 14, true);
		else
			sayRootMessage(kVerbUse);
		break;

	// The dung 133 (VMT 3fe6): Benyt with the barrow
	case 0x700034d6:
		if (second == kBarrow) {
			setFlag(object, 1);
			refreshObject(object);
			setField(kBarrow, 0x30, 1);
			refreshObject(kBarrow);
			say(handlerText(0x700034b6), 14, true);
		} else {
			say(handlerText(0x700034bd), 14, true);
		}
		break;

	// The dead chickens 132 (VMT 3f86): Benyt with ...
	case 0x700031ea:
		if (second == kDeadContainer) {
			dropDeadChickens(10, 0x70002e18);
		} else if (second == kWasteContainer) {
			dropDeadChickens(-10, 0x70002e89);
			setQuiz(0x70002f48, 0x70002f68, 0x70002f6b, 0, 2);
		} else if (second == kField) {
			dropDeadChickens(-10, 0x70002f6f);
			setQuiz(0x70002fe4, 0x70002f68, 0x70002f6b, 0, 2);
		} else if (second == kCompost) {
			dropDeadChickens(-10, 0x70003002);
			setQuiz(0x700030aa, 0x700030e8, 0x700030f1, 1, 2);
		} else if (second == kLorry) {
			dropDeadChickens(-10, 0x70003102);
			setQuiz(0x700031b8, 0x70002f68, 0x70002f6b, 1, 2);
		} else {
			say(handlerText(0x700031d4), 14, true);
		}
		break;

	// The drains 135 (to the tank) / 136 (to the stream) (VMT 4046)
	case 0x70003673: // Se på
		if (!getFlag(object))
			say(handlerText(0x70003660), 14, true);
		else
			say(handlerText(object == kStreamDrain ? 0x7000362b : 0x70003643), 14, true);
		break;
	case 0x7000371f: // Åbn
		if (getFlag(object)) {
			say(handlerText(0x70003704), 14, true);
		} else {
			setFlag(object, 1);
			refreshObject(object);
			playSound(kSoundOpen);
		}
		break;
	case 0x7000378e: // Luk
		if (!getFlag(object)) {
			say(handlerText(0x70003772), 14, true);
		} else {
			setFlag(object, 0);
			refreshObject(object);
			playSound(kSoundOpen);
		}
		break;

	// The floor 137 (VMT 4106) and the hose 138 (VMT 4166)
	case 0x70003bcd:
		say(handlerText(getFlag(object) ? 0x70003ba9 : 0x70003bb9), 14, true);
		break;
	case 0x70003db2: // Benyt the hose with the floor
		if (second == kDung) {
			say(handlerText(0x70003d41), 14, true);
		} else if (second != kFloor) {
			sayRootMessage(kVerbUse);
		} else if (getFlag(kFloor)) {
			say(handlerText(0x70003d5d), 14, true);
		} else if (getFlag(kDeadChickensProxy)) {
			say(handlerText(0x70003d78), 14, true);
		} else if (!getFlag(kTankDrain) && !getFlag(kStreamDrain)) {
			say(handlerText(0x70003d90), 14, true);
		} else {
			// the hose in use, command 0x1f to the room (69d2:0011)
			setFlag(object, 0);
			refreshObject(object);
			setInputOff(true);
			_scene->playPlayer(kHosingState, 50);
			_hosing = true;
		}
		break;

	// The chickens 118 (VMT 3c26): Se på (11a0, dead, is always false)
	case 0x70001029:
		say(handlerText(areChickensWell() ? 0x70000fd4 : 0x70001006), 14, true);
		break;

	// The water dispenser's switch 123 (VMT 3e66)
	case 0x70002628: // Se på
		say(handlerText(getFlag(object) ? 0x700025f6 : 0x7000260e), 14, true);
		break;
	case 0x70002714: // Tænd
		if (getFlag(object)) {
			say(handlerText(0x7000268a), 14, true);
		} else {
			setFlag(object, 1);
			refreshObject(object);
			refreshObject(kPipeHole);
			award(10, 0, 10, 0, 0x700026a2, 0);
			playSound(kSoundClick);
		}
		break;
	case 0x70002851: // Sluk
		if (!getFlag(object)) {
			say(handlerText(0x7000282e), 14, true);
		} else {
			setFlag(object, 0);
			refreshObject(object);
			refreshObject(kPipeHole);
			award(-10, 0, -10, 0, 0x700027b3, 0);
			playSound(kSoundClick);
		}
		break;

	// The hole in the water pipe 121 (VMT 3ec6) and the tape 122 (VMT 3f26)
	case 0x700029cf: // Se på
		say(handlerText(getFlag(object) ? 0x700029a8 : 0x700029ba), 14, true);
		break;
	case 0x70002c04: // Benyt the tape with the hole
		if (second == kPipeHole) {
			removeObject(object);
			setFlag(kPipeHole, 1);
			refreshObject(kPipeHole);
			award(10, 0, 0, 10, 0x70002b74, 0);
		} else {
			sayRootMessage(kVerbUse);
		}
		break;

	default:
		return false;
	}
	return true;
}

} // End of namespace Miljostrup
