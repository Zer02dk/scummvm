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

// The close-ups with answer buttons (location type kLocPopup, overlay unit
// 53cd): RADIATOR, TASKVALG, LOEFT, VOLUME (notes/situation-logic.md).

#include "graphics/paletteman.h"

#include "olga/locations_data.h"
#include "olga/olga.h"

namespace Olga {

namespace {

// Ovr53cd_0001: the room view (4, 14)-(155, 161) holds the 152x148 picture
const Common::Point kPopupView(4, 14);

// Ovr53cd_0294 / 020e: answer k (1..4) is image k + 1 of the LBM, at
// x 164 + 80 * col, y 17 + 76 * row, with a 3-pixel bevel; command 0x3c + k
Common::Point answerPos(int k) {
	return Common::Point(0xa4 + 0x50 * ((k - 1) % 2), 0x11 + 0x4c * ((k - 1) / 2));
}

const int kAnswerBevel = 3;

} // End of anonymous namespace

bool OlgaEngine::gotoPopup(int number) {
	const LocationDef &loc = kLocations[number - 1];
	if (!_scene.load(loc, 0))
		return false;
	_location = number;
	_kind = kLocPopup;
	_nextLocation = 0;
	_pending = false;
	_textTimer = 0;
	memset(_controlOff, 0, sizeof(_controlOff));
	for (uint i = 0; i < _scene.scn().objects.size(); i++)
		refresh(_scene.scn().objects[i].hotspot.id);
	_scene.settle();
	_popupAnswers = MIN(4, loc.answers);
	for (int k = 1; k <= _popupAnswers; k++) {
		const LbmImage *img = _scene.image(k + 1);
		if (img) {
			_answerImages[k - 1].surface.copyFrom(img->surface);
			_answerImages[k - 1].keyed = false; // Gui_02d5 draws button images opaque
		}
	}
	g_system->getPaletteManager()->setPalette(_scene.palette(), 0, 256);
	_roomView.create(152, 148, Graphics::PixelFormat::createFormatCLUT8());
	drawPopup();
	setInput(true);
	return true;
}

void OlgaEngine::drawPopup() {
	// Room_Init clears the screen; Ovr53cd_0130 draws the grey background
	// with the frame around the picture and the stripes at the bottom; the
	// title goes in the message line (Room_ShowText, colour 14)
	_screen->clear(0);
	_screen->fillRect(Common::Rect(0, 9, 320, 174), 7);
	_screen->frameRect(Common::Rect(3, 13, 157, 163), 0);
	_screen->hLine(0, 174, 319, 0);
	_screen->fillRect(Common::Rect(0, 175, 320, 177), 8);
	_screen->hLine(0, 177, 319, 15);
	_screen->fillRect(Common::Rect(0, 178, 320, 183), 7);
	_screen->hLine(0, 183, 319, 0);
	_screen->fillRect(Common::Rect(0, 184, 320, 186), 8);
	_screen->hLine(0, 186, 319, 15);
	_screen->fillRect(Common::Rect(0, 187, 320, 200), 7);
	for (int k = 1; k <= _popupAnswers; k++) {
		Common::Point p = answerPos(k);
		drawButton(_answerImages[k - 1], p.x, p.y, false, kAnswerBevel);
	}
	_font.drawCentred(*_screen, _exe.readString(kLocations[_location - 1].titleSeg,
		kLocations[_location - 1].titleOff), 0, 14);
}

int OlgaEngine::popupHitTest(const Common::Point &p) {
	for (int k = 1; k <= _popupAnswers; k++) {
		Common::Point a = answerPos(k);
		const Graphics::ManagedSurface &s = _answerImages[k - 1].surface;
		int e = 1 + kAnswerBevel;
		if (Common::Rect(a.x - e, a.y - e, a.x + s.w + e, a.y + s.h + e).contains(p))
			return k;
	}
	return 0;
}

void OlgaEngine::popupAnswer(int choice) {
	// Popup_Answer (53cd:04fc) and the four answer handlers; a wrong answer
	// sets the pending devil (DS:2592), some right ones the praise (2593)
	switch (_location) {
	case 10: // RADIATOR (53cd:030f): 3 and 4 are wrong
		if (choice == 3 || choice == 4) {
			_flags[103] = 0;
			_flags[104] &= ~4;
			_pendingDevil = true;
		} else {
			_flags[103] = 1;
			_flags[104] |= 4;
			_pendingPraise = true;
		}
		setExit(1, 3);
		break;
	case 24: // TASKVALG (53cd:038e): only 3 is right
		if (choice == 3)
			_flags[186] |= 4;
		else
			_pendingDevil = true;
		setExit(3, 1);
		break;
	case 28: // LOEFT (53cd:03f5): 1 and 4 are right, two ways
		if (choice == 1)
			_flags[188] = 3;
		else if (choice == 4)
			_flags[188] = 2;
		else
			_pendingDevil = true;
		setExit(7, 1);
		break;
	case 29: // VOLUME (53cd:0483): 1 and 2 are right
		if (choice == 1 || choice == 2) {
			_flags[173] |= 1;
			_flags[174] |= 1;
			_pendingPraise = true;
		} else {
			_pendingDevil = true;
		}
		setExit(19, 1);
		break;
	default:
		break;
	}
}

} // End of namespace Olga
