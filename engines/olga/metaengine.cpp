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

#include "olga/metaengine.h"
#include "olga/detection.h"
#include "olga/olga.h"

const char *OlgaMetaEngine::getName() const {
	return "olga";
}

Common::Error OlgaMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	*engine = new Olga::OlgaEngine(syst, desc);
	return Common::kNoError;
}

bool OlgaMetaEngine::hasFeature(MetaEngineFeature f) const {
	// Save games (saves.cpp) in the extended format: description, thumbnail,
	// date and play time are added by Engine::saveGameState
	return f == kSavesUseExtendedFormat || f == kSupportsListSaves || f == kSupportsDeleteSave ||
		f == kSavesSupportMetaInfo || f == kSavesSupportThumbnail || f == kSavesSupportCreationDate ||
		f == kSavesSupportPlayTime || f == kSupportsLoadingDuringStartup || f == kSimpleSavesNames;
}

void OlgaMetaEngine::getSavegameThumbnail(Graphics::Surface &thumb) {
	if (Olga::g_engine && Olga::g_engine->getRoomThumbnail(thumb))
		return;
	MetaEngine::getSavegameThumbnail(thumb);
}

#if PLUGIN_ENABLED_DYNAMIC(OLGA)
REGISTER_PLUGIN_DYNAMIC(OLGA, PLUGIN_TYPE_ENGINE, OlgaMetaEngine);
#else
REGISTER_PLUGIN_STATIC(OLGA, PLUGIN_TYPE_ENGINE, OlgaMetaEngine);
#endif
