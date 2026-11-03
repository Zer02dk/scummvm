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

// The response screen after an award (unit b331, notes/response.md).

#include "common/debug.h"
#include "common/events.h"
#include "common/system.h"
#include "graphics/framelimiter.h"

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/rooms.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/response_data.h"

namespace Miljostrup {

enum {
	kPanelImage = 23,
	kBarPlusImage = 24,     // green
	kBarMinusImage = 25,    // red
	kPresenterBox = 26,
	kPageBoxImage = 38,
	kQuizPanel = 39,
	kQuizButton1 = 40,
	kQuizButton2 = 41,
	kPresenterX = 4,
	kPresenterY = 6,
	kPresenterTicks = 3,    // 2381:0000(0, 0, 1, 3)
	kTextX = 12,
	kTextY = 98,
	kTextLineHeight = 9,
	kTextColor = 23,
	kMaxLine = 38,          // a word fits while line + word < 38
	kOkX = 132, kOkY = 185, // command 3
	kHelpX = 296, kHelpY = 182 // command 0x40
};

static void wrapText(const Common::String &text, Common::Array<Common::String> &lines) {
	// b331:0788 / 06c0: words end at a space; a run of \r ends the paragraph
	uint pos = 0;
	Common::String line;
	while (pos < text.size()) {
		Common::String word;
		while (pos < text.size() && text[pos] != ' ' && text[pos] != '\r')
			word += text[pos++];
		if (pos < text.size() && text[pos] == ' ')
			++pos;
		bool newParagraph = false;
		while (pos < text.size() && text[pos] == '\r') {
			newParagraph = true;
			++pos;
		}
		if (!line.empty())
			line += ' ';
		if (line.size() + word.size() < kMaxLine) {
			line += word;
		} else {
			lines.push_back(line);
			line = word;
		}
		if (newParagraph) {
			if (!line.empty())
				lines.push_back(line);
			line.clear();
		}
	}
	if (!line.empty())
		lines.push_back(line);
}

bool MiljostrupEngine::runResponse(const Common::String &text, const Common::String &page, const int values[4], bool showHelp) {
	// b331:0000: panel, page box, text, buttons; the presenter (b331:00de)
	// gestures by the sign of the points, then the bars and the jingle
	// (b331:034b) appear
	setPalette(kPaletteA);
	Graphics::ManagedSurface *panel = _archive.loadImage(kPanelImage);
	if (panel) {
		_screen->transBlitFrom(*panel, Common::Point(0, 0), panel->getTransparentColor());
		delete panel;
	}
	if (!page.empty()) {
		Graphics::ManagedSurface *box = _archive.loadImage(kPageBoxImage);
		if (box) {
			_screen->transBlitFrom(*box, Common::Point(4, 180), box->getTransparentColor());
			delete box;
		}
		_font.drawString(_screen, readPascalString(_ovr, kTextPagePrefix) + page +
			readPascalString(_ovr, kTextPageSuffix), 37, 186, 8);
	}
	Common::Array<Common::String> lines;
	wrapText(text, lines);
	for (uint i = 0; i < lines.size(); ++i)
		_font.drawString(_screen, lines[i], kTextX, kTextY + i * kTextLineHeight, kTextColor);

	Graphics::ManagedSurface *ok = loadImageObject(_exe, kButtonOkImage);
	Graphics::ManagedSurface *help = loadImageObject(_exe, kButtonHelpImage);
	drawBevelButton(_screen, *ok, kOkX, kOkY, false);
	if (showHelp)
		drawBevelButton(_screen, *help, kHelpX, kHelpY, false);

	// Presenter: state 1 plays its sequence once (a walk of n steps), then
	// state 2 loops
	int first, idle;
	if (values[0] < 0) {
		first = 1;
		idle = 2;
	} else if (values[0] == 0) {
		first = 3;
		idle = 4;
	} else {
		first = 5;
		idle = 6;
	}
	const SequenceInfo &a = kPresenterSequences[first - 1];
	const SequenceInfo &b = kPresenterSequences[idle - 1];
	Graphics::ManagedSurface *box = _archive.loadImage(kPresenterBox);
	Graphics::ManagedSurface *bar[2] = { _archive.loadImage(kBarPlusImage), _archive.loadImage(kBarMinusImage) };

	_interface->setCursor(Interface::kCursorArrow);
	Graphics::FrameLimiter limiter(g_system, 25, false);
	int countdown = 1, step = 0, idleStep = 0;
	bool barsShown = false, helpChosen = false, done = false;
	uint16 shown = 0;
	while (!done && !shouldQuit()) {
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				// b331:02e9: Enter, Esc or Space close the screen
				if (e.kbd.keycode == Common::KEYCODE_RETURN || e.kbd.keycode == Common::KEYCODE_KP_ENTER ||
					e.kbd.keycode == Common::KEYCODE_ESCAPE || e.kbd.keycode == Common::KEYCODE_SPACE)
					done = true;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				Common::Rect okRect(kOkX - 2, kOkY - 2, kOkX + ok->w + 2, kOkY + ok->h + 2);
				Common::Rect helpRect(kHelpX - 2, kHelpY - 2, kHelpX + help->w + 2, kHelpY + help->h + 2);
				if (okRect.contains(e.mouse)) {
					done = trackButton(*ok, kOkX, kOkY);
				} else if (showHelp && helpRect.contains(e.mouse) && trackButton(*help, kHelpX, kHelpY)) {
					helpChosen = true; // b331:03d1: DS:cdfe := 1
					done = true;
				}
			}
		}

