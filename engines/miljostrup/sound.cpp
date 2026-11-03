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

// PC-speaker effects (notes/sound.md).

#include "audio/softsynth/pcspk.h"
#include "common/util.h"

#include "miljostrup/sound.h"
#include "miljostrup/sound_data.h"

namespace Miljostrup {

enum {
	kPitClock = 1193182,
	kBeepDividend = 0xB61481 // 1d26:0078: divisor = 11932801 div freq (tenths of Hz)
};

Sound::Sound() {
}

Sound::~Sound() {
	delete _speaker;
}

bool Sound::init() {
	_speaker = new Audio::PCSpeaker();
	return _speaker->init();
}

void Sound::play(SoundEffectId effect) {
	// 1d26:0078 Beep / 1d26:0000 Pause do nothing while the sound is off,
	// so the effect takes no time either
	if (!_on || !_speaker)
		return;
	for (uint i = 0; i < ARRAYSIZE(kSoundEffects); ++i) {
		if (kSoundEffects[i].address != effect)
			continue;
		for (uint k = 0; k < kSoundEffects[i].count; ++k) {
			const SoundStep &step = kSoundSteps[kSoundEffects[i].first + k];
			uint32 us = step.ms * 1000;
			if (step.freq) {
				float hz = (float)kPitClock / (kBeepDividend / step.freq);
				_speaker->playQueue(Audio::PCSpeaker::kWaveFormSquare, hz, us);
			} else {
				_speaker->playQueue(Audio::PCSpeaker::kWaveFormSilence, 0, us);
			}
		}
	}
}

bool Sound::isPlaying() const {
	return _speaker && _speaker->isPlaying();
}

} // End of namespace Miljostrup
