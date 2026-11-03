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

// The shop (home, room 24), written from notes/shop.md: the goods, the
// assistant who fetches them, the basket and paying with the coins.

#include "miljostrup/interface.h"
#include "miljostrup/miljostrup.h"
#include "miljostrup/scene.h"
#include "miljostrup/verbs.h"
#include "miljostrup/handlers_data.h"

namespace Miljostrup {

enum {
	kCoins = 101,         // +0x2f the amount, DS:ca56
	kAssistant = 199,     // ekspedient, DS:c9e6
	kShopRoom = 24,
	kBasketSize = 50,     // 9909:1294: full above 49
	kAssistantStep = 5    // +0x34: pixels per walking step
};

static const ShopItem *findShopItem(int location, int id) {
	for (uint i = 0; i < ARRAYSIZE(kShopItems); ++i) {
		if (kShopItems[i].location == location && kShopItems[i].id == id)
			return &kShopItems[i];
	}
	return nullptr;
}

void MiljostrupEngine::assistantSay(const Common::String &text) {
	// 9909:10df: stop, talk (state 4) for 20 steps, then state 3; the room's
	// Say in colour 13 without the player talking
	_scene->stopFigure(kAssistant);
	_scene->queueFigureWalk(kAssistant, 4, 0, 0, 20);
	_scene->queueFigureWalk(kAssistant, 3, 0, 0, 1);
	say(text, 13, false);
}

int MiljostrupEngine::getBasketTotal() const {
	int total = 0;
	for (uint i = 0; i < _basket.size(); ++i) {
		if (const ShopItem *item = findShopItem(_location, _basket[i]))
			total += item->price;
	}
	return total;
}

void MiljostrupEngine::putBasketBack() {
	// 9909:11ef (basket VMT 0x10), when the shop room ends (8b6b:08c2): every
	// item's VMT 0x78 puts it back on the shelf
	for (int i = (int)_basket.size() - 1; i >= 0; --i) {
		const ShopItem *item = findShopItem(_location, _basket[i]);
		if (!item)
			continue;
		setFlag(item->counter, 0);
		if (item->bulb)
			setField(item->counter, 0x36, 0); // 9909:15b6
		else
			setFlag(item->id, 1);             // 9909:14d1
	}
	_basket.clear();
}

void MiljostrupEngine::tickShop() {
	// The assistant's walks end with message 0x21 (2381:0065), which the
	// shop (8b6b:08c2) passes to his VMT 0x64 (9909:0f80)
	if (!_shopFetching || _scene->isFigureBusy(kAssistant))
		return;
	const ShopItem *item = findShopItem(_location, _shopItem);
	if (!item) {
		_shopFetching = 0;
		return;
	}
	if (_shopFetching == 1) {
		// He has reached it: VMT 0x80 takes it from the shelf (bulbs stay,
		// 9909:15af), then VMT 0x6c (9909:0f39) walks back
		_shopFetching = 2;
		if (!item->bulb) {
			setFlag(item->id, 0);
			refreshObject(item->id);
		}
		_scene->queueFigureWalk(kAssistant, _shopBackState, -_shopDistance, 0,
			ABS(_shopDistance) / kAssistantStep);
		return;
	}
	// Back at the counter: idle, input on, the item on the counter (VMT 0x7c:
	// 9909:1440, bulbs 9909:1553 count them) and in the basket (9909:11ad)
	_shopFetching = 0;
	setInputOff(false);
	if (item->bulb) {
		if (getField(item->counter, 0x36) == 0)
			setFlag(item->counter, 1);
		setField(item->counter, 0x36, getField(item->counter, 0x36) + 1);
	} else {
		setFlag(item->counter, 1);
	}
	refreshObject(item->counter);
	_basket.push_back(item->id);
	_shopItem = 0;
	refreshObject(kAssistant);
}

void MiljostrupEngine::leavingRoom(int room) {
	if (_location == 1 && room == kShopRoom)
		putBasketBack();
	if (_location == 6)
		factoryLeavingRoom();
}

int MiljostrupEngine::getShopFigureState(uint32 rule, int object, bool &visible) const {
	// 9909:114d: the assistant stands (state 3) while he is idle
	if (rule == 0x9909114d)
		return _shopFetching ? 0 : 3;
	return 0;
}

bool MiljostrupEngine::runShopHandler(uint32 handler, int verb, int object, int second) {
	switch (handler) {
	case 0x99091340: { // goods Se på: text + ": " + price + " mønter."
		const ShopItem *item = findShopItem(_location, object);
		if (!item)
			return false;
		say(readPascalString(_ovr, item->text) + handlerText(0x99091334) +
			Common::String::format("%d", item->price) + handlerText(0x99091337), 14, true);
		break;
	}
	case 0x990913d2: { // goods Tag: the assistant fetches it, if the coins cover it
		const ShopItem *item = findShopItem(_location, object);
		if (!item)
			return false;
		if (getFlag(kCoins) < item->price + getBasketTotal()) {
			assistantSay(handlerText(0x990913b2));
			break;
		}
		// 9909:0de7 (his VMT 0x60)
		if ((int)_basket.size() >= kBasketSize) {
			assistantSay(handlerText(0x99090dc4));
			break;
		}
		_shopItem = object;
		setInputOff(true);
		_scene->stopFigure(kAssistant);
		_shopDistance = item->x - _scene->getFigureX(kAssistant);
		int there = _shopDistance < 0 ? 1 : 2; // +0x3a: the walking state there
		_shopBackState = _shopDistance < 0 ? 2 : 1; // +0x38: and back
		// 9909:0ed8: walk there in steps of 5 pixels, reach (the item's pose) for 2 steps
		_scene->queueFigureWalk(kAssistant, there, _shopDistance, 0, ABS(_shopDistance) / kAssistantStep);
		_scene->queueFigureWalk(kAssistant, item->pose, 0, 0, 2);
		_shopFetching = 1;
		break;
	}
	case 0x99091050: // assistant Tal til
		if (_basket.empty())
			assistantSay(handlerText(0x99091013));
		else
			assistantSay(handlerText(0x99091032) + Common::String::format("%d", getBasketTotal()) +
				handlerText(0x9909103e));
		break;
	case 0x99091862: { // coins Giv til the assistant: pay for the basket
		if (second != kAssistant) {
			sayRootMessage(kVerbGive);
			break;
		}
		if (_basket.empty()) {
			assistantSay(handlerText(0x99091809));
			break;
		}
		if (getBasketTotal() > getFlag(kCoins)) {
			assistantSay(handlerText(0x99091831));
			break;
		}
		// 9909:1235 (basket VMT 0x18): every item's VMT 0x74 (9909:1492): the
		// counter proxy's Tag gives the goods, the coins pay the price (9909:191c)
		for (int i = (int)_basket.size() - 1; i >= 0; --i) {
			const ShopItem *item = findShopItem(_location, _basket[i]);
			if (!item)
				continue;
			if (item->bulb) {
				// 9909:163b: an empty pack (+0x31 = 0) gets 1 bulb (VMT 0x70)
				// and goes into the inventory; otherwise it gets as many as
				// are still on the counter (VMT 0x6c, +0x36), so three of a
				// kind give 1 + 2 + 1 = 4 bulbs (as in the original). Then
				// the counter's count goes down.
				int onCounter = getField(item->counter, 0x36);
				if (getField(item->goods, 0x31) == 0) {
					setField(item->goods, 0x31, 1);
					takeObject(item->goods);
				} else {
					setField(item->goods, 0x31, getField(item->goods, 0x31) + onCounter);
				}
				setField(item->counter, 0x36, onCounter - 1);
				if (getField(item->counter, 0x36) <= 0)
					setFlag(item->counter, 0);
			} else {
				setFlag(item->counter, 0); // ac7f:025c
				takeObject(item->goods);
			}
			refreshObject(item->counter);
			setFlag(kCoins, getFlag(kCoins) - item->price);
			if (getFlag(kCoins) <= 0)
				removeObject(kCoins);
		}
		_basket.clear();
		assistantSay(handlerText(0x99091851));
		break;
	}
	default:
		return false;
	}
	return true;
}

} // End of namespace Miljostrup
