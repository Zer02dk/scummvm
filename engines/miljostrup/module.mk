MODULE := engines/miljostrup

MODULE_OBJS = \
	archive.o \
	closeup.o \
	console.o \
	font.o \
	interface.o \
	metaengine.o \
	miljostrup.o \
	objects_bathroom.o \
	objects_chicken.o \
	objects_factory.o \
	objects_farm.o \
	objects_garden.o \
	objects_home.o \
	objects_kitchen.o \
	objects_school.o \
	objects_slaughter.o \
	objects_super.o \
	objects_shop.o \
	response.o \
	rooms.o \
	saves.o \
	scene.o \
	sound.o \
	startup.o \
	users.o \
	verbs.o

# This module can be built as a plugin
ifeq ($(ENABLE_MILJOSTRUP), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
