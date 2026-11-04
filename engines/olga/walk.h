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

#ifndef OLGA_WALK_H
#define OLGA_WALK_H

#include "common/array.h"
#include "common/rect.h"

#include "olga/scene.h"

namespace Olga {

/**
 * Olga's walk to a point of the room (notes/walking.md): from the edge or
 * point nearest to her feet along the walk graph to the point nearest the
 * target, as moves for her script. Returns false if there is nothing to
 * walk (no graph, or she is already there).
 */
bool planWalk(const ScnFile &scn, const Scene &scene, const SceneObject &olga,
			  const Common::Point &target, Common::Array<SceneObject::Move> &moves);

} // End of namespace Olga

#endif // OLGA_WALK_H
