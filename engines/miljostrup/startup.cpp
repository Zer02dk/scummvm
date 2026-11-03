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

// Start-up screens: title pages, licence, login and the town map shown
// before each location (notes/startup.md).

#include "common/events.h"
#include "common/file.h"
#include "common/savefile.h"
#include "common/system.h"
#include "graphics/framelimiter.h"

#include "miljostrup/miljostrup.h"
#include "miljostrup/startup.h"
#include "miljostrup/startup_data.h"

namespace Miljostrup {

enum {
	kConsoleColumns = 39,
	kConsoleLines = 19,
	kCursorChar = 0xdb,
	kTitleFrames = 200, // idle countdown before a title page moves on
	kMapFrames = 250,   // the same for the town map
	kBlinkFrames = 6    // frames per marker image (HYPOTHESIS)
};

TextConsole::TextConsole(Graphics::ManagedSurface *screen, const GameFont *font) :
	_screen(screen), _font(font) {
}

void TextConsole::clear() {
	_screen->fillRect(Common::Rect(0, 0, 320, 200), _bg);
	_col = 0;
	_line = 0;
}

void TextConsole::putChar(byte c) {
	if (_col >= kConsoleColumns)
		newLine();
	int x = _col * 8 + 4;
	int y = _line * 10 + 5;
	_screen->fillRect(Common::Rect(x, y, x + 8, y + 10), _bg);
	_font->drawChar(_screen, c, x, y, _fg);
	++_col;
}

void TextConsole::write(const Common::String &s) {
	for (uint i = 0; i < s.size(); ++i)
		putChar((byte)s[i]);
}

void TextConsole::writeLine(const Common::String &s) {
	write(s);
	newLine();
}

void TextConsole::newLine() {
	_col = 0;
	_line = (_line + 1) % kConsoleLines;
}

void TextConsole::back() {
	if (_col == 0) {
		_col = kConsoleColumns - 1;
		_line = (_line + kConsoleLines - 1) % kConsoleLines;
	} else {
		--_col;
	}
}

void TextConsole::erase() {
	back();
	putChar(' ');
	back();
}

bool MiljostrupEngine::loadStrings() {
	Common::File ovr;
	if (!ovr.open("MLJSTRUP.OVR"))
		return false;

	_loginStrings.resize(kLoginStringCount);
	for (int i = 0; i < kLoginStringCount; ++i) {
		ovr.seek(kLoginStringOffsets[i]);
		byte len = ovr.readByte();
		char buf[256];
		ovr.read(buf, len);
		_loginStrings[i] = Common::String(buf, len);
	}
	return !ovr.err();
}

int MiljostrupEngine::pollKey() {
	Common::Event e;
	while (g_system->getEventManager()->pollEvent(e)) {
		// An engine addition (the user's wish, 2026-11-03): a mouse click
		// counts as Enter on the start-up screens (HYPOTHESIS: the
		// original reads only the keyboard there)
		if (e.type == Common::EVENT_LBUTTONDOWN)
			return kKeyEnter;
		if (e.type != Common::EVENT_KEYDOWN)
			continue;
		switch (e.kbd.keycode) {
		case Common::KEYCODE_RETURN:
		case Common::KEYCODE_KP_ENTER:
			return kKeyEnter;
		case Common::KEYCODE_ESCAPE:
			return kKeyEscape;
		case Common::KEYCODE_BACKSPACE:
			return kKeyBackspace;
		case Common::KEYCODE_F1:
			return kKeyF1;
		case Common::KEYCODE_F5:
			return kKeyF5;
		case Common::KEYCODE_F6:
			return kKeyF6;
		case Common::KEYCODE_F10:
			return kKeyF10;
		default:
			break;
		}
		byte c = unicodeToCp865(e.kbd.ascii);
		return c ? (int)c : (int)kKeyOther;
	}
	return kKeyNone;
}

int MiljostrupEngine::readKey() {
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!shouldQuit()) {
		int key = pollKey();
		if (key != kKeyNone)
			return key;
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	return kKeyNone;
}

bool MiljostrupEngine::runTitlePage(uint image) {
	// Unit accd: Enter, Esc or Space, or the idle countdown, ends the page.
	// The first page also plays a motif (1d26:028e) that blocks for 2100 ms
	showImage(image, kPaletteA);
	if (image == 1) {
		_screen->update();
		playSound(kSoundTitle);
	}

	Graphics::FrameLimiter limiter(g_system, 25, false);
	for (int frames = 0; frames <= kTitleFrames && !shouldQuit(); ++frames) {
		int key = pollKey();
		if (key == kKeyEnter || key == kKeyEscape || key == ' ')
			break;
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}
	return !shouldQuit();
}

bool MiljostrupEngine::runEndScene() {
	// Phase 8 (5038:0069 -> room 5 = 5042:087d), mode 1 (DS:c48a = 1): a
	// full-screen room (5042:0504, b3d8:0102) with picture 4 and palette A,
	// three figures (rooms.py reads them, test_rooms test_end_room):
	// - (162, 51), delay 1, 3 frames per image, images 5..9 (the boy waves);
	// - (214, 53), delay 3, 3 frames per image, image 12 x16 then 13 (blinks);
	// - (50, 199), delay 1, 3 frames per image, z 50 (5042:0324): image 10
	//   ("Til lykke! ...") Walk(1, 0, -256, 128), message 0x34; then
	//   (5042:0200) SetPos(47, 199), image 11 (the credits) Walk(2, 0, -324,
	//   162), message 0x35, which posts command 0x34: mode 2 (5042:0039).
	// Enter, Esc or Space post command 0x34 at once (5042:019e).
	Graphics::ManagedSurface *background = _archive.loadImage(4);
	Graphics::ManagedSurface *wave[5], *blink[2], *text[2];
	for (int k = 0; k < 5; ++k)
		wave[k] = _archive.loadImage(5 + k);
	blink[0] = _archive.loadImage(12);
	blink[1] = _archive.loadImage(13);
	text[0] = _archive.loadImage(10);
	text[1] = _archive.loadImage(11);
	setPalette(kPaletteA);

	static const int kBlinkLength = 17; // image 12 sixteen times, then 13
	int waveCountdown = 1, waveIndex = 0;
	int blinkCountdown = 3, blinkIndex = 0;
	int textCountdown = 1, textStage = 0, textStep = 0;
	int textX = 50, textY = 199;
	bool done = false;
	Graphics::FrameLimiter limiter(g_system, 25, false);
	while (!done && !shouldQuit()) {
		int key = pollKey();
		if (key == kKeyEnter || key == kKeyEscape || key == ' ')
			break;
		if (runFunctionKey(key, true)) {
			// 5042:00a6: GoTo(1 / 3 / 4, 0) and back: room 5 starts again
			// in the same mode, unless the menu chose something
			if (_menuAction)
				break;
			setPalette(kPaletteA);
			waveCountdown = 1, waveIndex = 0, blinkCountdown = 3, blinkIndex = 0;
			textCountdown = 1, textStage = 0, textStep = 0, textX = 50, textY = 199;
			continue;
		}

		// 24d2:08ac: one animation step every 3 frames
		if (--waveCountdown <= 0) {
			waveCountdown = 3;
			waveIndex = (waveIndex + 1) % 5;
		}
		if (--blinkCountdown <= 0) {
			blinkCountdown = 3;
			blinkIndex = (blinkIndex + 1) % kBlinkLength;
		}
		if (--textCountdown <= 0) {
			textCountdown = 3;
			// a straight walk: step i of n at start + round(dy * i / n)
			static const int kDy[2] = { -256, -324 }, kSteps[2] = { 128, 162 };
			++textStep;
			textY = 199 + (kDy[textStage] * textStep) / kSteps[textStage];
			if (textStep >= kSteps[textStage]) {
				if (textStage == 0) {
					textStage = 1;
					textStep = 0;
					textX = 47;
					textY = 199;
				} else {
					done = true;
				}
			}
		}

		_screen->blitFrom(background->rawSurface(), Common::Point(0, 0));
		Graphics::ManagedSurface *w = wave[waveIndex];
		if (w)
			_screen->transBlitFrom(*w, Common::Point(162, 51), w->getTransparentColor());
		Graphics::ManagedSurface *b = blink[blinkIndex < 16 ? 0 : 1];
		if (b)
			_screen->transBlitFrom(*b, Common::Point(214, 53), b->getTransparentColor());
		Graphics::ManagedSurface *t = text[textStage];
		if (t)
			_screen->transBlitFrom(*t, Common::Point(textX, textY), t->getTransparentColor());

		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	delete background;
	for (int k = 0; k < 5; ++k)
		delete wave[k];
	delete blink[0];
	delete blink[1];
	delete text[0];
	delete text[1];
	return !shouldQuit();
}

void MiljostrupEngine::runEndOfGame() {
	// 5038:0069: a location with all-zero targets; DS:c48a := 1, DS:c488 :=
	// 250. Room 5 alternates: mode 1 the scene above, mode 2 (5042:0550) the
	// credits page, picture 2, for 250 idle calls of >= 4 timer ticks
	// (5042:0615) or until Enter / Esc / Space; F1 / F6 / F10 (the global
	// screens, 5042:00a6, runFunctionKey); it ends through the F10 menu.
	while (!shouldQuit() && !_menuAction) {
		if (!runEndScene() || _menuAction)
			return;
		if (!runEndCredits())
			return;
	}
}

bool MiljostrupEngine::runEndCredits() {
	// Mode 2 (5042:0550): picture 2, 250 idle calls (5042:0615) or Enter,
	// Esc, Space; F1 / F6 / F10 as in mode 1 (5042:0670)
	for (;;) {
		showImage(2, kPaletteA);
		bool again = false;
		Graphics::FrameLimiter limiter(g_system, 25, false);
		for (int frames = 0; frames <= kTitleFrames && !shouldQuit(); ++frames) {
			int key = pollKey();
			if (key == kKeyEnter || key == kKeyEscape || key == ' ')
				break;
			if (runFunctionKey(key, true)) {
				if (_menuAction)
					return !shouldQuit();
				again = true;
				break;
			}
			limiter.delayBeforeSwap();
			_screen->update();
			limiter.startFrame();
		}
		if (!again)
			return !shouldQuit();
	}
}

bool MiljostrupEngine::runLicence() {
	// ad0a:149c, on a blue screen
	Common::File file;
	Common::String owner;
	bool valid = file.open("MLJSTRUP.LIC") && readLicence(file, owner);

	TextConsole console(_screen, &_font);
	console.setBackground(1);
	console.setForeground(8);
	console.clear();

	if (valid) {
		_font.drawString(_screen, _loginStrings[kLicence1], 32, 20, 14);
		_font.drawString(_screen, _loginStrings[kLicence2], 128, 30, 14);
		int x = (320 - owner.size() * 8) / 2;
		_screen->fillRect(Common::Rect(x - 8, 57, x + owner.size() * 8 + 9, 70), 11);
		_font.drawString(_screen, owner, x, 60, 13);
		_font.drawString(_screen, _loginStrings[kPressKey], 172, 190, 7);
	} else {
		console.writeLine(_loginStrings[kLicenceError1]);
		console.writeLine(_loginStrings[kLicenceError2]);
		console.newLine();
		console.writeLine(_loginStrings[kPressKey]);
	}
	readKey();
	return valid && !shouldQuit();
}

bool MiljostrupEngine::readName(TextConsole &console, Common::String &name) {
	// ad0a:05a3: magenta text with a block cursor, at most 30 characters
	name.clear();
	console.setForeground(5);
	console.putChar(kCursorChar);
	bool escaped = false;
	while (!shouldQuit()) {
		int key = readKey();
		if (key == kKeyBackspace) {
			if (!name.empty()) {
				console.erase();
				console.erase();
				console.putChar(kCursorChar);
				name.deleteLastChar();
			}
		} else if (key == kKeyEscape) {
			escaped = true;
			break;
		} else if (key == kKeyEnter) {
			if (!name.empty())
				break;
		} else if (key > 0 && key < 0x100 && name.size() < kMaxNameLength) {
			console.erase();
			console.putChar(key);
			console.putChar(kCursorChar);
			name += (char)key;
		}
	}
	console.erase();
	while (!name.empty() && name.lastChar() == ' ')
		name.deleteLastChar();
	console.setForeground(8);
	return !escaped && !shouldQuit();
}

bool MiljostrupEngine::loginExistingUser(TextConsole &console) {
	// ad0a:0f5e; Esc goes back to the question
	for (;;) {
		console.clear();
		console.setForeground(1);
		console.writeLine(_loginStrings[kHaveName]);
		console.setForeground(8);
		console.newLine();
		console.writeLine(_loginStrings[kEnterName]);

		Common::String name;
		if (!readName(console, name))
			return false;
		_user = _users.find(name);
		if (_user >= 0)
			return true;

		console.newLine();
		console.newLine();
		console.write(_loginStrings[kUnknown1]);
		console.setForeground(1);
		console.writeLine(name);
		console.setForeground(8);
		console.writeLine(_loginStrings[kUnknown2]);
		console.write(_loginStrings[kUnknown3]);
		readKey();
		if (shouldQuit())
			return false;
	}
}

void MiljostrupEngine::newUserHeader(TextConsole &console) {
	// ad0a:073f
	console.clear();
	console.setForeground(1);
	if (_users.count() == 0) {
		console.writeLine(_loginStrings[kWelcomeNew]);
		console.newLine();
	}
	console.writeLine(_loginStrings[kNewUser]);
	console.newLine();
}

bool MiljostrupEngine::loginNewUser(TextConsole &console) {
	// ad0a:0e1f; Esc goes back to the question
	if (_users.count() >= kMaxUsers) {
		newUserHeader(console);
		console.setForeground(8);
		console.writeLine(_loginStrings[kFull1]);
		console.writeLine(_loginStrings[kFull2]);
		console.writeLine(_loginStrings[kFull3]);
		console.writeLine(_loginStrings[kFull4]);
		console.newLine();
		console.writeLine(_loginStrings[kFullPressKey]);
		readKey();
		return false;
	}

	for (;;) {
		newUserHeader(console);
		console.setForeground(8);
		for (int i = kExplain1; i <= kExplain7; ++i)
			console.writeLine(_loginStrings[i]);
		console.newLine();
		console.write(_loginStrings[kEnterNameNew]);
		console.newLine();

		Common::String name;
		bool entered = readName(console, name);
		console.newLine();
		console.newLine();
		if (!entered)
			return false;

		if (_users.find(name) < 0) {
			_user = _users.add(name);
			saveUsers();
			console.writeLine(_loginStrings[kYourName]);
			console.setForeground(1);
			console.write(name);
			console.setForeground(8);
			console.writeLine(_loginStrings[kPeriod]);
			console.writeLine(_loginStrings[kRemember]);
			console.writeLine(_loginStrings[kCreatedPressKey]);
			readKey();
			return !shouldQuit();
		}

		console.writeLine(_loginStrings[kExists1]);
		console.write(_loginStrings[kExists2]);
		console.setForeground(1);
		console.write(name);
		console.setForeground(8);
		console.writeLine(_loginStrings[kExists3]);
		console.write(_loginStrings[kExists4]);
		readKey();
		if (shouldQuit())
			return false;
	}
}

bool MiljostrupEngine::runLogin() {
	// ad0a:15ff / ad0a:1153: white screen, grey text
	loadUsers();
	TextConsole console(_screen, &_font);
	console.setBackground(15);
	console.setForeground(8);
	_user = -1;

	while (!shouldQuit()) {
		if (_users.count() == 0) {
			if (loginNewUser(console))
				return true;
			continue;
		}

		console.clear();
		console.setForeground(1);
		console.writeLine(_loginStrings[kWelcomeQuestion]);
		console.setForeground(8);
		console.newLine();
		console.writeLine(_loginStrings[kQuestion1]);
		console.write(_loginStrings[kQuestion2]);

		int key;
		do {
			key = readKey();
		} while (!shouldQuit() && key != 'J' && key != 'j' && key != 'N' && key != 'n' && key != kKeyEscape);

		if (key == kKeyEscape || shouldQuit())
			return false;
		if ((key == 'J' || key == 'j') ? loginExistingUser(console) : loginNewUser(console))
			return true;
	}
	return false;
}

Common::String MiljostrupEngine::getUserBaseName() const {
	return _targetName + ".bas";
}

void MiljostrupEngine::loadUsers() {
	// The engine keeps its own copy of the user base in the save directory and
	// starts from the game's MLJSTRUP.BAS, so the game files are never written
	_users = UserBase();
	Common::SeekableReadStream *saved = g_system->getSavefileManager()->openForLoading(getUserBaseName());
	if (saved) {
		if (!_users.load(*saved))
			warning("Invalid user base %s", getUserBaseName().c_str());
		delete saved;
		return;
	}

	Common::File file;
	if (file.open("MLJSTRUP.BAS") && !_users.load(file))
		warning("Invalid MLJSTRUP.BAS");
}

void MiljostrupEngine::saveUsers() {
	Common::OutSaveFile *out = g_system->getSavefileManager()->openForSaving(getUserBaseName(), false);
	bool ok = out && _users.save(*out);
	if (out) {
		out->finalize();
		ok = ok && !out->err();
	}
	if (!ok)
		warning("Could not write %s", getUserBaseName().c_str());
	delete out;
}

// The status file: the phase and the eight totals as words, then their
// 16-bit sum (b288:0423 / b288:06eb). The original keeps it in the game
// directory as "MILJØGEM." + the user number in three digits; the engine
// keeps it in the save directory as <target>.NNN and reads an original
// file from the game directory when it has none of its own yet.
enum {
	kStatusWords = 9
};

Common::String MiljostrupEngine::getStatusName() const {
	// Not "<target>.NNN": that is ScummVM's name of save slot NNN
	return _targetName + Common::String::format("-gem.%03d", _user);
}

Common::SeekableReadStream *MiljostrupEngine::openStatus(Common::String &name) {
	name = getStatusName();
	Common::SeekableReadStream *in = g_system->getSavefileManager()->openForLoading(name);
	if (in)
		return in;
	// An older engine stored it as "<target>.NNN", which clashes with save
	// slot NNN: such a file is taken only if it has a status file's size
	Common::String oldName = _targetName + Common::String::format(".%03d", _user);
	in = g_system->getSavefileManager()->openForLoading(oldName);
	if (in && in->size() == (kStatusWords + 1) * 2) {
		name = oldName;
		return in;
	}
	delete in;
	Common::File *file = new Common::File();
	name = Common::String::format("MILJ\x9d" "GEM.%03d", _user);
	if (file->open(Common::Path(Common::String::format("MILJ\xc3\x98" "GEM.%03d", _user))))
		return file;
	delete file;
	return nullptr;
}

bool MiljostrupEngine::startStatus() {
	// Fasekontrol.Start (b288:0068): load the file if it is there, else a
	// new one at the start phase
	Common::String name;
	Common::SeekableReadStream *in = openStatus(name);
	if (!in) {
		resetStatus();
		return true;
	}
	delete in;
	return loadStatus();
}

bool MiljostrupEngine::loadStatus() {
	// b288:07e4; a missing or bad file stops the game (2ae9:003d) with the
	// file's name in the message
	Common::String name;
	Common::SeekableReadStream *in = openStatus(name);
	Common::String error;
	if (!in) {
		error = handlerText(0xb28804fb) + name + handlerText(0xb2880522);
	} else {
		uint16 words[kStatusWords];
		uint16 sum = 0;
		for (int k = 0; k < kStatusWords; ++k) {
			words[k] = in->readUint16LE();
			sum += words[k];
		}
		uint16 check = in->readUint16LE();
		if (in->err() || in->eos())
			error = handlerText(0xb288062b) + name + handlerText(0xb2880649);
		else if (check != sum)
			error = handlerText(0xb28806c1) + name + handlerText(0xb28806c9);
		else {
			_phase = words[0];
			for (int k = 0; k < 8; ++k)
				_totals[k] = (int16)words[k + 1];
		}
		delete in;
	}
	if (!error.empty()) {
		// code page 850 has æ, ø, å, Æ, Ø, Å at the same codes as 865
		GUIErrorMessage(Common::U32String(error, Common::kDos850));
		return false;
	}
	debug(1, "Status %s: phase %d, total %d of %d points", name.c_str(), _phase, _totals[0], _totals[1]);
	return true;
}

void MiljostrupEngine::resetStatus() {
	// b288:01e5: the start phase, the totals 0, saved
	_phase = _startPhase;
	for (int k = 0; k < 8; ++k)
		_totals[k] = 0;
	saveStatus();
}

void MiljostrupEngine::saveStatus() {
	// b288:04d5; without a login (skip_intro) there is no file
	if (_user < 0)
		return;
	Common::OutSaveFile *out = g_system->getSavefileManager()->openForSaving(getStatusName(), false);
	if (out) {
		uint16 sum = 0;
		for (int k = 0; k < kStatusWords; ++k) {
			uint16 word = (uint16)(k == 0 ? _phase : _totals[k - 1]);
			out->writeUint16LE(word);
			sum += word;
		}
		out->writeUint16LE(sum);
		out->finalize();
	}
	if (!out || out->err())
		warning("Could not write %s", getStatusName().c_str());
	delete out;
}

bool MiljostrupEngine::runMapIntro(int location) {
	// Unit 5009: the town map with the location's name blinking at its place
	const MapMarker &marker = kMapMarkers[location - 1];
	Graphics::ManagedSurface *map = _archive.loadImage(14);
	Graphics::ManagedSurface *label = _archive.loadImage(marker.label);
	Graphics::ManagedSurface *blank = _archive.loadImage(marker.blank);
	if (!map || !label || !blank) {
		delete map;
		delete label;
		delete blank;
		return false;
	}

	setPalette(kPaletteA);
	_screen->blitFrom(*map);
	_screen->update();
	playSound(kSoundPositive);
	Graphics::FrameLimiter limiter(g_system, 25, false);
	for (int frames = 0; frames <= kMapFrames && !shouldQuit(); ++frames) {
		int key = pollKey();
		if (key == kKeyEnter || key == kKeyEscape || key == ' ')
			break;

		// Backgrounds are drawn opaque, sprites keyed
		_screen->blitFrom(*map);
		Graphics::ManagedSurface *frame = (frames / kBlinkFrames) % 2 ? blank : label;
		_screen->transBlitFrom(*frame, Common::Point(marker.x, marker.y), frame->getTransparentColor());

		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	delete map;
	delete label;
	delete blank;
	return !shouldQuit();
}

} // End of namespace Miljostrup
