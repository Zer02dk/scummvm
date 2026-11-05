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

#ifndef MILJOSTRUP_H
#define MILJOSTRUP_H

#include "common/hashmap.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "common/error.h"
#include "common/random.h"
#include "common/rect.h"
#include "engines/engine.h"
#include "graphics/screen.h"

#include "miljostrup/archive.h"
#include "miljostrup/detection.h"
#include "miljostrup/font.h"
#include "miljostrup/sound.h"
#include "miljostrup/users.h"

#include "common/serializer.h"

namespace Miljostrup {

enum {
	kPaletteA = 0, // rooms outside the home, title and credits
	kPaletteB = 1  // the home
};

// Keys as returned by readKey(): code page 865 characters, or one of these
enum {
	kKeyNone = -1,
	kKeyBackspace = 8,
	kKeyEnter = 13,
	kKeyEscape = 27,
	kKeyOther = 0x100, // a key without a character
	// function keys: 0x100 + BIOS scan code, as ae72 tests them
	kKeyF1 = 0x13b,
	kKeyF5 = 0x13f,
	kKeyF6 = 0x140,
	kKeyF10 = 0x144
};

// What the F10 menu (b06f:0161) chose: the commands of its buttons
enum MenuAction {
	kMenuContinue = 3,   // "fortsætte spillet"
	kMenuRestartGame = 4, // "spille spillet forfra" (b06f:00ca)
	kMenuReplay = 5,     // "spille lokaliteten forfra" (b06f:0079)
	kMenuStop = 6        // "stoppe spillet indtil videre" (b06f:011b)
};

class Interface;
struct LocationTarget;
class Scene;
class TextConsole;

class MiljostrupEngine : public Engine {
private:
	const ADGameDescription *_gameDescription;
	Common::RandomSource _randomSource;
	ImageArchive _archive;
	byte _palettes[2][256 * 3];
	byte _rawPalettes[2][256 * 3]; // 6-bit values as in the EXE
	GameFont _font;
	Common::Array<Common::String> _loginStrings;
	UserBase _users;
	int _user = -1;

