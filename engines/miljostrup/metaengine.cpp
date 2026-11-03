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

#include "common/translation.h"

#include "miljostrup/metaengine.h"
#include "miljostrup/detection.h"
#include "miljostrup/miljostrup.h"

static const ADExtraGuiOptionsMap optionsList[] = {
	{
		GAMEOPTION_ORIGINAL_LOGIN,
		{
			_s("Use the original login"),
			_s("Ask for the user name at the start and keep each user's progress in the original's status files"),
			"original_login",
			false,
			0,
			0
		}
	},
	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

const ADExtraGuiOptionsMap *MiljostrupMetaEngine::getAdvancedExtraGuiOptions() const {
	return optionsList;
}

const char *MiljostrupMetaEngine::getName() const {
	return "miljostrup";
}

Common::Error MiljostrupMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	*engine = new Miljostrup::MiljostrupEngine(syst, desc);
	return Common::kNoError;
}

bool MiljostrupMetaEngine::hasFeature(MetaEngineFeature f) const {
	// Save games (saves.cpp) in the extended format: description, thumbnail,
	// date and play time are added by Engine::saveGameState
	return f == kSavesUseExtendedFormat || f == kSupportsListSaves || f == kSupportsDeleteSave ||
		f == kSavesSupportMetaInfo || f == kSavesSupportThumbnail || f == kSavesSupportCreationDate ||
		f == kSavesSupportPlayTime || f == kSupportsLoadingDuringStartup || f == kSimpleSavesNames;
}

#if PLUGIN_ENABLED_DYNAMIC(MILJOSTRUP)
REGISTER_PLUGIN_DYNAMIC(MILJOSTRUP, PLUGIN_TYPE_ENGINE, MiljostrupMetaEngine);
#else
REGISTER_PLUGIN_STATIC(MILJOSTRUP, PLUGIN_TYPE_ENGINE, MiljostrupMetaEngine);
#endif
