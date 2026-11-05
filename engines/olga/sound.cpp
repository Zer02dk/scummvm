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

// The PC speaker, the game's only sound (unit Speaker 1c46, notes/sound.md).

#include "audio/softsynth/pcspk.h"

#include "olga/olga.h"

namespace Olga {

namespace {

enum {
	kPitClock = 1193182,
	kBeepDividend = 0xb61481 // Speaker_Beep: divisor = 11932801 div freq (tenths of Hz)
};

struct Tone {
	uint32 freq; // tenths of a hertz
	uint16 ms;   // Crt.Delay
};

// Speaker_0045, 0068, 008b
const Tone kClick[] = { { 0xcb2, 6 }, { 0x1964, 6 } };
const Tone kDoor[] = { { 0x659, 10 }, { 0xcb2, 10 } };
const Tone kDevil[] = { { 0xdc0, 6 } };

} // End of anonymous namespace

bool OlgaEngine::initSound() {
	_speaker = new Audio::PCSpeaker();
	return _speaker->init();
}

void OlgaEngine::playSound(SoundEffect effect) {
	// The original busy-waits while the tones play; here they are queued
	if (!_speaker)
		return;
	const Tone *tones = kClick;
	uint count = ARRAYSIZE(kClick);
	if (effect == kSoundDoor) {
		tones = kDoor;
		count = ARRAYSIZE(kDoor);
	} else if (effect == kSoundDevil) {
		tones = kDevil;
		count = ARRAYSIZE(kDevil);
	}
	for (uint i = 0; i < count; i++) {
		float hz = (float)kPitClock / (kBeepDividend / tones[i].freq);
		_speaker->playQueue(Audio::PCSpeaker::kWaveFormSquare, hz, tones[i].ms * 1000);
	}
}

} // End of namespace Olga
