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

#ifndef MILJOSTRUP_ARCHIVE_H
#define MILJOSTRUP_ARCHIVE_H

#include "common/array.h"
#include "common/file.h"
#include "graphics/managed_surface.h"

namespace Miljostrup {

/**
 * The image archive MLJSTRUP.ARK: a u32 image count, a table of count + 1
 * absolute offsets (entry 0 is the count itself), then the images.
 * Images are numbered 1..count, as in the original.
 */
class ImageArchive {
public:
	bool open(const Common::Path &filename);
	uint getImageCount() const { return _offsets.size(); }

	/**
	 * Decode image `number`. The surface's transparent colour is set to the
	 * image's key colour; the original draws every archive image keyed.
	 * Returns nullptr on error. The caller owns the surface.
	 */
	Graphics::ManagedSurface *loadImage(uint number);

private:
	Common::File _file;
	Common::Array<uint32> _offsets; // _offsets[n - 1] = start of image n
	uint32 _fileSize = 0;
};

} // End of namespace Miljostrup

#endif // MILJOSTRUP_ARCHIVE_H
