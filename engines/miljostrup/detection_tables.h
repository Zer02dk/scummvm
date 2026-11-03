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

namespace Miljostrup {

const PlainGameDescriptor miljostrupGames[] = {
	{ "miljostrup", "Milj\xC3\xB8strup" },
	{ 0, 0 }
};

const ADGameDescription gameDescriptions[] = {
	{
		"miljostrup",
		nullptr,
		AD_ENTRY2s("MLJSTRUP.EXE", "16392dffbeaa7f89cb2a4df8d1a2175a", 204368,
				   "MLJSTRUP.OVR", "8dbca34b96202c985efe8b575ca293b9", 383486),
		Common::DA_DNK,
		Common::kPlatformDOS,
		ADGF_UNSTABLE,
		GUIO3(GUIO_NOSPEECH, GUIO_NOMIDI, GAMEOPTION_ORIGINAL_LOGIN)
	},

	AD_TABLE_END_MARKER
};

} // End of namespace Miljostrup
