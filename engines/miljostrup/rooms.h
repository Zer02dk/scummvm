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

#ifndef MILJOSTRUP_ROOMS_H
#define MILJOSTRUP_ROOMS_H

#include "common/scummsys.h"

namespace Miljostrup {

enum {
	kLocationCount = 7,
	kFirstRoom = 5 // rooms 1..4 are the function-key screens
};

struct LocationInfo {
	byte number;
	byte startRoom;
	byte startNode;
};

struct WalkNode {
	int16 x, y;
};

struct WalkEdge {
	byte a, b; // node numbers, 1-based
};

struct RoomRegion {
	uint16 object;
	int16 x1, y1, x2, y2; // room coordinates, inclusive
	byte node;            // walk node the player goes to
};

struct RoomExit {
	uint16 object; // passage or door
	byte room;     // target room
	byte node;     // entry node in the target room
	byte door;     // 1 for a door, which can be closed
	byte open;     // initial state of a door
	uint16 partner; // door on the other side that opens and closes with it, 0 = none
};

struct SequenceInfo {
	uint16 first; // index into kSequenceImages
	uint16 count;
};

struct FigureInfo {
	int16 x, y;
	byte delay;   // frames until the first animation step
	byte ticks;   // frames per animation step
	byte player;
	uint16 object; // bound object, 0 if none
	byte state;    // start state set by the object's class, 0 = the usual rule
	uint16 firstFrame;
	byte frameCount;
};

struct FigureFrame {
	byte state;
	byte sequence; // 1-based within the room
	int16 dx, dy;  // offset of the image from the figure's position
	int16 z;       // added to the bottom edge for the draw order
};

struct RoomInfo {
	byte location;
	byte room;
	uint16 image;   // background image in MLJSTRUP.ARK, 0 for close-ups drawn by code
	byte palette;   // kPaletteA or kPaletteB
	int16 x, y;     // screen position of the background
	// Rooms without a player have no walk data (all counts 0)
	uint16 firstNode;
	byte nodeCount;
	uint16 firstEdge;
	byte edgeCount;
	uint16 firstRegion;
	byte regionCount;
	uint16 firstExit;
	byte exitCount;
	uint16 firstSequence;
	byte sequenceCount;
	uint16 firstFigure;
	byte figureCount;
	byte hStep, vStep;    // player step in pixels, horizontal and vertical walks
	int16 footX, footY;   // figure position + foot = point on the walk graph
};

const LocationInfo *findLocation(int location);
const RoomInfo *findRoom(int location, int room);
uint getRoomCount();
const RoomInfo *getRoomInfo(uint index);

const WalkNode &getNode(const RoomInfo *room, int node);
const WalkEdge &getEdge(const RoomInfo *room, int edge);
const RoomRegion &getRegion(const RoomInfo *room, int region);
const RoomExit &getExit(const RoomInfo *room, int exit);
const FigureInfo &getFigure(const RoomInfo *room, int figure);
const FigureFrame &getFigureFrame(const FigureInfo &figure, int frame);
/** ARK image `index` (0-based) of sequence `sequence` (1-based) of a room. */
uint16 getSequenceImage(const RoomInfo *room, int sequence, int index);
uint getSequenceLength(const RoomInfo *room, int sequence);

} // End of namespace Miljostrup

#endif // MILJOSTRUP_ROOMS_H
