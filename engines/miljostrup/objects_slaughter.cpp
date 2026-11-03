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

// The slaughterhouse (location 5), written from notes/slaughterhouse.md: the
// hose (sewer or the cleaning plant) and the chickens' by-products, each with
// the close-up "Hvor bør ... sendes hen?" (unit 648d).

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
	kSlaughterhouse = 5,
	kCleaningPlant = 102,  // anlæg
	kSewer = 103,          // udløb
	kHose = 104            // DS:c660, +0x2f into the sewer (start 1)
};

// The by-products (6799:0144 -> 6799:0000: +0x12f answered, +0x130 the
// close-up room) and their close-ups' kind (dispatcher 6473: rooms 8..15 ->
// 648d:12b2(kind)): the kind is the row of the tables
struct Byproduct {
	uint16 object;
	byte kind;
};
static const Byproduct kByproducts[8] = {
	{ 109, 0 }, { 110, 1 }, { 111, 2 }, { 112, 3 }, { 106, 4 }, { 105, 5 }, { 113, 6 }, { 115, 7 }
};

// 648d: the tables' layout. Rows (destinations 1..9): y (DS:288e) and
// height (DS:28a0); columns (kinds 0..7): x (DS:28b4) and width (DS:28c4);
// kinds 0..3 on picture 1276, 4..7 on 1277
static const int kRowY[10] = { 0, 27, 46, 63, 80, 97, 114, 131, 148, 165 };
static const int kRowH[10] = { 0, 16, 14, 14, 14, 14, 14, 14, 14, 14 };
static const int kColX[8] = { 100, 151, 202, 253, 100, 130, 179, 222 };
static const int kColW[8] = { 48, 48, 48, 48, 27, 46, 40, 78 };

static const Byproduct *findByproduct(int object) {
	for (int i = 0; i < 8; ++i) {
		if (kByproducts[i].object == object)
			return &kByproducts[i];
	}
	return nullptr;
}

// The line (rooms 5 and 6): figures that walk along and start again when
// their walk ends (message 0x1f.. back to the room). Room 5: the hanging
// chickens 931..933 (6688:0356, again 0011: SetPos(96, 40), Walk(1, 96, 0,
// 24), Walk(3, 4, 0, 1), Walk(2, 44, 0, 11)) and the lower line 934 / 935
// (0536 / 0622, again 00d5: SetPos(-28, 62), Walk(1, 52, 0, 13), Walk(2, 44,
// 0, 11)); room 6: 941..945 (6605:02a9, again 0011: SetPos(-12, 40), Walk(1,
// 240, 0, 60)). Walk(state, dx, dy, steps).
void MiljostrupEngine::slaughterRoomEntered() {
	if (_location != kSlaughterhouse)
		return;
	if (_room == 5) {
		static const int kOff[3] = { 16, 28, 4 };
		for (int k = 0; k < 3; ++k) {
			int off = kOff[k], whole = 35 - off, first = 24 - off;
			int fig = 931 + k;
			if (first < 1) {
				_scene->queueFigureWalk(fig, 2, 4 * whole, 0, whole);
			} else {
				if (first > 1)
					_scene->queueFigureWalk(fig, 1, 4 * (23 - off), 0, 23 - off);
				_scene->queueFigureWalk(fig, 3, 4, 0, 1);
				_scene->queueFigureWalk(fig, 2, 4 * (whole - first), 0, whole - first);
			}
		}
		_scene->queueFigureWalk(934, 1, 48, 0, 12);
		_scene->queueFigureWalk(934, 2, 44, 0, 11);
		_scene->queueFigureWalk(935, 2, 44, 0, 11);
		static const WalkCommand kHanging[3] = { { 1, 96, 0, 24 }, { 3, 4, 0, 1 }, { 2, 44, 0, 11 } };
		static const WalkCommand kLower[2] = { { 1, 52, 0, 13 }, { 2, 44, 0, 11 } };
		for (int fig = 931; fig <= 933; ++fig)
			_scene->setFigureRoute(fig, 96, 40, kHanging, 3);
		for (int fig = 934; fig <= 935; ++fig)
			_scene->setFigureRoute(fig, -28, 62, kLower, 2);
	} else if (_room == 6) {
		static const WalkCommand kLine[1] = { { 1, 240, 0, 60 } };
		for (int k = 0; k < 5; ++k) {
			_scene->queueFigureWalk(941 + k, 1, 236 - 48 * k, 0, 59 - 12 * k);
			_scene->setFigureRoute(941 + k, -12, 40, kLine, 1);
		}
	}
}

