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

#include "common/debug.h"
#include "common/textconsole.h"

#include "miljostrup/archive.h"
#include "miljostrup/detection.h"

namespace Miljostrup {

enum {
	kHeaderSize = 0x14,
	kCompressionRaw = 0,
	kCompressionRle = 1
};

bool ImageArchive::open(const Common::Path &filename) {
	if (!_file.open(filename))
		return false;

	_fileSize = _file.size();
	uint32 count = _file.readUint32LE();
	if (count == 0 || (count + 1) * 4 > _fileSize) {
		warning("ImageArchive: bad image count %u", count);
		_file.close();
		return false;
	}

	_offsets.resize(count);
	for (uint32 i = 0; i < count; ++i)
		_offsets[i] = _file.readUint32LE();

	debugC(1, kDebugResources, "ImageArchive: %u images", count);
	return true;
}

Graphics::ManagedSurface *ImageArchive::loadImage(uint number) {
	if (number < 1 || number > _offsets.size()) {
		warning("ImageArchive: no image %u", number);
		return nullptr;
	}

	uint32 start = _offsets[number - 1];
	uint32 end = number < _offsets.size() ? _offsets[number] : _fileSize;
	if (end < start + kHeaderSize || end > _fileSize) {
		warning("ImageArchive: bad bounds for image %u", number);
		return nullptr;
	}

	_file.seek(start);
	byte header[kHeaderSize];
	_file.read(header, kHeaderSize);
	uint16 width = READ_LE_UINT16(header);
	uint16 height = READ_LE_UINT16(header + 2);
	byte compression = header[0x0A];
	byte key = header[0x0C];

	Common::Array<byte> data;
	data.resize(end - start - kHeaderSize);
	_file.read(data.data(), data.size());

	Graphics::ManagedSurface *surface = new Graphics::ManagedSurface();
	surface->create(width, height, Graphics::PixelFormat::createFormatCLUT8());
	surface->setTransparentColor(key);

	// Rows are stored with an even length; the pad pixel is dropped
	uint stride = width + (width & 1);
	uint pos = 0;
	bool ok = true;

	for (uint y = 0; y < height && ok; ++y) {
		byte *dest = (byte *)surface->getBasePtr(0, y);

		if (compression == kCompressionRaw) {
			if (pos + stride > data.size()) {
				ok = false;
				break;
			}
			memcpy(dest, &data[pos], width);
			pos += stride;
		} else if (compression == kCompressionRle) {
			// PackBits: c < 0x80 copies c + 1 bytes, c > 0x80 repeats the next
			// byte 257 - c times, 0x80 does nothing
			uint x = 0;
			while (x < stride) {
				if (pos >= data.size()) {
					ok = false;
					break;
				}
				byte c = data[pos++];
				if (c < 0x80) {
					if (pos + c + 1 > data.size()) {
						ok = false;
						break;
					}
					for (uint i = 0; i <= c; ++i, ++x) {
						byte v = data[pos++];
						if (x < width)
							dest[x] = v;
					}
				} else if (c > 0x80) {
					if (pos >= data.size()) {
						ok = false;
						break;
					}
					byte v = data[pos++];
					for (uint i = 0; i < 257u - c; ++i, ++x) {
						if (x < width)
							dest[x] = v;
					}
				}
			}
		} else {
			warning("ImageArchive: image %u has unknown compression %d", number, compression);
			ok = false;
		}
	}

	if (!ok) {
		warning("ImageArchive: image %u is truncated", number);
		delete surface;
		return nullptr;
	}

	debugC(2, kDebugResources, "ImageArchive: image %u %dx%d, compression %d, key %d",
		number, width, height, compression, key);
	return surface;
}

} // End of namespace Miljostrup
