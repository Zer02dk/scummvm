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

#ifndef MILJOSTRUP_SOUND_H
#define MILJOSTRUP_SOUND_H

#include "common/scummsys.h"

namespace Audio {
class PCSpeaker;
}

namespace Miljostrup {

/** The eight effects of unit 1d26, by the address of their routine. */
enum SoundEffectId {
	kSoundOpen = 0x010f,     // doors open, "allerede åben"
	kSoundClose = 0x0132,    // doors close, waste thrown
	kSoundClick = 0x0155,    // close-up panels
	kSoundClicks = 0x0178,   // close-up panels
	kSoundPositive = 0x01aa, // award > 0, town map
	kSoundZero = 0x0203,     // award = 0
	kSoundNegative = 0x025c, // award < 0
	kSoundTitle = 0x028e     // title page
};

struct SoundStep {
	uint32 freq;  // tenths of a hertz, 0 = silence
	uint16 ms;
};

struct SoundEffect {
	uint16 address;
	uint16 first;
	uint16 count;
};

/**
 * The PC speaker (unit 1d26): square-wave tones. The original busy-waits
 * while an effect plays; play() only queues it, isPlaying() tells when it
 * has finished.
 */
class Sound {
public:
	Sound();
	~Sound();

	bool init();
	/** DS:ce00, on at the start (1d26:0307), toggled by F2. */
	bool isOn() const { return _on; }
	void setOn(bool on) { _on = on; }

	void play(SoundEffectId effect);
	bool isPlaying() const;

private:
	Audio::PCSpeaker *_speaker = nullptr;
	bool _on = true;
};

} // End of namespace Miljostrup

#endif // MILJOSTRUP_SOUND_H