void MiljostrupEngine::initSlaughterhouse() {
	// 6799:01d1: the hose's water into the sewer
	setFlag(kHose, 1);
}

bool MiljostrupEngine::isSlaughterObjectEnabled(int object, bool &enabled) const {
	// 6799:0107: a by-product's region until it is answered (+0x12f)
	if (findByproduct(object)) {
		enabled = getField(object, 0x12f) == 0;
		return true;
	}
	return false;
}

int MiljostrupEngine::getSlaughterFigureState(uint32 rule, int object, bool &visible) const {
	switch (rule) {
	case 0x679904c0: // the hose: into the plant 2, into the sewer 1
		return getFlag(object) ? 1 : 2;
	default:
		return 0;
	}
}

bool MiljostrupEngine::runByproductScreen(int kind) {
	// 648d:0072 / 0164: the picture, the title "Hvor bør " + the name +
	// " sendes hen?" (centred in the black top line, colour 14); OK 1db8:01d1
	// at (132, 187) command 0x40; the rows (28c3:0261, commands 0x41..0x49)
	// toggle a cross (picture 1278, centred in the cell) in the kind's column
	// (648d:0486; off: the cell filled with colour 16)
	Graphics::ManagedSurface *background = _archive.loadImage(kind < 4 ? 1276 : 1277);
	Graphics::ManagedSurface *cross = _archive.loadImage(1278);
	Graphics::ManagedSurface *wrong = _archive.loadImage(1280);
	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	static const int kOkX = 132, kOkY = 187;
	const Common::String name = readPascalString(_exe, kByproductNames[kind]);

	setPalette(kPaletteA);
	_interface->setCursor(Interface::kCursorArrow);
	if (background)
		_screen->blitFrom(*background, Common::Point(0, 0));
	drawScreenTitle(handlerText(0x648d014d) + name + handlerText(0x648d0157));
	if (ok)
		drawBevelButton(_screen, *ok, kOkX, kOkY, false);

	auto cell = [&](int row, const Graphics::ManagedSurface &image) {
		return Common::Point(kColX[kind] + (kColW[kind] - image.w + 1) / 2, kRowY[row] + (kRowH[row] - image.h + 1) / 2);
	};
	bool marked[10] = { false };
	bool evaluated = false, done = false;
	int value = 0;
	const Common::Rect okRect = ok ? buttonRect(*ok, kOkX, kOkY) : Common::Rect();
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!done && !shouldQuit()) {
		Common::Event e;
		bool okPressed = false;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN && (e.kbd.keycode == Common::KEYCODE_RETURN ||
					e.kbd.keycode == Common::KEYCODE_KP_ENTER || e.kbd.keycode == Common::KEYCODE_SPACE ||
					e.kbd.keycode == Common::KEYCODE_ESCAPE)) {
				okPressed = true;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				if (ok && okRect.contains(e.mouse)) {
					okPressed = trackButton(*ok, kOkX, kOkY);
				} else if (!evaluated) {
					for (int r = 1; r <= 9; ++r) {
						if (e.mouse.y >= kRowY[r] - 1 && e.mouse.y <= kRowY[r] + kRowH[r]) {
							if (marked[r])
								_screen->fillRect(Common::Rect(kColX[kind], kRowY[r], kColX[kind] + kColW[kind] + 1,
									kRowY[r] + kRowH[r] + 1), 16);
							else if (cross)
								_screen->transBlitFrom(*cross, cell(r, *cross), cross->getTransparentColor());
							marked[r] = !marked[r];
							break;
						}
					}
				}
			}
		}
		if (okPressed) {
			if (evaluated) {
				done = true;
			} else {
				// 648d:062b: the first OK answers (6799:0134); 1171: 3 points
				// for a right cross, -1 and the wrong mark (1280) for a wrong
				// one; 0882: the verdict as the title with its sound; 104c:
				// the award text
				evaluated = true;
				for (int i = 0; i < 8; ++i) {
					if (kByproducts[i].kind == kind)
						setField(kByproducts[i].object, 0x12f, 1);
				}
				int right = 0, wrongCount = 0;
				for (int r = 1; r <= 9; ++r) {
					if (!marked[r])
						continue;
					if (kByproductRight[kind][r - 1]) {
						value += 3;
						++right;
					} else {
						value -= 1;
						++wrongCount;
						if (wrong)
							_screen->transBlitFrom(*wrong, cell(r, *wrong), wrong->getTransparentColor());
					}
				}
				uint32 verdict;
				SoundEffectId sound;
				if (!wrongCount) {
					verdict = right ? 0x648d0834 : 0x648d0816;
					sound = right ? kSoundPositive : kSoundZero;
				} else {
					verdict = right ? 0x648d0867 : 0x648d0851;
					sound = right ? kSoundZero : kSoundNegative;
				}
				drawScreenTitle(handlerText(verdict));
				_screen->update();
				playSound(sound);
				// the pointer stays, for the second OK (the award follows when it closes)
				_interface->setCursor(Interface::kCursorArrow);
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	delete background;
	delete cross;
	delete wrong;
	delete ok;
	if (!evaluated)
		return false;
	// straight from the close-up to the award screen (the user's check
	// against the original), then the room
	_award.active = true;
	_award.talking = false;
	_award.values[0] = value;
	_award.values[1] = 0;
	_award.values[2] = value;
	_award.values[3] = 0;
	_award.text = byproductText(kind);
	_award.page = Common::String();
	_award.quiz = false;
	showPendingAward();
	return true;
}

Common::String MiljostrupEngine::byproductText(int kind) {
	// 648d:104c / 0977: the name with a capital letter + " bør sendes til "
	// + the right destinations ("a, b eller c") + ". " + the kind's text (0e65)
	Common::String list, last;
	for (int r = 1; r <= 9; ++r) {
		if (!kByproductRight[kind][r - 1])
			continue;
		if (!last.empty())
			list = list.empty() ? last : list + handlerText(0x648d0974) + last;
		last = readPascalString(_exe, kDestinationNames[r - 1]);
	}
	Common::String places = list.empty() ? last : list + handlerText(0x648d1030) + last;
	Common::String name = readPascalString(_exe, kByproductNames[kind]);
	if (!name.empty())
		name.setChar(toupper(name[0]), 0);
	static const uint32 kTexts[8][4] = {
		{ 0x648d0a0c, 0, 0, 0 },
		{ 0x648d0afe, 0, 0, 0 },
		{ 0x648d0bba, 0x648d0bf0, 0x648d0c2d, 0x648d0c62 },
		{ 0x648d0c73, 0x648d0ca6, 0, 0 },
		{ 0x648d0cce, 0x648d0d06, 0, 0 },
		{ 0x648d0d22, 0x648d0d59, 0x648d0d90, 0 },
		{ 0x648d0da7, 0, 0, 0 },
		{ 0x648d0de0, 0, 0, 0 }
	};
	// 648d:0e65: hearts, livers and the trays get only their own text, the
	// others it after the sentence (String[255])
	Common::String text = name + handlerText(0x648d1038) + places + handlerText(0x648d1049);
	if (kind == 0 || kind == 1 || kind == 7)
		text.clear();
	for (int i = 0; i < 4 && kTexts[kind][i]; ++i)
		text += handlerText(kTexts[kind][i]);
	if (text.size() > 255)
		text = Common::String(text.c_str(), 255);
	return text;
}

bool MiljostrupEngine::runSlaughterHandler(uint32 handler, int verb, int object, int second) {
	if (_location != kSlaughterhouse)
		return false;
	switch (handler) {
	// The by-products (VMT 3036): Tag (00d3) and Benyt (00c0 = Tag) open
	// the close-up, GoTo(+0x130)
	case 0x679900d3:
	case 0x679900c0: {
		const Byproduct *b = findByproduct(object);
		if (b) {
			if (!runByproductScreen(b->kind))
				redrawRoom();
		}
		break;
	}

	// The hose 104 (VMT 3096)
	case 0x67990274: // Se på
		say(handlerText(getFlag(object) ? 0x67990229 : 0x67990251), 14, true);
		break;
	case 0x679903bd: // Benyt with the sewer or the cleaning plant
		if (second == kSewer) {
			say(handlerText(getFlag(object) ? 0x679902d6 : 0x679902f5), 14, true);
		} else if (second == kCleaningPlant) {
			if (!getFlag(object)) {
				say(handlerText(0x679903a2), 14, true);
			} else {
				award(10, 0, 0, 10, 0x6799030d, 0);
				setFlag(object, 0);
				refreshObject(object);
			}
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
