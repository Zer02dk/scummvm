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

#include "common/util.h"

#include "miljostrup/rooms.h"
#include "miljostrup/room_data.h"

namespace Miljostrup {

const LocationInfo *findLocation(int location) {
	for (uint i = 0; i < ARRAYSIZE(kLocations); ++i) {
		if (kLocations[i].number == location)
			return &kLocations[i];
	}
	return nullptr;
}

const RoomInfo *findRoom(int location, int room) {
	for (uint i = 0; i < ARRAYSIZE(kRooms); ++i) {
		if (kRooms[i].location == location && kRooms[i].room == room)
			return &kRooms[i];
	}
	return nullptr;
}

uint getRoomCount() {
	return ARRAYSIZE(kRooms);
}

const RoomInfo *getRoomInfo(uint index) {
	return &kRooms[index];
}

const WalkNode &getNode(const RoomInfo *room, int node) {
	return kWalkNodes[room->firstNode + node - 1];
}

const WalkEdge &getEdge(const RoomInfo *room, int edge) {
	return kWalkEdges[room->firstEdge + edge - 1];
}

const RoomRegion &getRegion(const RoomInfo *room, int region) {
	return kRoomRegions[room->firstRegion + region];
}

const RoomExit &getExit(const RoomInfo *room, int exit) {
	return kRoomExits[room->firstExit + exit];
}

const FigureInfo &getFigure(const RoomInfo *room, int figure) {
	return kFigures[room->firstFigure + figure];
}

const FigureFrame &getFigureFrame(const FigureInfo &figure, int frame) {
	return kFigureFrames[figure.firstFrame + frame];
}

uint16 getSequenceImage(const RoomInfo *room, int sequence, int index) {
	const SequenceInfo &seq = kSequences[room->firstSequence + sequence - 1];
	return kSequenceImages[seq.first + index];
}

uint getSequenceLength(const RoomInfo *room, int sequence) {
	if (sequence < 1 || sequence > room->sequenceCount)
		return 0;
	return kSequences[room->firstSequence + sequence - 1].count;
}

} // End of namespace Miljostrup