		if (--countdown <= 0) {
			countdown = kPresenterTicks;
			uint16 image;
			if (step < a.count) {
				image = kPresenterImages[a.first + step];
				++step;
			} else {
				// The advance after the last step ends the walk; its message
				// 0x1e reaches b331:034b at once (28c3:0177), which sets state
				// 2 before this advance animates: the idle sequence starts
				// here. The jingles (1d26:01aa / 0203 / 025c) are not played yet.
				if (!barsShown) {
					barsShown = true;
					for (int k = 0; k < 4; ++k) {
						int v = CLIP(values[k], -10, 10);
						int y = 17 * (k + 1);
						for (int i = 1; i <= v; ++i)
							_screen->transBlitFrom(*bar[0], Common::Point(238 + 6 * i, y), bar[0]->getTransparentColor());
						for (int i = -1; i >= v; --i)
							_screen->transBlitFrom(*bar[1], Common::Point(244 + 6 * i, y), bar[1]->getTransparentColor());
					}
					_screen->update();
					playSound(values[0] > 0 ? kSoundPositive : (values[0] < 0 ? kSoundNegative : kSoundZero));
				}
				image = kPresenterImages[b.first + idleStep % b.count];
				++idleStep;
			}
			if (image != shown) {
				shown = image;
				_screen->blitFrom(*box, Common::Point(kPresenterX, kPresenterY));
				Graphics::ManagedSurface *frame = _archive.loadImage(image);
				if (frame) {
					_screen->transBlitFrom(*frame, Common::Point(kPresenterX, kPresenterY), frame->getTransparentColor());
					delete frame;
				}
			}
		}

		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	delete box;
	delete bar[0];
	delete bar[1];
	delete ok;
	delete help;
	return helpChosen;
}

