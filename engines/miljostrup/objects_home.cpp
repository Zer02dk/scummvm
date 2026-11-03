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

// Class-specific verb handlers and figure rules of the home's objects,
// written from notes/bedroom.md. Handlers are identified by the address of
// the original method; texts are read from MLJSTRUP.OVR (handlers_data.h).

#include "common/debug.h"

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/rooms.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/handlers_data.h"
#include "miljostrup/response_data.h"
#include "miljostrup/verbs_data.h"

namespace Miljostrup {

enum {
	kBed = 115,          // vandseng, DS:cb02: +0x2f 0 water bed in use, 1 replaced
	kOtherBed = 134,     // seng (basement, proxy 182)
	kWindow = 116,
	kCurtains = 117,
	kWasteBasket = 118,
	kBedSwitch = 123,
	kLightSwitch = 124,
	kLaundry = 143,
	kBasket = 158,
	// living room (notes/livingroom.md)
	kBlind = 105,
	kCeilingSwitch = 125,
	kFather = 135,       // DS:cbfa: +0x2f freezing, +0x30 sweater, +0x31 fan heater
	kHeaterProxy = 249,  // DS:cabe: the fan heater in front of father, hidden at the start
	kRadiator = 171,
	kFloorLamp = 183,
	kThermometer = 220,      // the place on the wall, DS:cc6a (+0x31 hung up)
	kThermometerItem = 222,  // termometer from the shop
	kLivingWindow = 223,     // DS:cc72, +0x2f 100 leaky, else the item that seals it
	kSealingStrips = 224,
	kTape = 225,
	kVideo = 228,
	kTV = 287,
	kBedroomDoor = 107
};

// A lamp's award for a new bulb (its VMT 0x64: a000:92bd floor lamp 183,
// a89e desk lamp 200, ad0a bedroom ceiling lamp 201, b12d living-room
// ceiling lamp 221). Ordinary bulb: normal[0] + watt + normal[1..3];
// low-energy: low[0] + watt + low[1] + low[2] + watt + low[3] + low[4] +
// equal[i] + low[5] + verdict[i] for 9 / 11 / 15 W (i = 0 / 1 / 2).
struct LampAward {
	uint16 lamp;
	uint32 normal[4];
	uint32 low[6];
	uint32 equal[3], verdict[3];
	int lowPoints[3];
	uint32 question, answer1, answer2; // 0: no quiz
	uint32 page;
};

static const LampAward kLampAwards[] = {
	{ 183, { 0xa0009101, 0xa000911b, 0xa0009138, 0xa0009177 },
	  { 0xa0009206, 0xa0009215, 0xa000923b, 0xa000923f, 0xa000926e, 0xa000927a },
	  { 0xa0009197, 0xa00091b0, 0xa00091e4 }, { 0xa000919a, 0xa00091b3, 0xa00091e7 }, { 10, 6, 4 },
	  0xa0009287, 0xa00092aa, 0xa00092b2, 0xa00092ba },
	{ 200, { 0xa000a6e3, 0xa000a6fd, 0xa000a71b, 0xa000a75a },
	  { 0xa000a7df, 0xa000a7ee, 0xa000a815, 0xa000a819, 0xa000a848, 0xa000a854 },
	  { 0xa000a77a, 0xa000a789, 0xa000a7bd }, { 0xa000a77d, 0xa000a78c, 0xa000a7c0 }, { 10, 6, 4 },
	  0xa000a861, 0xa000a87d, 0xa000a88d, 0xa000a89b },
	{ 201, { 0xa000ab72, 0xa000ab8c, 0xa000aba7, 0xa000abe6 },
	  { 0xa000ac88, 0xa000ac97, 0xa000acbb, 0xa000acbf, 0xa000acee, 0xa000acfa },
	  { 0xa000ac06, 0xa000ac42, 0xa000ac79 }, { 0xa000ac09, 0xa000ac45, 0xa000ac7c }, { 2, 7, 10 },
	  0, 0, 0, 0xa000ad07 },
	{ 221, { 0xa000af9c, 0xa000afb6, 0xa000afd1, 0xa000b010 },
	  { 0xa000b0ab, 0xa000b0ba, 0xa000b0de, 0xa000b0e2, 0xa000b111, 0xa000b11d },
	  { 0xa000b030, 0xa000b061, 0xa000b07a }, { 0xa000b033, 0xa000b064, 0xa000b07d }, { 7, 10, 6 },
	  0, 0, 0, 0xa000b12a }
};

// DS:ccb6 (class ac6c, VMT DS:ba8e): VMT 0x0c ac6c:0023 is a lamp, VMT 0x10
// ac6c:0057 is a bulb pack (class afdc: watt +0x2f, count +0x31, low-energy +0x33)
static bool isLamp(int id) {
	return id == 200 || id == 183 || id == 201 || id == 221;
}

static bool isBulb(int id) {
	return id == 278 || id == 202 || id == 281 || id == 252 || id == 272 || id == 275;
}

static const ProxyObject *findProxy(int location, int id) {
	for (uint i = 0; i < ARRAYSIZE(kProxies); ++i) {
		if (kProxies[i].location == location && kProxies[i].id == id)
			return &kProxies[i];
	}
	return nullptr;
}

Common::String MiljostrupEngine::handlerText(uint32 address) {
	for (uint i = 0; i < ARRAYSIZE(kHandlerTexts); ++i) {
		if (kHandlerTexts[i].address == address)
			return readPascalString(_ovr, kHandlerTexts[i].offset);
	}
	warning("No text at %04x:%04x", address >> 16, address & 0xffff);
	return Common::String();
}

int MiljostrupEngine::getField(int object, int field) const {
	uint32 key = object << 16 | field;
	return _flags.contains(key) ? _flags[key] : 0;
}

void MiljostrupEngine::setField(int object, int field, int value) {
	_flags[object << 16 | field] = value;
}

bool MiljostrupEngine::isFatherCold() const {
	// a000:8dd3: not warmed (+0x30, +0x31) and the radiator (DS:cbca) at 20 degrees or less
	return !getField(kFather, 0x30) && !getField(kFather, 0x31) && getFlag(kRadiator) <= 20;
}

void MiljostrupEngine::initExitDoors(int location) {
	// exits that are doors (13ec:0177, the slaughterhouse's: closed)
	for (uint i = 0; i < ARRAYSIZE(kExitDoors); ++i) {
		if (kExitDoors[i].location == location)
			_doors[kExitDoors[i].object] = kExitDoors[i].open;
	}
}

void MiljostrupEngine::resetObjectState(int location) {
	_flags.clear();
	if (location == 1) {
		for (uint i = 0; i < ARRAYSIZE(kHomeStartFlags); ++i)
			setField(kHomeStartFlags[i].id, kHomeStartFlags[i].field, kHomeStartFlags[i].value);
		initWashingPowder();
	}
	_interface->clearNameOverrides();
	if (location == 2) {
		for (uint i = 0; i < ARRAYSIZE(kSchoolStartFlags); ++i)
			setField(kSchoolStartFlags[i].id, kSchoolStartFlags[i].field, kSchoolStartFlags[i].value);
		initSchool();
	}
	if (location == 3) {
		for (uint i = 0; i < ARRAYSIZE(kFarmStartFlags); ++i)
			setField(kFarmStartFlags[i].id, kFarmStartFlags[i].field, kFarmStartFlags[i].value);
		initFarm();
	}
	if (location == 4)
		initChickenFarm();
	if (location == 5)
		initSlaughterhouse();
	if (location == 6)
		initFactory();
	if (location == 7)
		initSupermarket();
	if (location == 1) {
		updateHolderName(149); // bathroom: empty at the start
		updateHolderName(151); // kitchen: the kitchen roll (a000:5da8)
	}
	for (uint i = 0; i < ARRAYSIZE(kProxies); ++i) {
		if (kProxies[i].location == location)
			setFlag(kProxies[i].id, kProxies[i].shown);
	}
	_greenhouseStage = 0;
	_toasterTimer = 0;
	_basket.clear();
	_shopFetching = 0;
	_inputOff = false;

	// The location's object unit puts some objects straight into the
	// inventory (the home: the coins); the list is emptied when a location
	// ends (b06f:0e35)
	_inventory.clear();
	_inventoryPage = 1;
	for (uint i = 0; i < ARRAYSIZE(kStartInventory); ++i) {
		if (kStartInventory[i].location == location)
			_inventory.push_back(kStartInventory[i].id);
	}
}

const LocationTarget *MiljostrupEngine::getLocationTarget() const {
	for (uint i = 0; i < ARRAYSIZE(kLocationTargets); ++i) {
		if (kLocationTargets[i].location == _location)
			return &kLocationTargets[i];
	}
	return nullptr;
}

void MiljostrupEngine::useExit() {
	// 13ec:07ce: enough points (manager VMT 0x2c = b06f:141b: points >=
	// threshold) -> the room ends (command 3) and PhaseCompleted (VMT 0x14);
	// else "Du mangler " + (threshold - points) + " points. "
	const LocationTarget *target = getLocationTarget();
	int threshold = target ? target->threshold : 0;
	if (_score[0] < threshold) {
		say(readPascalString(_exe, kTextExitMissing) + Common::String::format("%d", threshold - _score[0]) +
			readPascalString(_exe, kTextExitPoints), 14, true);
		return;
	}
	// b06f:107e: the location's points and maxima are added to the game's
	// totals (Fasekontrol +0x105.., b288:00dd), the phase goes up
	// (b288:01c6), the status file is saved (b288:04d5) and the location's
	// loop ends
	int values[8] = { _score[0], target ? target->maxPoints : 0, _score[1], target ? target->maxEnergy : 0,
		_score[2], target ? target->maxWaste : 0, _score[3], target ? target->maxWater : 0 };
	for (int k = 0; k < 8; ++k)
		_totals[k] += values[k];
	++_phase;
	saveStatus();
	_phaseDone = true;
	debug(1, "Phase completed: now phase %d, total %d of %d points", _phase, _totals[0], _totals[1]);
}

void MiljostrupEngine::updateHolderName(int holder) {
	// The paper holders' GetName (VMT 0x14 = a000:5c52 / 5edf) and
	// GetPronoun (VMT 0x1c = 5cbb / 5f48): with something on it (+0x2f not
	// 100) the holder is called by that object's name
	int on = getFlag(holder);
	if (on == 100) {
		_interface->removeNameOverride(holder);
	} else {
		_interface->setNameOverride(holder, _interface->getObjectName(on));
	}
}

void MiljostrupEngine::giveAllItems() {
	// Testing aid, not in the original: every object of the location that
	// has an inventory icon goes into the inventory, and the rooms show them
	// as taken: proxies hidden (as after Tag, ac7f:025c), the bedroom's
	// basket and wastebasket emptied (as their Tag does). Objects with a
	// figure of their own in a room (the washing powder, the schedule) are
	// used where they are and stay there. Bulb packs get 2 bulbs each.
	Common::HashMap<uint16, bool> shown;
	for (uint r = 0; r < getRoomCount(); ++r) {
		const RoomInfo *room = getRoomInfo(r);
		if (room->location != _location)
			continue;
		for (int f = 0; f < room->figureCount; ++f)
			shown[getFigure(room, f).object] = true;
	}
	Common::Array<uint16> items;
	getCarriableObjects(_location, items);
	for (uint i = 0; i < items.size(); ++i) {
		if (isCarried(items[i]) || shown.contains(items[i]))
			continue;
		_inventory.push_back(items[i]);
		if (isBulb(items[i]))
			setField(items[i], 0x31, 2);
	}
	for (uint i = 0; i < ARRAYSIZE(kProxies); ++i) {
		if (kProxies[i].location == _location && isCarried(kProxies[i].other)) {
			setFlag(kProxies[i].id, 0);
			refreshObject(kProxies[i].id);
		}
	}
	if (_location == 3)
		giveFarmItems();
	if (_location == 1) {
		// a000:75ca: the basket without the laundry; a000:778e: the
		// wastebasket without its three things
		if (getFlag(kBasket) && isCarried(kLaundry)) {
			setFlag(kBasket, 0);
			_interface->setNameOverride(kBasket, readPascalString(_ovr, kNameKurv));
			refreshObject(kBasket);
		}
		if (!getFlag(kWasteBasket) && isCarried(179) && isCarried(180) && isCarried(181)) {
			setFlag(kWasteBasket, 1);
			refreshObject(kWasteBasket);
		}
	}
	_inventoryPage = 1;
	_interface->drawInventory();
	debug(1, "Gave all %d items", (int)items.size());
}

int MiljostrupEngine::getDoorPartner(int object) const {
	for (uint i = 0; i < getRoomCount(); ++i) {
		const RoomInfo *room = getRoomInfo(i);
		if (room->location != _location)
			continue;
		for (int e = 0; e < room->exitCount; ++e) {
			if (getExit(room, e).object == object)
				return getExit(room, e).partner;
		}
	}
	return 0;
}

int MiljostrupEngine::getProxyTarget(int object) const {
	const ProxyObject *proxy = findProxy(_location, object);
	return proxy ? proxy->other : 0;
}

bool MiljostrupEngine::takesSecondObject(int object) const {
	// ac7f:0146: a proxy answers with the referenced object's VMT 0x20
	if (int target = getProxyTarget(object))
		object = target;
	for (uint i = 0; i < ARRAYSIZE(kSecondObjectFlags); ++i) {
		if (kSecondObjectFlags[i].location == _location && kSecondObjectFlags[i].id == object)
			return true;
	}
	return false;
}

bool MiljostrupEngine::runProxyVerb(int verb, int object, int second) {
	int target = getProxyTarget(object);
	if (!target)
		return false;
	// The kitchen's proxy subclasses override Tag (a000:8c04, 837e, 8c9d,
	// a21a) and then call the proxy's own Tag (runKitchenProxyTag returns
	// false for that); they come before the check below, which would hand
	// them to the class dispatch
	if (verb == kVerbTake && _location == 1 && runKitchenProxyTag(object))
		return true;
	bool kitchenTag = verb == kVerbTake && _location == 1 && isKitchenTagProxy(object);
	// Other proxy subclasses with their own method for this verb (the shop's
	// goods, e.g. Tag 9909:13d2) go through the normal class dispatch
	if (const ObjectInfo *info = findObject(_location, object)) {
		uint32 handler = getObjectClass(info).handler[verb - kVerbFirst];
		if ((handler >> 16) != 0xac7f && !kitchenTag)
			return false;
	}
	if (verb == kVerbTake) {
		// ac7f:025c: hide the proxy (VMT 0x64 = ac7f:044f), the room takes the
		// referenced object (VMT 0x50 = 229f:0035)
		setFlag(object, 0);
		refreshObject(object);
		takeObject(target);
	} else if (verb == kVerbUse || verb == kVerbGive) {
		// ac7f:01b9 / 02b4: "Du har dem ikke!" or "Du har " + pronoun + " ikke!"
		const ObjectInfo *info = findObject(_location, target);
		Common::String pronoun = info ? readPascalString(_ovr, info->pronoun) : Common::String();
		if (pronoun == readPascalString(_ovr, kTextProxyDe))
			say(readPascalString(_ovr, kTextProxyNotThem), 14, true);
		else
			say(readPascalString(_ovr, kTextProxyYouHave) + pronoun + readPascalString(_ovr, kTextProxyNot), 14, true);
	} else {
		// The other verbs call the referenced object's verb method
		executeVerb(verb, target, second);
	}
	return true;
}

bool MiljostrupEngine::isObjectEnabled(int object) const {
	// ac7f:03f8: a proxy's region is there while it is shown (+0x2f)
	bool enabled;
	if (_location == 2 && isSchoolObjectEnabled(object, enabled))
		return enabled;
	if (_location == 3 && isFarmObjectEnabled(object, enabled))
		return enabled;
	if (_location == 4 && isChickenFarmObjectEnabled(object, enabled))
		return enabled;
	if (_location == 5 && isSlaughterObjectEnabled(object, enabled))
		return enabled;
	if (_location == 6 && isFactoryObjectEnabled(object, enabled))
		return enabled;
	if (_location == 7 && isSuperObjectEnabled(object, enabled))
		return enabled;
	if (findProxy(_location, object))
		return getFlag(object) != 0 && (_location != 1 || isKitchenProxyOpen(object));
	if (_location == 1 && isKitchenObjectEnabled(object, enabled))
		return enabled;
	// a000:1f8e: the water-bed switch is there only while the bed is in use
	if (_location == 1 && object == kBedSwitch)
		return getFlag(kBed) == 0;
	// a000:6890: the little brother's region by his state (+0x30)
	if (_location == 1 && object == 154)
		return isLittleBrotherRegionOn();
	return true;
}

void MiljostrupEngine::adjustRegion(int object, Common::Rect &r) const {
	if (_location == 2)
		adjustSchoolRegion(object, r);
	if (_location == 7)
		adjustSuperRegion(object, r);
	if (_location != 1)
		return;
	// a000:2340 narrows the curtains' region while they hang open: x2 = x1 + 17
	// while blowing, x1 + 11 while still. Drawn (state 2) it keeps the
	// rectangle it has; in the original the award that comes with drawing
	// them rebuilds the room, so the region is back to full width.
	if (object == kCurtains && !getFlag(kCurtains))
		r.right = r.left + (getFlag(kWindow) ? 17 : 11);
	// a000:154d: the roller blind reaches down to screen y 95 when down, 45
	// when up (widget rectangles are in screen coordinates, b3d8:01e3 adds
	// the room's position)
	if (object == kBlind) {
		const RoomInfo *room = findRoom(_location, _room);
		r.bottom = (getFlag(kBlind) ? 0x5f : 0x2d) - (room ? room->y : 0);
	}
	// a000:67d8 moves the little brother's region, same size, to screen
	// (160, 77) while he waits (2) and (75, 77) while he brushes his teeth (5)
	if (object == 154) {
		const RoomInfo *room = findRoom(_location, _room);
		int state = getField(object, 0x30);
		if (room && (state == 2 || state == 5))
			r.moveTo((state == 2 ? 160 : 75) - room->x, 77 - room->y);
	}
}

int MiljostrupEngine::getFigureState(int object, bool &visible) const {
	// The object's VMT 0x58 (22a9:03af at binding, and after changes)
	visible = true;
	// ac7f:03f8: a proxy shows state 1 while +0x2f is set (24d2:1168 = SetVisible)
	if (findProxy(_location, object)) {
		visible = getFlag(object) != 0;
		// a000:8cce: the chicken in the fridge is seen only while the fridge is open
		if (_location == 1 && object == 178)
			visible = visible && isKitchenProxyOpen(object);
		return 1;
	}
	// the chicken farm's run: the chickens by their well-being (6b06:0411)
	if (_location == 4 && object >= 911 && object <= 930) {
		visible = (object <= 915 || (object >= 921 && object <= 925)) == areChickensWell();
		return 1;
	}
	// the factory's line (no objects): objects_factory.cpp
	if (_location == 6 && object >= 951 && object <= 978)
		return getFactoryLineState(object, visible);
	// room 9's falling bag (no object, 7554:0b89): state 2, shown while it falls
	if (_location == 3 && object == 904) {
		visible = _bagging;
		return 2;
	}
	const ObjectInfo *info = findObject(_location, object);
	uint32 rule = info ? getObjectClass(info).stateRule : 0;
	switch (rule) {
	case 0x13ec057d: // door
		return _doors.contains(object) && !_doors[object] ? 2 : 1;
	case 0xa0001b3f: // water bed: in use 1, off 2
		return getFlag(kBed) ? 2 : 1;
	case 0xa00018a0: // window: open 1, closed 2
		return getFlag(kWindow) ? 1 : 2;
	case 0xa0002340: // curtains: drawn 2, blowing while the window is open 3, else 1
		if (getFlag(kCurtains))
			return 2;
		return getFlag(kWindow) ? 3 : 1;
	case 0xa0001f8e: // water-bed switch: shown while the bed is in use
		visible = getFlag(kBed) == 0;
		return 1;
	case 0xa00076d4: // basket: full 1, empty 2
	case 0xa000789a: // wastebasket: flag set (empty) 1, else 2
		return getFlag(object) ? 1 : 2;
	case 0xa0000e0a: // TV, video: on 1, standby or off 2
		return getFlag(object) == 1 || getFlag(object) == 2 ? 2 : 1;
	case 0xa000154d: // roller blind: down 2, up 1
		return getFlag(object) ? 2 : 1;
	case 0xa0008fcd: // father: cold 5, else 6 with +0x30 set, else 1
		if (getFlag(kFather) == 1 || isFatherCold())
			return 5;
		return getField(kFather, 0x30) ? 6 : 1;
	case 0xa00096b0: // floor lamp: off (+0x34) 2, on 1
		return getField(object, 0x34) ? 2 : 1;
	case 0xa0005592: // living-room door: closed 3; open 1 if door 107 (DS:cae2) is open, else 2
		if (!_doors.contains(object) || !_doors[object])
			return 3;
		return _doors.contains(kBedroomDoor) && _doors[kBedroomDoor] ? 1 : 2;
	case 0xa000c538: // living-room window: hidden while leaky (100), else state 1
		visible = getFlag(object) != 100;
		return 1;
	case 0xa000c063: // thermometer: hung up (+0x31) 1, else 2
		return getField(object, 0x31) ? 1 : 2;
	default:
		// the root rule or one not implemented (0): the caller decides
		if (int state = getGardenFigureState(rule, object, visible))
			return state;
		if (int state = getBathroomFigureState(rule, object, visible))
			return state;
		if (int state = getKitchenFigureState(rule, object, visible))
			return state;
		if (int state = getSchoolFigureState(rule, object, visible))
			return state;
		if (int state = getFarmFigureState(rule, object, visible))
			return state;
		if (int state = getChickenFigureState(rule, object, visible))
			return state;
		if (int state = getSlaughterFigureState(rule, object, visible))
			return state;
		if (int state = getFactoryFigureState(rule, object, visible))
			return state;
		if (int state = getSuperFigureState(rule, object, visible))
			return state;
		return getShopFigureState(rule, object, visible);
	}
}

bool MiljostrupEngine::isRoomDark() const {
	// Every home room inserts the interval (16, 240, no cycling, 3, 3) and
	// never sets a factor, so the factor is 1. Only the light switch 124
	// sets the dim flag, from its VMT 0x58 (a000:2613: dim := switch off)
	// when it is bound, i.e. in the bedroom.
	if (_location != 1 || !_scene->getRoom())
		return false;
	const RoomInfo *room = _scene->getRoom();
	for (int i = 0; i < room->regionCount; ++i) {
		if (getRegion(room, i).object == kLightSwitch)
			return getFlag(kLightSwitch) == 0;
	}
	return false;
}

void MiljostrupEngine::setRoomPalette(int palette) {
	int factor = isRoomDark() ? 1 : 0;
	if (_location == 2)
		factor = getSchoolDimFactor();
	if (_location == 4) {
		setPalette(palette, getChickenDimFactor());
		return;
	}
	if (_location == 3) {
		// the cellar dims colours 16..30 only (750d:0372)
		setPalette(palette, getFarmDimFactor(), 16, 30);
		return;
	}
	setPalette(palette, factor);
}

void MiljostrupEngine::refreshObject(int object) {
	_scene->updateObject(object);
	// a000:2613: the switch's figure rule sets the dim flag; the palette
	// handler's next tick (one frame) shows it
	if (object == kLightSwitch)
		setRoomPalette(_scene->getRoom()->palette);
	// figures that depend on other objects
	if (object == kWindow)
		_scene->updateObject(kCurtains);
	if (object == kBed)
		_scene->updateObject(kBedSwitch);
}

void MiljostrupEngine::award(int points, int energy, int waste, int water, uint32 text, uint32 page) {
	awardText(points, energy, waste, water, handlerText(text), page ? handlerText(page) : Common::String());
}

void MiljostrupEngine::awardText(int points, int energy, int waste, int water, const Common::String &text,
		const Common::String &page) {
	// Manager VMT 0x28 (b06f:10db) + b06f:1140 (page)
	_award.active = true;
	_award.values[0] = points;
	_award.values[1] = energy;
	_award.values[2] = waste;
	_award.values[3] = water;
	_award.text = text;
	_award.page = page;
	_award.quiz = false;
	// command 0x40 -> b000:0295: input off (hourglass), the player talks for
	// 15 steps, then the room closes and the main loop shows the award
	_award.talking = true;
	_interface->setCursor(Interface::kCursorWait);
	_scene->startTalk(15);
}

void MiljostrupEngine::setQuiz(uint32 question, uint32 answer1, uint32 answer2, int correct, int category) {
	// b06f:1185: the quiz behind the award's "?" button; correct 1 = the first
	// answer, category 1 energy, 2 waste, 3 water
	_award.quiz = true;
	_award.question = handlerText(question);
	_award.answer1 = handlerText(answer1);
	_award.answer2 = handlerText(answer2);
	_award.correct = correct;
	_award.category = category;
}

void MiljostrupEngine::sayRootMessage(int verb) {
	// The root class's verb methods (232c:0276 ..): their fixed text
	say(readPascalString(_exe, getRootMessageOffset(verb)), 14, true);
}

void MiljostrupEngine::takeObject(int object) {
	// Room VMT 0x50 = 229f:0035: into the inventory list, the object loses
	// its region, figure and node, and the inventory shows the newest rows
	// (14a3:02fd: page = 1 below 4 items, else (count - 1) div 3)
	_inventory.push_back(object);
	int count = _inventory.size();
	_inventoryPage = count < 4 ? 1 : (count - 1) / 3;
	_interface->drawInventory();
	debug(1, "Took object %d (%s)", object, _interface->getObjectName(object).c_str());
}

// The waste class per location: the home a000:04ee, the school its copy
// 8557:420f (same text structure, its own literals and container texts)
struct WasteLiterals {
	byte location;
	uint32 thrown, in, dot, newline, is, belongs, end; // Du har smidt / i / . / CR / er / og skal derfor smides i / .
};

static const WasteLiterals kWasteLiterals[] = {
	{ 1, 0xa00004b6, 0xa00004c4, 0xa00004c8, 0xa00004cb, 0xa00004cd, 0xa00004d2, 0xa00004ec },
	{ 2, 0x855741d7, 0x855741e5, 0x855741e9, 0x855741ec, 0x855741ee, 0x855741f3, 0x8557420d }
};

// a000:0818 / 8557:462a: the chosen container; a000:096a / 8557:462a again:
// where it belongs; a000:0a56 / 8557:4711: the kind of waste
struct WasteBin {
	byte location;
	uint16 id;
	uint32 chosen, belongs, kind;
};

static const WasteBin kWasteBins[] = {
	{ 1, 214, 0xa0000786, 0xa00008d2, 0xa0000a0e },
	{ 1, 240, 0xa00007a4, 0xa00008d2, 0xa0000a0e },
	{ 1, 215, 0xa00007af, 0xa0000901, 0xa0000a1b },
	{ 1, 102, 0xa00007cd, 0xa000091f, 0xa0000a28 },
	{ 1, 217, 0xa00007ed, 0xa000093f, 0xa0000a37 },
	{ 1, 218, 0xa00007fe, 0xa0000950, 0xa0000a42 },
	{ 2, 185, 0x855745a3, 0x855745a3, 0x855746c9 }, // green
	{ 2, 183, 0x855745c1, 0x855745c1, 0x855746d6 }, // hazardous
	{ 2, 176, 0x855745e1, 0x855745e1, 0x855746e5 }, // other
	{ 2, 184, 0x855745ff, 0x855745ff, 0x855746f2 }, // glass
	{ 2, 177, 0x85574610, 0x85574610, 0x855746fd }  // paper
};

static const WasteBin *findWasteBin(int location, int id) {
	for (uint i = 0; i < ARRAYSIZE(kWasteBins); ++i) {
		if (kWasteBins[i].location == location && kWasteBins[i].id == id)
			return &kWasteBins[i];
	}
	return nullptr;
}

static bool isCompost(int location, int id) {
	return location == 1 && (id == 214 || id == 240); // green waste container or the compost heap
}

bool MiljostrupEngine::useWaste(int item, int container) {
	// Benyt <waste> med <container>; anything else is the root Benyt
	const WasteBin *chosen = findWasteBin(_location, container);
	if (!chosen)
		return false;
	const WasteItem *waste = nullptr;
	for (uint i = 0; i < ARRAYSIZE(kWasteItems); ++i) {
		if (kWasteItems[i].location == _location && kWasteItems[i].id == item)
			waste = &kWasteItems[i];
	}
	const WasteBin *belongs = waste ? findWasteBin(_location, waste->bin) : nullptr;
	const WasteLiterals *lit = nullptr;
	for (uint i = 0; i < ARRAYSIZE(kWasteLiterals); ++i) {
		if (kWasteLiterals[i].location == _location)
			lit = &kWasteLiterals[i];
	}
	const WasteTexts *talk = nullptr;
	for (uint i = 0; i < ARRAYSIZE(kWasteTexts); ++i) {
		if (kWasteTexts[i].location == _location)
			talk = &kWasteTexts[i];
	}
	if (!waste || !belongs || !lit || !talk)
		return false;

	bool right = container == waste->bin || (isCompost(_location, container) && isCompost(_location, waste->bin));
	Common::String definite = readPascalString(_ovr, waste->definite);

	Common::String text = handlerText(lit->thrown) + definite + handlerText(lit->in) +
		handlerText(chosen->chosen) + handlerText(lit->dot);
	if (right) {
		text += readPascalString(_exe, talk->praise[getRandomNumber(4)]);
	} else {
		text += readPascalString(_exe, talk->scolding[getRandomNumber(3)]);
		text += handlerText(lit->newline) + capitalize(definite) + handlerText(lit->is) +
			handlerText(belongs->kind) + handlerText(lit->belongs) + handlerText(belongs->belongs) +
			handlerText(lit->end);
	}
	if (waste->note) {
		Common::String note = readPascalString(_ovr, waste->note);
		if (!note.empty())
			text += handlerText(lit->newline) + note;
	}
	// The school's items have a page (+0x331, b06f:1140), the home's none
	Common::String page = waste->page ? readPascalString(_ovr, waste->page) : Common::String();

	// Award (5, 0, 5, 0) or (-5, 0, -5, 0); a wrong choice also sets a quiz
	// for some items (a000:02bc / 8557:3fdd, b06f:1185, category 2 = waste);
	// the item leaves the inventory; sound 1d26:0132
	if (right) {
		awardText(5, 0, 5, 0, text, page);
	} else {
		awardText(-5, 0, -5, 0, text, page);
		for (uint i = 0; i < ARRAYSIZE(kWasteQuizzes); ++i) {
			if (kWasteQuizzes[i].location == _location && kWasteQuizzes[i].item == item) {
				_award.quiz = true;
				_award.question = readPascalString(_ovr, kWasteQuizzes[i].question);
				_award.answer1 = readPascalString(_ovr, kWasteQuizzes[i].answer1);
				_award.answer2 = readPascalString(_ovr, kWasteQuizzes[i].answer2);
				_award.correct = kWasteQuizzes[i].correct;
				_award.category = 2;
			}
		}
	}
	removeObject(item);
	playSound(kSoundClose);
	return true;
}

void MiljostrupEngine::useBulb(int pack) {
	// The pack's VMT 0x68 (a000:b5a3): count - 1; at 0 the room's VMT 0x54
	// takes it out of the inventory
	setField(pack, 0x31, getField(pack, 0x31) - 1);
	if (getField(pack, 0x31) <= 0)
		removeObject(pack);
}

void MiljostrupEngine::lampAward(int lamp) {
	// The lamp's VMT 0x64 (kLampAwards): ordinary 40 / 60 / 75 W -4 / -6 /
	// -8, low-energy 9 / 11 / 15 W by the lamp; energy = points
	const LampAward *a = nullptr;
	for (uint i = 0; i < ARRAYSIZE(kLampAwards); ++i) {
		if (kLampAwards[i].lamp == lamp)
			a = &kLampAwards[i];
	}
	if (!a)
		return;
	int watt = getFlag(lamp);
	Common::String w = Common::String::format("%d", watt);
	Common::String text;
	int points = 0;
	if (!getField(lamp, 0x31)) {
		points = watt == 40 ? -4 : watt == 60 ? -6 : watt == 75 ? -8 : 0;
		text = handlerText(a->normal[0]) + w + handlerText(a->normal[1]) + handlerText(a->normal[2]) +
			handlerText(a->normal[3]);
	} else {
		int i = watt == 9 ? 0 : watt == 11 ? 1 : 2;
		points = a->lowPoints[i];
		text = handlerText(a->low[0]) + w + handlerText(a->low[1]) + handlerText(a->low[2]) + w +
			handlerText(a->low[3]) + handlerText(a->low[4]) + handlerText(a->equal[i]) +
			handlerText(a->low[5]) + handlerText(a->verdict[i]);
	}
	awardText(points, points, 0, 0, text, handlerText(a->page));
	if (a->question) {
		// b06f:1185: correct 0 (the second answer), category 1 energy
		setQuiz(a->question, a->answer1, a->answer2, 0, 1);
	}
}

void MiljostrupEngine::useTVCloseUp(int object) {
	// Close-up 18 / 19 (8d9d:0901 / 0995): the screen shows both devices and
	// switches the one used; OK -> its VMT 0x60 (a000:0c22: +0x30 := 1, the
	// state) and VMT 0x5c (+0x2c), so the room's rebuild (VMT 0x58 =
	// a000:0e0a) calls VMT 0x64, the award
	bool video = object != kTV;
	int tv = getFlag(kTV), vcr = getFlag(kVideo);
	bool ok = runTVScreen(video, tv, vcr);
	if (ok) {
		setFlag(object, video ? vcr : tv);
		setField(object, 0x30, 1);
		refreshObject(object);
	}
	redrawRoom();
	if (!ok)
		return;

	// a000:0fe0 (TV) / a000:1244 (video): off 5, stand-by 2, on -5 (energy
	// the same); quiz (correct 1 = "Tændt", category 1 energy), no page
	static const uint32 kTexts[2][3] = {
		{ 0xa0000f60, 0xa0000ecd, 0xa0000e69 },
		{ 0xa00011ca, 0xa000113a, 0xa00010d9 }
	};
	static const int kPoints[3] = { -5, 2, 5 };
	int state = CLIP(getFlag(object), 0, 2);
	award(kPoints[state], kPoints[state], 0, 0, kTexts[video][state], 0);
	setQuiz(video ? 0xa0001211 : 0xa0000faa, video ? 0xa0001235 : 0xa0000fd1, video ? 0xa000123b : 0xa0000fd7, 1, 1);
}

void MiljostrupEngine::useRadiator(int object) {
	// a000:3cd1: once set (+0x33) it stays; else GoTo(+0x31, 0) and the room
	// ends: close-up 16 (bedroom) or 17 (living room), unit 8e40
	if (getField(object, 0x33)) {
		say(handlerText(0xa0003cb5), 14, true);
		return;
	}
	bool bedroom = object != kRadiator;
	int level = runThermostat(bedroom ? 115 : 116, bedroom ? 64 : 63, 7, getField(object, 0x3c),
		kRadiatorTemperatures);
	if (level >= 0) {
		// 8e40:066f: radiator VMT 0x64 (a000:79db -> 3d3a): level, temperature,
		// +0x33 := 1; VMT 0x5c: +0x2c := 1, so the room's rebuild (VMT 0x58,
		// a000:3d50) calls VMT 0x60, the award
		setField(object, 0x3c, level);
		setFlag(object, kRadiatorTemperatures[level]);
		setField(object, 0x33, 1);
	}
	// The close-up returns with GoTo(room, 0): the room again, the player stays
	redrawRoom();
	if (level < 0)
		return;
	if (!bedroom) {
		// a000:7f5a by +0x34[+0x3c]: 24 and 22 degrees too warm, 20 and 18 fine
		switch (getFlag(object)) {
		case 24:
			award(-10, -10, 0, 0, 0xa0007da5, 0xa0007f54);
			break;
		case 22:
			award(-5, -5, 0, 0, 0xa0007df8, 0xa0007f54);
			break;
		case 20:
			award(10, 10, 0, 0, 0xa0007e4b, 0xa0007f54);
			break;
		default:
			award(10, 10, 0, 0, 0xa0007eb2, 0xa0007f54);
			break;
		}
		// b06f:1185(question, "15°C", "20°C", correct 0 = the second, category
		// 1 energy), page "12-13" (b06f:1140)
		setQuiz(0xa0007f19, 0xa0007f4a, 0xa0007f4f, 0, 1);
		return;
	}

	// a000:7be5
	int t = getFlag(object);
	if (t == 24) {
		award(-4, -4, 0, 0, 0xa0007a68, 0xa0007be2);
	} else if (t == 22) {
		award(2, 2, 0, 0, 0xa0007aee, 0xa0007be2);
	} else {
		awardText(10, 10, 0, 0, handlerText(0xa0007b44) + Common::String::format("%d", t) +
			readPascalString(_ovr, kTextDegrees) + handlerText(0xa0007b70) + handlerText(0xa0007b7f),
			handlerText(0xa0007be2));
	}
	// b06f:1185(question, answer1, answer2, correct 1, category 1 energy), page "13"
	setQuiz(0xa0007bb0, 0xa0007bc7, 0xa0007bd9, 1, 1);
}

bool MiljostrupEngine::runClassHandler(uint32 handler, int verb, int object, int second) {
	switch (handler) {
	// Garden gate 190/191 (a000:c5a8): a door whose partner on the other
	// side opens and closes with it (a000:c63e / a000:c67d)
	case 0xa000c63e: // Åbn
	case 0xa000c67d: { // Luk
		bool wantOpen = handler == 0xa000c63e;
		bool open = _doors.contains(object) && _doors[object];
		Common::String pronoun = capitalize(readPascalString(_ovr, findObject(_location, object)->pronoun));
		if (open == wantOpen) {
			say(pronoun + readPascalString(_exe, wantOpen ? kTextDoorAlreadyOpen : kTextDoorAlreadyClosed), 14, true);
		} else {
			_doors[object] = wantOpen;
			refreshObject(object);
			int partner = getDoorPartner(object);
			if (partner) {
				_doors[partner] = wantOpen;
				refreshObject(partner);
			}
			playSound(wantOpen ? kSoundOpen : kSoundClose); // via 13ec:0484 / 050b
		}
		break;
	}

	// Waste items (VMT 9a88)
	case 0xa00004ee:
	case 0x8557420f: // the school's copy (VMT 66f8)
		if (!useWaste(object, second))
			sayRootMessage(verb);
		break;
	case 0x855744e4: { // the school's waste Tag (the item is already carried)
		const ObjectInfo *info = findObject(_location, object);
		Common::String pronoun = info ? readPascalString(_ovr, info->pronoun) : Common::String();
		if (pronoun == handlerText(0x8557449d))
			say(handlerText(0x855744a1), 14, true);
		else if (pronoun == handlerText(0x855744b6))
			say(handlerText(0x855744ba), 14, true);
		else
			say(handlerText(0x855744cf), 14, true);
		break;
	}

	// Water bed 115
	case 0xa0001a3f: // Benyt
		say(handlerText(0xa0001a28), 14, true);
		break;
	case 0xa0001991: // Se på
		if (getFlag(kBed))
			say(handlerText(0xa0001943), 14, true);
		else if (getFlag(kBedSwitch))
			say(handlerText(0xa0001953), 14, true);
		else
			say(handlerText(0xa000197a), 11, true);
		break;
	case 0xa0003b38: // the other bed 134 (basement, proxy 182) Benyt med the water bed (0x73)
		if (second != kBed) {
			sayRootMessage(kVerbUse);
			break;
		}
		// The room's VMT 0x54 takes 134 out of the inventory; the water bed's
		// VMT 0x60 (a000:1ac9): +0x2f := 1, its figure (VMT 0x58 a000:1b3f:
		// state 2, the switch 123 is updated and disappears), Award(10, 10,
		// 0, 0), page "14-15". GetName (VMT 0x14 a000:1b9c) now says "seng".
		removeObject(kOtherBed);
		setFlag(kBed, 1);
		refreshObject(kBed);
		refreshObject(kBedSwitch);
		_interface->setNameOverride(kBed, handlerText(0xa0001b8e));
		award(10, 10, 0, 0, 0xa0001a70, 0xa0001ac3);
		break;

	// Window 116
	case 0xa0001688: // Se på
		say(handlerText(getFlag(kWindow) ? 0xa000166b : 0xa0001679), 14, true);
		break;
	case 0xa0001719: // Åbn: it never opens
		say(handlerText(getFlag(kWindow) ? 0xa00016ea : 0xa0001701), 14, true);
		break;
	case 0xa00017f1: // Luk
		if (!getFlag(kWindow)) {
			say(handlerText(0xa000177b), 14, true);
		} else {
			setFlag(kWindow, 0);
			award(10, 10, 0, 0, 0xa0001793, 0xa00017ee);
			refreshObject(kWindow);
			playSound(kSoundOpen); // 1d26:010f, as in the code
		}
		break;

	// Curtains 117
	case 0xa00020bf: // Se på
		if (getFlag(kCurtains))
			say(handlerText(0xa000207e), 14, true);
		else if (getFlag(kWindow))
			say(handlerText(0xa0002091), 14, true);
		else
			say(handlerText(0xa00020ac), 14, true);
		break;
	case 0xa0002232: // Træk
		if (getFlag(kCurtains)) {
			say(handlerText(0xa0002158), 14, true);
		} else if (getFlag(kWindow)) {
			say(handlerText(0xa0002170), 14, true);
		} else {
			setFlag(kCurtains, 1);
			refreshObject(kCurtains);
			award(10, 10, 0, 0, 0xa0002187, 0xa000222f);
		}
		break;

	// Water-bed switch 123
	case 0xa0001caa: // Se på
		say(handlerText(getFlag(kBedSwitch) ? 0xa0001c7e : 0xa0001c93), 14, true);
		break;
	case 0xa0001d4d: // Tænd: it stays off
		say(handlerText(getFlag(kBedSwitch) ? 0xa0001d0c : 0xa0001d2a), 14, true);
		break;
	case 0xa0001eb1: // Sluk
		if (getFlag(kBedSwitch)) {
			setFlag(kBedSwitch, 0);
			award(0, 0, 0, 0, 0xa0001daf, 0xa0001e8b);
			debug(1, "Quiz: %s / %s / %s", handlerText(0xa0001e55).c_str(),
				handlerText(0xa0001e6d).c_str(), handlerText(0xa0001e7a).c_str());
		} else {
			say(handlerText(0xa0001e91), 14, true);
		}
		break;

	// Light switch 124 (the palette dimming 2472:057e is not done yet)
	case 0xa00024c2: // Se på (also the living room's switch 125)
		say(handlerText(getFlag(object) ? 0xa00024a4 : 0xa00024b2), 14, true);
		break;
	case 0xa000253b: // Tænd
		if (getFlag(kLightSwitch)) {
			say(handlerText(0xa0002524), 14, true);
		} else {
			setFlag(kLightSwitch, 1);
			refreshObject(kLightSwitch);
			award(-10, -10, 0, 0, 0xa000273c, 0xa00027a8);
		}
		break;
	case 0xa00025ae: // Sluk
		if (getFlag(kLightSwitch)) {
			setFlag(kLightSwitch, 0);
			refreshObject(kLightSwitch);
			award(10, 10, 0, 0, 0xa000264f, 0xa00026d2);
		} else {
			say(handlerText(0xa0002595), 14, true);
		}
		break;

	// Laundry 143, basket 158, wastebasket 118
	case 0xa0005561: // vasketøj Se på
		say(handlerText(0xa0005542), 14, true);
		break;
	case 0xa00075ca: // kurv Tag
		if (getFlag(kBasket)) {
			takeObject(kLaundry);
			setFlag(kBasket, 0);
			// a000:7608: the empty basket has its own name
			_interface->setNameOverride(kBasket, readPascalString(_ovr, kNameKurv));
			refreshObject(kBasket);
		} else {
			sayRootMessage(verb);
		}
		break;
	case 0xa000778e: // papirkurv Tag
		if (getFlag(kWasteBasket)) {
			say(handlerText(0xa0007782), 14, true);
		} else {
			takeObject(179);
			takeObject(180);
			takeObject(181);
			setFlag(kWasteBasket, 1);
			refreshObject(kWasteBasket);
		}
		break;
	case 0xa0007838: // papirkurv Se på
		say(handlerText(getFlag(kWasteBasket) ? 0xa0007815 : 0xa0007821), 14, true);
		break;

	// Lamps 200, 201, 221 (and 183): watt +0x2f, low-energy +0x31, bulb changed +0x32
	case 0xa000a3be: // Se på: "Der sidder en " + watt + " W " + type + " i."
		say(handlerText(0xa000a3a7) + Common::String::format("%d", getFlag(object)) +
			handlerText(0xa000a3b6) + handlerText(getField(object, 0x31) ? 0xa000a389 : 0xa000a397) +
			handlerText(0xa000a3ba), 14, true);
		break;
	case 0xa000a540: // Benyt med a bulb (DS:ccb6 VMT 0x10)
		if (!isBulb(second)) {
			sayRootMessage(kVerbUse);
		} else if (getField(object, 0x32)) {
			say(handlerText(0xa000a521), 14, true);
		} else {
			// the lamp's VMT 0x60 (a000:a4ef): watt, low-energy, changed; its
			// figure (VMT 0x58) and award (VMT 0x64); then the pack's VMT 0x68
			setFlag(object, getFlag(second));
			setField(object, 0x31, getField(second, 0x33));
			setField(object, 0x32, 1);
			refreshObject(object);
			lampAward(object);
			useBulb(second);
		}
		break;

	// Bulb packs 278, 202, 281 (40/60/75 W), 252, 272, 275 (9/11/15 W
	// low-energy): watt +0x2f, count +0x31, low-energy +0x33
	case 0xa000b434: { // Se på: "Der er 1 stk. " / "Der er " + n + " stk. " + watt + " W " + kind + "."
		int count = getField(object, 0x31);
		bool low = getField(object, 0x33) != 0;
		Common::String watt = Common::String::format("%d", getFlag(object));
		Common::String text;
		if (count == 1)
			text = handlerText(0xa000b3ef) + watt + handlerText(0xa000b3fe) +
				handlerText(low ? 0xa000b3d1 : 0xa000b3df);
		else
			text = handlerText(0xa000b425) + Common::String::format("%d", count) + handlerText(0xa000b42d) +
				watt + handlerText(0xa000b3fe) + handlerText(low ? 0xa000b404 : 0xa000b413);
		say(text + handlerText(0xa000b402), 14, true);
		break;
	}
	case 0xa000b5dd: // Benyt med a lamp (DS:ccb6 VMT 0x0c): the lamp's Benyt
		if (isLamp(second))
			executeVerb(kVerbUse, second, object);
		else
			sayRootMessage(kVerbUse);
		break;
	case 0xa000a6b2:
		say(handlerText(0xa000a696), 14, true);
		break;
	case 0xa000a665:
		say(handlerText(0xa000a64c), 14, true);
		break;
	case 0xa000a4be:
		say(handlerText(0xa000a4ad), 14, true);
		break;
	case 0xa000a47c:
		say(handlerText(0xa000a46b), 14, true);
		break;

	// Living room (notes/livingroom.md)
	// TV 287, video 228: +0x2f 0 on, 1 standby, 2 off; +0x30 set once done
	case 0xa0000c59: { // Se på: pronoun + " er " + state
		static const uint32 kStates[] = { 0xa0000c3d, 0xa0000c44, 0xa0000c50 };
		Common::String text = capitalize(readPascalString(_ovr, findObject(_location, object)->pronoun)) +
			handlerText(0xa0000c38);
		if (getFlag(object) <= 2)
			text += handlerText(kStates[getFlag(object)]);
		say(text, 14, true);
		break;
	}
	case 0xa0000ddc: // Tænd, Sluk: Benyt
	case 0xa0000df3:
	case 0xa0000d5d: // Benyt
		if (!getField(object, 0x30)) {
			// GoTo(+0x31, 0): close-up 18 (TV) or 19 (video)
			useTVCloseUp(object);
		} else {
			say(capitalize(readPascalString(_ovr, findObject(_location, object)->pronoun)) +
				handlerText(0xa0000d4d), 14, true);
		}
		break;

	// Roller blind 105
	case 0xa00014bf: // Træk
		if (getFlag(kBlind)) {
			say(handlerText(0xa000142a), 14, true);
		} else {
			setFlag(kBlind, 1);
			refreshObject(kBlind);
			award(10, 10, 0, 0, 0xa0001440, 0);
		}
		break;
	case 0xa00013c8: // Se på
		say(handlerText(getFlag(kBlind) ? 0xa00013a0 : 0xa00013b4), 14, true);
		break;

	// Ceiling-light switch 125 (class of 124; the floor lamp is DS:cbfe)
	case 0xa000282c: // Tænd: on -> 124's Tænd says it is on already
		say(handlerText(getFlag(object) ? 0xa0002524 : 0xa0002810), 14, true);
		break;
	case 0xa00028e5: // Sluk
		if (!getFlag(object)) {
			say(handlerText(0xa0002595), 14, true); // 124's Sluk: already off
		} else if (getField(kFloorLamp, 0x34)) {
			say(handlerText(0xa0008e24), 14, true); // father 0x64
		} else {
			setFlag(object, 0);
			refreshObject(object);
			award(10, 10, 0, 0, 0xa000286f, 0);
		}
		break;

	// Father 135
	case 0xa0008ee3: // Se på
		say(handlerText(getFlag(kFather) == 1 || isFatherCold() ? 0xa0008eb9 : 0xa0008ece), 14, true);
		break;
	case 0xa0008f6b: // Tal til: he answers, the player does not talk
		if (getFlag(kFather) == 1 || isFatherCold())
			say(handlerText(0xa0008f45), 11, false);
		else
			say(handlerText(0xa0008f59), 2, false);
		break;
	case 0xa00056d0: // the sweater 148 Giv til far (0x87)
	case 0x990934d3: { // the fan heater 247 (basement, proxy 248) Giv til far
		bool sweater = handler == 0xa00056d0;
		if (second != kFather) {
			sayRootMessage(kVerbGive);
		} else if (!isFatherCold()) {
			// father's VMT 0x60 (a000:8dd3); he answers in colour 2
			say(handlerText(sweater ? 0xa00056aa : 0x990934ad), 2, false);
		} else {
			// The room's VMT 0x54 takes the item. Sweater: father VMT 0x70
			// (a000:8e9c: +0x30 := 1, +0x2f := 0, figure: he wears it),
			// Award(10, 10, 0, 0). Heater: proxy 249 (DS:cabe) VMT 0x64(1)
			// shows it in front of him, father VMT 0x68 (a000:8e07: +0x31
			// := 1, +0x2f := 0), Award(-10, -10, 0, 0). No page.
			removeObject(object);
			if (sweater) {
				setField(kFather, 0x30, 1);
			} else {
				setFlag(kHeaterProxy, 1);
				refreshObject(kHeaterProxy);
				setField(kFather, 0x31, 1);
			}
			setFlag(kFather, 0);
			refreshObject(kFather);
			if (sweater)
				award(10, 10, 0, 0, 0xa00055fe, 0);
			else
				award(-10, -10, 0, 0, 0x99093435, 0);
		}
		break;
	}

	// Radiator 171 (temperature +0x2f, level +0x3c, close-up +0x31 = 17);
	// Åbn, Luk, Tænd, Sluk call Benyt. The thermometer is DS:cc6a.
	case 0xa0003c33:
	case 0xa0003c1c:
	case 0xa0003bee:
	case 0xa0003c05:
		// Åbn, Luk, Tænd, Sluk: the class's Benyt (VMT 0x30) with 100
		if (object == 138 || object == 139)
			return runClassHandler(0xa0003cd1, kVerbUse, object, 100); // the kitchen thermostats
		return runClassHandler(object == kRadiator ? 0xa0007d5b : 0xa0007a1e, kVerbUse, object, 100);
	case 0xa0007d5b: // Benyt
		if (getField(kThermometer, 0x31))
			useRadiator(object);
		else
			say(handlerText(0xa0007d3f), 14, true);
		break;
	case 0xa0007a1e: // bedroom radiator 122 Benyt: not while the window (DS:cafa) is open
		if (getFlag(kWindow))
			say(handlerText(0xa0007a02), 14, true);
		else
			useRadiator(object);
		break;
	case 0xa0007985: { // Se på (also the bedroom radiator 122)
		int level = getField(object, 0x3c);
		say(handlerText(0xa000796f) + ((uint)level < ARRAYSIZE(kRadiatorLevelNames) ?
			readPascalString(_exe, kRadiatorLevelNames[level]) : Common::String()) +
			handlerText(0xa0007983), 14, true);
		break;
	}

	// Floor lamp 183 (watt +0x2f, 0 = blown; off +0x34; the switch is DS:cb12)
	case 0xa000966a: // Se på
		if (getFlag(object))
			return runClassHandler(0xa000a3be, verb, object, second);
		say(handlerText(0xa0009657), 14, true);
		break;
	case 0xa00095d8: // Tænd
		if (!getFlag(object)) {
			say(handlerText(0xa00095ab), 14, true);
		} else if (!getField(object, 0x34)) {
			say(handlerText(0xa00095c0), 14, true);
		} else {
			setField(object, 0x34, 0);
			refreshObject(object);
		}
		break;
	case 0xa0009540: // Sluk
		if (getField(object, 0x34)) {
			say(handlerText(0xa0009527), 14, true);
		} else if (getFlag(kCeilingSwitch)) {
			setField(object, 0x34, 1);
			refreshObject(object);
		} else {
			say(handlerText(0xa0008e24), 14, true); // father 0x64
		}
		break;
	// Thermometer 220 (hung up +0x31), window 223 (+0x2f 100 leaky, 224/225 sealed)
	case 0xa000c01d: // Se på
		if (!getField(object, 0x31)) {
			say(handlerText(0xa000bff9), 14, true);
		} else {
			// a000:8150: "Det viser " + VMT 0x60 (a000:8118: the temperature of
			// the radiator in +0x2f, 171, a000:be8f) + ac76:0003 ("°C") + "."
			say(handlerText(0xa0008143) + Common::String::format("%d", getFlag(kRadiator)) +
				readPascalString(_ovr, kTextDegrees) + handlerText(0xa000814e), 14, true);
		}
		break;
	case 0xa000c0a5: // the shop's thermometer 222 Benyt med 220 (0xdc)
		if (second != kThermometer) {
			sayRootMessage(kVerbUse);
			break;
		}
		// 220's VMT 0x64 (a000:bf57): hung up, figure, Award(0, 0, 0, 0);
		// then the room's VMT 0x54 takes 222 out of the inventory
		setField(kThermometer, 0x31, 1);
		refreshObject(kThermometer);
		removeObject(kThermometerItem);
		award(0, 0, 0, 0, 0xa000befd, 0);
		break;
	case 0xa000c18a: // sealing strips 224 Benyt med the window (0xdf)
	case 0xa000c2ce: // tape 225
		if (second != kLivingWindow) {
			sayRootMessage(kVerbUse);
		} else if (getFlag(kLivingWindow) != 100) {
			// window VMT 0x60 (a000:c4ff): already sealed
			say(handlerText(handler == 0xa000c18a ? 0xa000c172 : 0xa000c2b6), 14, true);
		} else {
			// window VMT 0x64 (a000:c51e): +0x2f := the item, figure; the item
			// out of the inventory; strips Award(10, 10, 0, 0), tape Award(0,
			// 0, 0, 0) with page "12"
			setFlag(kLivingWindow, object);
			refreshObject(kLivingWindow);
			removeObject(object);
			if (handler == 0xa000c18a)
				award(10, 10, 0, 0, 0xa000c0e2, 0);
			else
				award(0, 0, 0, 0, 0xa000c24f, 0xa000c2b3);
		}
		break;
	case 0xa000c46a: // Se på
		if (getFlag(object) == 100)
			say(handlerText(0xa000c419), 14, true);
		else if (getFlag(object) == 224)
			say(handlerText(0xa000c426), 14, true);
		else if (getFlag(object) == 225)
			say(handlerText(0xa000c448), 14, true);
		break;

	// Coins 101 (amount in +0x2f, 150 at the start)
	case 0x99091753: // Se på: "Der er " + amount + " mønter."
		say(handlerText(0x99091742) + Common::String::format("%d", getFlag(object)) +
			handlerText(0x9909174a), 14, true);
		break;
	case 0x990917d8: // Tag
		say(handlerText(0x990917c3), 14, true);
		break;

	default:
		if (runGardenHandler(handler, verb, object, second) || runBathroomHandler(handler, verb, object, second) ||
				runKitchenHandler(handler, verb, object, second) || runShopHandler(handler, verb, object, second) ||
				runSchoolHandler(handler, verb, object, second) || runFarmHandler(handler, verb, object, second) ||
				runChickenHandler(handler, verb, object, second) ||
				runSlaughterHandler(handler, verb, object, second) ||
				runFactoryHandler(handler, verb, object, second) ||
				runSuperHandler(handler, verb, object, second))
			return true;
		// handlers that only say one fixed text (generated)
		for (uint i = 0; i < ARRAYSIZE(kSimpleHandlers); ++i) {
			if (kSimpleHandlers[i].address == handler) {
				say(readPascalString(_ovr, kSimpleHandlers[i].text), kSimpleHandlers[i].color,
					kSimpleHandlers[i].talk != 0);
				return true;
			}
		}
		return false;
	}
	return true;
}

} // End of namespace Miljostrup
