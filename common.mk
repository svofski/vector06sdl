SRC_DIRS ?= src fast-filters/sources
RESOURCES += boots singlepass_vsh singlepass_fsh icon64
TEST_SRCS ?= test/tests.cpp
TARGET_TEST ?= tests

ifneq ($(ARCH), )
    ARCHPREFIX=$(ARCH)-
    ARCHSUFFIX = -$(ARCH)
endif

CC := $(ARCHPREFIX)gcc
CXX := $(ARCHPREFIX)g++

BUILD_DIR ?= build$(ARCHSUFFIX)

ifneq ($(LIBROOT), )
    SDL2_CONFIG ?= $(LIBROOT)/bin/sdl2-config
    LDFLAGS += -L$(LIBROOT)
else
    SDL2_CONFIG ?= sdl2-config
endif

ifneq ($(SYSROOT), )
    SYS_INC_DIRS := $(SYSROOT)/include $(LIBROOT)/include
endif

all:	$(BUILD_DIR)/$(TARGET_EXEC) $(BUILD_DIR)/$(TARGET_TEST)

#DEBUG := -O0 -g
DEBUG := -O3
CFLAGS := -Wall -fpermissive $(DEBUG) -ffunction-sections -fdata-sections -Wl,--gc-sections
CXXFLAGS := $(CFLAGS) -std=gnu++17

BOOST_LIBS := boost_program_options$(MT) boost_system$(MT) boost_thread$(MT) boost_chrono$(MT) boost_filesystem$(MT)

ifneq ($(BOOST_LIBRARY_PATH), )
    BOOST_LDFLAGS := $(addprefix $(BOOST_LIBRARY_PATH)/lib, $(BOOST_LIBS))
    BOOST_LDFLAGS := $(addsuffix .a, $(BOOST_LDFLAGS))
else
    BOOST_LDFLAGS := $(addprefix -l,$(BOOST_LIBS))
endif

ifneq ($(BOOST_STATIC), )
    BOOST_LDFLAGS := -Wl,-Bstatic $(BOOST_LDFLAGS) -Wl,-Bdynamic
endif


SDL_CFLAGS := $(shell $(SDL2_CONFIG) --cflags)

ifneq ($(SDL_STATIC), )
    SDL_LDFLAGS := -Wl,-Bstatic $(shell $(SDL2_CONFIG) --static-libs | sed s/-mwindows//g) -lSDL2_image -lpng16 -lz -Wl,-Bdynamic 
else
    ifneq ($(SDL_LIBRARY_PATH), )
    	SDL_LDFLAGS := libSDL2.a libSDL2_image.a libpng.a libtiff.a libjpeg.a libwebp.a
    	SDL_LDFLAGS := $(addprefix $(SDL_LIBRARY_PATH)/,$(SDL_LDFLAGS))
	SDL_LDFLAGS := $(SDL_LDFLAGS) /usr/local/opt/zlib/lib/libz.a
	SDL_LDFLAGS := $(SDL_LDFLAGS) $(shell $(SDL2_CONFIG) --static-libs) 
    else
	#SDL_LDFLAGS := $(shell $(SDL2_CONFIG) --libs) -lSDL2_image -ldl -lpthread
	SDL_LDFLAGS := $(shell $(SDL2_CONFIG) --static-libs) -lSDL2_image 
    endif
endif

$(info SDL_CFLAGS:	$(SDL_CFLAGS))
$(info SDL_LDFLAGS	$(SDL_LDFLAGS))

$(info CFLAGS		$(CFLAGS))

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

TEST_OBJS := $(TEST_SRCS:%=$(BUILD_DIR)/%.o)
ALL_OBJS := $(OBJS) $(TEST_OBJS)
DEPS := $(ALL_OBJS:.o=.d)

INC_DIRS := $(SYS_INC_DIRS) $(shell find $(SRC_DIRS) -type d)

INC_DIRS := $(INC_DIRS) coreutil/sources chaiscript/include

INC_FLAGS := $(addprefix -I,$(INC_DIRS))

EXTRA_DEFS += -DCHAISCRIPT_NO_THREADS=1
EXTRA_DEFS += -DHAS_IMAGE=1
EXTRA_DEFS += -DHAVE_OPENGL=1
EXTRA_DEFS += -DUSED_XXD=1

RESOBJS := $(RESOURCES:%=$(BUILD_DIR)/%.o)

$(info RESOBJS		$(RESOBJS))

CPPFLAGS += $(INC_FLAGS) $(SDL_CFLAGS) -MMD -MP $(EXTRA_DEFS)
LDFLAGS += $(BOOST_LDFLAGS) $(SDL_LDFLAGS) $(OPENGL_LDFLAGS) $(EXTRA_LIBS)

$(info LDFLAGS		$(LDFLAGS))

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) $(RESOBJS)
	$(CXX) $(OBJS) $(RESOBJS) -o $@ $(LDFLAGS)

$(info OBJS 		$(OBJS))
$(info TEST_OBJS 	$(TEST_OBJS))

$(BUILD_DIR)/$(TARGET_TEST): $(RESOBJS) $(TEST_OBJS)
	$(CXX) $(RESOBJS) $(TEST_OBJS) -o $@ $(LDFLAGS)

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

binobj = \
	cp $(1) $(BUILD_DIR) ; \
	cd $(BUILD_DIR) ; \
	xxd -i $(notdir $(1)) $(notdir $(2)) ; \
	$(CC) -c $(2) -o $(3)

# resource
$(BUILD_DIR)/%.o: boot/%.bin
	$(call binobj,$<,$*.c,$*.o)

$(BUILD_DIR)/singlepass_vsh.o:	shaders/singlepass.vsh
	$(call binobj,$<,$(notdir $*.c),$(notdir $*.o))
	
$(BUILD_DIR)/singlepass_fsh.o:	shaders/singlepass.fsh
	$(call binobj,$<,$(notdir $*.c),$(notdir $*.o))

$(BUILD_DIR)/icon64.o:	res/icon64.rgba
	$(call binobj,$<,$(notdir $*.c),$(notdir $*.o))

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p