int MiljostrupEngine::runQuiz(const Common::String &question, const Common::String &answer1,
		const Common::String &answer2) {
	// b21c:0000: panel image 39, the title with a shadow, the question
	// (wrapped as the response text) at (12, 51 + 9n), the answers at
	// (42, 121) and (42, 141), the buttons (images 40 and 41) at (18, 119)
	// command 0x40 and (18, 139) command 0x41
	setPalette(kPaletteA);
	Graphics::ManagedSurface *panel = _archive.loadImage(kQuizPanel);
	if (panel) {
		_screen->transBlitFrom(*panel, Common::Point(0, 0), panel->getTransparentColor());
		delete panel;
	}
	Common::String title = readPascalString(_ovr, kTextQuizTitle);
	_font.drawString(_screen, title, 69, 17, 8);
	_font.drawString(_screen, title, 68, 16, 15);
	Common::Array<Common::String> lines;
	wrapText(question, lines);
	for (uint i = 0; i < lines.size(); ++i)
		_font.drawString(_screen, lines[i], kTextX, 51 + i * kTextLineHeight, kTextColor);
	_font.drawString(_screen, answer1, 42, 121, 8);
	_font.drawString(_screen, answer2, 42, 141, 8);
	Graphics::ManagedSurface *button[2] = { _archive.loadImage(kQuizButton1), _archive.loadImage(kQuizButton2) };
	for (int k = 0; k < 2; ++k) {
		if (button[k])
			drawBevelButton(_screen, *button[k], 18, 119 + 20 * k, false);
	}
	_interface->setCursor(Interface::kCursorArrow);

	// b21c:014b / 01e0: a button or the key 1 / 2; DS:cdf4 = 1 for the first
	int chosen = -1;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (chosen < 0 && !shouldQuit()) {
		Common::Event e;
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_KEYDOWN) {
				if (e.kbd.ascii == '1')
					chosen = 1;
				else if (e.kbd.ascii == '2')
					chosen = 0;
			} else if (e.type == Common::EVENT_LBUTTONDOWN) {
				for (int k = 0; k < 2; ++k) {
					if (button[k] && Common::Rect(16, 117 + 20 * k, 18 + button[k]->w + 2,
							119 + 20 * k + button[k]->h + 2).contains(e.mouse))
						chosen = k == 0 ? 1 : 0;
				}
			}
		}
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	delete button[0];
	delete button[1];
	return chosen < 0 ? 0 : chosen;
}

void MiljostrupEngine::showPendingAward() {
	// b06f:13d8: the "?" button only for a negative award with a quiz; then
	// b06f:1229 adds the four values
	_award.active = false;
	_award.talking = false;
	bool help = _award.quiz && _award.values[0] < 0;
	bool helpChosen = runResponse(_award.text, _award.page, _award.values, help);
	for (int k = 0; k < 4; ++k)
		_score[k] += _award.values[k];

	if (help && helpChosen) {
		// b06f:12bf: the quiz, then a second screen with +1 or -1 point and
		// the same in the quiz's category, "Rigtigt!" or "Forkert!"
		int chosen = runQuiz(_award.question, _award.answer1, _award.answer2);
		bool right = chosen == _award.correct; // b21c:061a
		int values[4] = { right ? 1 : -1, 0, 0, 0 };
		if (_award.category >= 1 && _award.category <= 3)
			values[_award.category] = values[0];
		runResponse(readPascalString(_ovr, right ? kTextQuizRight : kTextQuizWrong), Common::String(), values, false);
		for (int k = 0; k < 4; ++k)
			_score[k] += values[k];
	}
	_award.quiz = false;
	debug(1, "Score now %d (energy %d, waste %d, water %d)", _score[0], _score[1], _score[2], _score[3]);
	redrawRoom();
}

void MiljostrupEngine::redrawRoom() {
	// The room runs again (main loop) with entry node 0: the player stays
	const RoomInfo *info = _scene->getRoom();
	// Building the room again binds every object, so each figure gets the
	// state its class's rule gives now (VMT 0x58), e.g. the plants after
	// watering
	_scene->refreshFigures();
	bathroomBuilt();
	schoolBuilt();
	farmBuilt();
	chickenFarmBuilt();
	factoryRoomEntered(); // the room is built again: the stations' rules
	setRoomPalette(info->palette);
	_screen->clear();
	drawInterface();
	_interface->setObject(kNoObject);
	_interface->draw();
	_interface->setCursor(Interface::kCursorArrow);
	_messageTimer = 0;
	_scene->draw(_screen);
	_screen->update();
}

} // End of namespace Miljostrup