	int _location = 0;
	int _room = 0;
	Scene *_scene = nullptr;
	Interface *_interface = nullptr;
	Common::File _ovr;
	Sound _sound;
	Common::File _exe;
	int _messageTimer = 0; // frames until the message goes away
	Common::HashMap<uint32, int> _flags; // fields of the location's objects: ID << 16 | offset (offsets reach 0x130)
	int _score[4] = { 0, 0, 0, 0 };      // points, energy, waste, water
	// Fasekontrol (DS:cdf6): the phase (= location 1..7) and the game's
	// totals: points, max points, energy, max, waste, max, water, max
	int _phase = 1;
	int _startPhase = 1;       // +0x101: from the command line (1000:0206), else 1
	int _totals[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	bool _phaseDone = false;   // PhaseCompleted: the location's loop ends
	struct PendingAward {
		bool active = false;
		bool talking = false;
		int values[4] = { 0, 0, 0, 0 };
		Common::String text, page;
		// b06f:1185: the quiz behind the "?" button
		bool quiz = false;
		Common::String question, answer1, answer2;
		int correct = 0;   // 1 = the first answer
		int category = 0;  // 1 energy, 2 waste, 3 water
	} _award;
	Common::Array<uint16> _inventory; // DS:d100
	int _inventoryPage = 1;            // DS:ccc0, first shown row (14a3:04d9)
	Common::HashMap<uint16, bool> _doors; // open flag per door of the location

	bool loadPalettes();
	void showImage(uint number, int palette);
	void drawInterface();
	void runRoomLoop();

	// Input: pollKey() returns kKeyNone if no key is waiting, readKey() waits
	int pollKey();
	int readKey();

	// Start-up screens (startup.cpp)
	bool loadStrings();
	bool runTitlePage(uint image);
	/** F1: the function keys' help (unit b1c3). */
	void runHelpScreen();
	/** F5 / F6: the score of this location or of the whole game (b06f:0413). */
	void runScoreScreen(bool wholeGame);
	/** F10: the menu "Jeg ønsker at" (b06f:002e) over the screen; a MenuAction. */
	int runMenu();
	/**
	 * A function key in a room or the end screen (ae72 / 5042): runs its
	 * screen; a menu choice other than "continue" is kept in _menuAction.
	 * Returns true if a screen was shown.
	 */
	bool runFunctionKey(int key, bool endOfGame);
	int _menuAction = 0;
	/** Phase 8, room 5 mode 1 (5042): the congratulations and credits scroll. */
	bool runEndScene();
	/** Phase 8, room 5 mode 2 (5042:0550): the credits page. */
	bool runEndCredits();
	/** Phase 8 (5038:0069): the end scene and the credits page in turn. */
	void runEndOfGame();
	bool runLicence();
	bool runLogin();
	bool loginExistingUser(TextConsole &console);
	bool loginNewUser(TextConsole &console);
	void newUserHeader(TextConsole &console);
	bool readName(TextConsole &console, Common::String &name);
	bool runMapIntro(int location);
	Common::String getUserBaseName() const;
	void loadUsers();
	void saveUsers();
	// The status file MILJØGEM.NNN (notes/users.md, Fasekontrol b288)
	Common::String getStatusName() const;
	Common::SeekableReadStream *openStatus(Common::String &name);
	bool startStatus();
	bool loadStatus();
	void resetStatus();
	void saveStatus();

protected:
	// Engine APIs
	Common::Error run() override;

public:
	Graphics::Screen *_screen = nullptr;

public:
	MiljostrupEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~MiljostrupEngine() override;

	uint32 getFeatures() const;
	Common::String getGameId() const;

	uint32 getRandomNumber(uint maxNum) {
		return _randomSource.getRandomNumber(maxNum);
	}

	bool hasFeature(EngineFeature f) const override {
		return f == kSupportsReturnToLauncher || f == kSupportsLoadingDuringRuntime ||
			f == kSupportsSavingDuringRuntime;
	}
	// Save games (saves.cpp)
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override;
	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override;
	Common::Error saveGameStream(Common::WriteStream *stream, bool isAutosave = false) override;
	Common::Error loadGameStream(Common::SeekableReadStream *stream) override;

	/**
	 * Unit 2472: the palette; `dimFactor` > 0 dims colours 16..255 as the
	 * palette handler's interval does (2472:011d), in 16ths.
	 */
	/** dimFactor 0 = the palette as it is; the dimmed colours are first..last. */
	void setPalette(int palette, int dimFactor = 0, int first = 16, int last = 255);
	/** A room's palette, dimmed while its light is off (objects_home.cpp). */
	void setRoomPalette(int palette);
	bool isRoomDark() const;

	int getLocation() const { return _location; }
	/** Debugging: the location's points (F5). */
	int getPoints() const { return _score[0]; }
	void setPoints(int points) { _score[0] = points; }
	int getRoom() const { return _room; }
	Scene *getScene() const { return _scene; }
	/**
	 * Debugging (console "pic"): an archive picture drawn over the room at
	 * (x, y) until the next click or key; 0 = none.
	 */
	void showDebugPicture(uint number, int x, int y);

	/** Start a location: reset its doors. */
	void enterLocation(int location);
	/** GoTo(room, node) in the current location; node = entry walk node. */
	bool gotoRoom(int room, int node);
	Common::HashMap<uint16, bool> &getDoors() { return _doors; }
	void updateDoor(uint16 object);
	void executeVerb(int verb, int object, int second = 100); // 100 = kNoObject, no second object
	/** Play a PC-speaker effect and wait for it, as the original does. */
	void playSound(SoundEffectId effect);
	bool isCarried(int object) const;
	/** The object a proxy stands for (VMT DS:baa2, +0x30), or 0. */
	int getProxyTarget(int object) const;
	/** VMT 0x20 (232c:0246, root +0x2b): Benyt/Giv with it asks for a second object. */
	bool takesSecondObject(int object) const;
	/** Proxies' verbs (unit ac7f); false if `object` is no proxy. */
	bool runProxyVerb(int verb, int object, int second);
	/** The kitchen proxies whose subclass overrides Tag (runKitchenProxyTag). */
	bool isKitchenTagProxy(int proxy) const;
	void removeObject(int object);

	// Object state and class handlers (objects_home.cpp)
	Common::String handlerText(uint32 address);
	int getField(int object, int field) const;
	void setField(int object, int field, int value);
	int getFlag(int object) const { return getField(object, 0x2f); }
	void setFlag(int object, int value) { setField(object, 0x2f, value); }
	bool isFatherCold() const;
	void resetObjectState(int location);
	bool isObjectEnabled(int object) const;
	int getDoorPartner(int object) const;
	/** The object's region as its class sets it (28c3:0233); `r` holds the built one. */
	void adjustRegion(int object, Common::Rect &r) const;
	/** The state the object's VMT 0x58 gives its figure; 0 if not known. */
	int getFigureState(int object, bool &visible) const;
	void refreshObject(int object);
	void award(int points, int energy, int waste, int water, uint32 text, uint32 page);
	void takeObject(int object);
	/** b06f:1185: the quiz of the pending award (text addresses as handlerText). */
	void setQuiz(uint32 question, uint32 answer1, uint32 answer2, int correct, int category);
	/** The root class's text for a verb ("Det kan ikke lade sig gøre." ...). */
	void sayRootMessage(int verb);
	/** A lamp's award for its new bulb (its VMT 0x64). */
	void lampAward(int lamp);
	/** A bulb pack's VMT 0x68: one bulb less; empty packs leave the inventory. */
	void useBulb(int pack);
	/** Benyt/Tænd/Sluk on the TV or the video: close-up 18 / 19 and the award. */
	void useTVCloseUp(int object);
	/** The paper holders 149/151 take the name of the paper on them. */
	void updateHolderName(int holder);
	/** The location's threshold and maxima (manager +0..+0x10). */
	const LocationTarget *getLocationTarget() const;
	/** 13ec:07ce: an exit object; enough points end the phase (b06f:107e). */
	void useExit();
	/** Testing aid: every carriable object of the location into the inventory. */
	void giveAllItems();
	bool runClassHandler(uint32 handler, int verb, int object, int second);
	void awardText(int points, int energy, int waste, int water, const Common::String &text,
		const Common::String &page);
	bool useWaste(int item, int container);

	// Awards and the response screen (response.cpp)
	/** b331: the response screen; returns true if "?" was chosen. */
	bool runResponse(const Common::String &text, const Common::String &page, const int values[4], bool showHelp);
	/** b21c:061a: the quiz dialog; returns 1 for the first answer, 0 for the second. */
	int runQuiz(const Common::String &question, const Common::String &answer1, const Common::String &answer2);
	/** b06f:13d8 after the player has finished talking: screen, then the score. */
	void showPendingAward();
	void redrawRoom();

	// Close-ups drawn by code (closeup.cpp)
	/**
	 * Unit 8e40: the thermostat close-up (rooms 16, 17). Returns the chosen
	 * level (0..3), or -1 when cancelled.
	 */
	int runThermostat(uint16 picture, int dx, int dy, int level, const int temperatures[4]);
	/** The thermostat-like close-ups: a picture, four dials, a display; returns 0..3 or -1. */
	int runDialScreen(uint16 picture, uint16 firstDial, const Common::String &title,
		int displayX, int displayY, const uint16 displayImages[4], int level);
	/** Close-up 25 (unit 8af9): water for the eggs, 1..4 dl, or -1. */
	int runEggWater();
	/**
	 * Close-ups 18 / 19 (unit 8d9d): the TV and the video; the one used
	 * (video or not) can be switched. False when cancelled.
	 */
	bool runTVScreen(bool video, int &tv, int &vcr);
	/**
	 * Unit 80ab: a choice screen (the school's close-ups 12, 13): a picture,
	 * four option images, OK if `hasOk`. Returns the choice 1..4, or 0.
	 */
	int runChoiceScreen(uint16 picture, int pictureX, int pictureY, uint16 firstOption,
		const Common::String &title, bool hasOk, int initial);
	/** Unit 8000: the temperature screen (the school's close-up 14); the chosen value. */
	int runTemperatureScreen(int value, int minValue, int maxValue);
	/** Close-up 26 (unit 8a94): the little brother's bath, choice 1..6, or -1. */
	int runBathChoice();
	/** Close-ups 27/28: a picture, extra images and an OK button. */
	void runInfoScreen(uint16 picture, const uint16 *images, const Common::Point *pos, int count);
	/** Close-up 27 (EXE unit 128e): the washing powder box with the towns' water hardness. */
	void runPowderBox(const int hardness[6]);
	/** Close-up 28 (EXE unit 1268): the dosing chart. */
	void runSchedule();
	/** Close-up 29 (EXE unit 11f8): the dosing scale; returns the dose, 0..240 ml. */
	int runDosing();
	/** Close-up 30 (unit 8a01): program and temperature; false when cancelled. */
	bool runWashProgram(int &program, int &temperature);
	/** 9909:0544 + 0184: the washing machine starts; the award for the wash. */
	void washingAward();
	/** Close-up 15 (unit 8ebc): how to peel the potatoes, 0..2, or -1. */
	int runPotatoPeeling();
	void drawScreenTitle(const Common::String &title);
	/** Close-ups 13/14 (unit 8f1b): the temperature, or -999 when cancelled. */
	int runThermostatDisplay(uint16 picture, int value);
	void thermostatAward(int object);
	void useRadiator(int object);

	// The garden (objects_garden.cpp)
	bool runGardenHandler(uint32 handler, int verb, int object, int second);
	int getGardenFigureState(uint32 rule, int object, bool &visible) const;
	void tickGarden();
	bool isHoseRunning(int hose) const;
	void refreshSprinklers();
	void setMealybugsShown(bool shown);
	void startWater(int source);
	void waterEnded(int source);
	/** Room VMT 0x24 (2395:02ec): input off with the hourglass. */
	void setInputOff(bool off);
	bool _inputOff = false;
	uint _debugPicture = 0;
	Common::Point _debugPicturePos;
	// Save games (saves.cpp)
	bool _canSaveNow = false;  // set only around the room loop's pollEvent
	bool _loadedGame = false;  // a save was loaded: build its room
	int _loadedNode = 0;       // the player's node in the save
	bool isGameIdle() const;
	void syncGame(Common::Serializer &s);
	void enterLoadedRoom();
	int _greenhouseStage = 0; // the greenhouse's look: 1 wait, 2 magnifier, 3 wait
	int _greenhouseTimer = 0;

	// The bathroom (objects_bathroom.cpp)
	bool runBathroomHandler(uint32 handler, int verb, int object, int second);
	int getBathroomFigureState(uint32 rule, int object, bool &visible) const;
	void tickBathroom();
	/** a000:bc98: the bathroom door lets the player out only with the cold tap closed. */
	bool canLeaveBathroom(int door);
	/**
	 * The bathroom was built (again): the little brother's binding (VMT 0x58)
	 * and the start of the room's run (VMT 0x38 = 90f3:0ef4).
	 */
	void bathroomBuilt();
	/** 9909:0b84: the washing powder's six random water hardness values. */
	void initWashingPowder();
	/** Tænd on the washing machine: close-up 30, then back to the bathroom. */
	void startWashProgram();
	/** a000:6890: the little brother's figure, a state machine (+0x30). */
	void littleBrotherFigure();
	/** a000:682e (VMT 0x68): on to the next state at the next figure update. */
	void littleBrotherNext();
	bool isLittleBrotherRegionOn() const;
	bool _brotherWalking = false;  // his walk ends with message 0x1f (2381:0065)

	// The school (objects_school.cpp)
	bool runSchoolHandler(uint32 handler, int verb, int object, int second);
	int getSchoolFigureState(uint32 rule, int object, bool &visible) const;
	void initSchool();
	/** Classroom 1 built (again): the radiator's and the notice's awards (VMT 0x58). */
	void schoolBuilt();
	/** The bat swing (messages 0x1f..0x21 of classroom 1, 82d8:0226). */
	void tickSchool();
	void useSchoolRadiator();
	void useNotice();
	/** 8557:2286: a spot is "tom plads" or has the bin's name. */
	void updateSpotName(int spot);
	int _batStage = 0;
	bool isSchoolObjectEnabled(int object, bool &enabled) const;
	// The slaughterhouse (objects_slaughter.cpp)
	void initSlaughterhouse();
	void initExitDoors(int location);
	// The supermarket (objects_super.cpp)
	void initSupermarket();
	bool isSuperObjectEnabled(int object, bool &enabled) const;
	void adjustSuperRegion(int object, Common::Rect &r) const;
	int getSuperFigureState(uint32 rule, int object, bool &visible) const;
	void sortSuperWaste(int object, int target);
	int runSuperTemperatureScreen(int value);
	int runSuperLightScreen(int value);
	void superPanelAward(int panel);
	bool isCabinetOn(int cabinet) const;
	void storeBox(int box, int freezer);
	void countStoredBox();
	bool runSuperHandler(uint32 handler, int verb, int object, int second);
	// The factory (objects_factory.cpp)
	void initFactory();
	bool isFactoryObjectEnabled(int object, bool &enabled) const;
	/** VMT 0x24: the node the player walks to for this object. */
	int getObjectNode(int object, int regionNode) const;
	int getFactoryFigureState(uint32 rule, int object, bool &visible) const;
	void sortPeels(int target);
	int getFactoryLineState(int object, bool &visible) const;
	void factoryRoomEntered();
	void factoryLeavingRoom();
	void tickFactory();
	void runStationIfPending(int station);
	void factoryMessage(int message);
	void waitForLine(int figure, int message);
	void showLineFigure(int figure);
	void glassMachineAward();
	void autoclaveRule();
	void autoclaveAward();
	void shrinkMachineRule();
	void runAutoclaveScreen();
	struct LineWait {
		int figure;
		int message;
	};
	Common::Array<LineWait> _lineWaits; // messages at a figure's walk end
	bool _lineShown[28];                // the line's figures 951..978 shown
	bool runFactoryHandler(uint32 handler, int verb, int object, int second);
	void slaughterRoomEntered();
	bool isSlaughterObjectEnabled(int object, bool &enabled) const;
	int getSlaughterFigureState(uint32 rule, int object, bool &visible) const;
	bool runByproductScreen(int kind);
	/**
	 * A click on a button (28c3:0883 / 08a8 / 08f8): drawn pressed while the
	 * mouse button is held and the mouse is over it; true when it is released
	 * over it (the button's command). The arguments are drawBevelButton's.
	 */
	bool trackButton(const Graphics::ManagedSurface &image, int x, int y, int frame = 1, int bevel = 1,
		int light = 15, int corner = 7);
	Common::String byproductText(int kind);
	bool runSlaughterHandler(uint32 handler, int verb, int object, int second);
	// The chicken farm (objects_chicken.cpp)
	void initChickenFarm();
	void chickenFarmBuilt();
	bool isChickenFarmObjectEnabled(int object, bool &enabled) const;
	void emptyBarrow(int value, uint32 text);
	void dropDeadChickens(int value, uint32 text);
	void moveBarrow(bool toDungRoom);
	void tickChickenFarm();
	bool _hosing = false;   // the chicken farm's dung room is being hosed
	void panelAward(int panel);
	void runPictureScreen(uint16 picture, int number);
	bool runPanelScreen(int panel);
	bool isHumidityRight() const;
	bool isTemperatureRight() const;
	bool areChickensWell() const;
	int getChickenDimFactor() const;
	int getChickenFigureState(uint32 rule, int object, bool &visible) const;
	bool runChickenHandler(uint32 handler, int verb, int object, int second);
	// The farm (objects_farm.cpp)
	void initFarm();
	void farmBuilt();
	void giveFarmItems();
	void updatePileName(int pile);
	void updateFieldName(int field);
	void setItemCount(int item, int count);
	void setPileCount(int pile, int count);
	void usePileItem(int item, int second);
	void feedCows(int feed);
	void workField(int machine, int field, int anim, int stage);
	void waterField(int sprinkler, int field);
	void machineAward(int machine);
	void fieldAward(int points, int energy, int waste, int water, const Common::String &text, uint32 page);
	void runFieldWork(int anim, bool leftField, int oldStage, int newStage, int otherStage);
	void startBagging();
	void nextBag();
	bool _bagging = false;   // room 9's bag is falling
	bool _showBag = false;   // show the bag at the next tick
	int _sprayObject = 0;    // the field sprinkler's spray that is running
	void tickFarm();
	void room9Award(int value, uint32 text, int object);
	void loadCrate(int crate, int item);
	int _bagsLeft = 0;       // room 9's bagging: bags still to come
	void updatePotatoes(int object);
	bool isFarmObjectEnabled(int object, bool &enabled) const;
	int getFarmDimFactor() const;
	int getFarmFigureState(uint32 rule, int object, bool &visible) const;
	bool runFarmHandler(uint32 handler, int verb, int object, int second);
	void adjustSchoolRegion(int object, Common::Rect &r) const;
	/** The gym's palette dimming (8557:2bd7): 0 = none, else the factor. */
	int getSchoolDimFactor() const;
	/** 8557:2d64: light in the gym (a switch on or the curtains open). */
	bool hasGymLight() const;
	int gymSwitchesOn() const;
	void gymBuilt();
	void refreshGymLight();
	void tickGym();
	void usePanel();
	int _gymStage = 0;   // the boys coming in (messages 0x1f..0x21 of 81da)

	// The kitchen (objects_kitchen.cpp)
	bool runKitchenHandler(uint32 handler, int verb, int object, int second);
	int getKitchenFigureState(uint32 rule, int object, bool &visible) const;
	void tickKitchen();
	/** a000:a2c0: the kitchen door asks the fridge and the freezer. */
	bool canLeaveKitchen(int door);
	/** Whether a kitchen proxy's place (fridge, freezer, oven, cupboard) is open. */
	bool isKitchenProxyOpen(int proxy) const;
	/** Regions that depend on other objects; false if `object` has no such rule. */
	bool isKitchenObjectEnabled(int object, bool &enabled) const;
	/** Proxies with their own Tag; true if it is done, false to continue with ac7f:025c. */
	bool runKitchenProxyTag(int proxy);
	void refreshFridge();
	void cookEggs();
	void refreshEggTray();
	int _toasterTimer = 0;

	// The shop (objects_shop.cpp)
	bool runShopHandler(uint32 handler, int verb, int object, int second);
	int getShopFigureState(uint32 rule, int object, bool &visible) const;
	void tickShop();
	void assistantSay(const Common::String &text);
	int getBasketTotal() const;
	void putBasketBack();
	/** Called before the player leaves `room` of the current location. */
	void leavingRoom(int room);
	Common::Array<uint16> _basket; // DS:c9ea: the fetched goods (shelf IDs)
	int _shopFetching = 0;         // the assistant: 1 walking to the goods, 2 back
	int _shopItem = 0;
	int _shopDistance = 0;         // +0x32
	int _shopBackState = 1;        // +0x38
	/** b000:03c7: show a message; the player talks while it shows. */
	void say(const Common::String &text, byte color, bool talk);
	void clearMessage();
};

extern MiljostrupEngine *g_engine;
#define SHOULD_QUIT ::Miljostrup::g_engine->shouldQuit()

} // End of namespace Miljostrup

#endif // MILJOSTRUP_H
