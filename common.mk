SRC_DIRS ?= src fast-filters/sources
RESOURCES += boots singlepass_vsh singlepass_fsh icon64

ifneq ($(ARCH), )
    ARCHPREFIX=$(ARCH)-
    ARCHSUFFIX = -$(ARCH)
endif

CC := $(ARCHPREFIX)gcc
CXX := $(ARCHPREFIX)g++

BUILD_DIR ?= build$(ARCHSUFFIX)

ifneq ($(LIBROOT), )
    SDL2_CONFIG ?= $(LIBROOT)/bin/sdl2-config
    LDFLAGS := -L$(LIBROOT)
else
    SDL2_CONFIG ?= sdl2-config
endif

ifneq ($(SYSROOT), )
    SYS_INC_DIRS := $(SYSROOT)/include $(LIBROOT)/include
endif

CFLAGS := -Wall -fpermissive -O3 -ffunction-sections -fdata-sections -Wl,--gc-sections
CXXFLAGS := $(CFLAGS) -std=gnu++14

BOOST_LDFLAGS := -lboost_program_options$(MT) -lboost_system$(MT) -lboost_thread$(MT) -lboost_chrono$(MT) -lboost_filesystem$(MT)

SDL_CFLAGS := $(shell $(SDL2_CONFIG) --cflags)

ifneq ($(SDL_STATIC), )
    SDL_LDFLAGS := -Wl,-Bstatic $(shell $(SDL2_CONFIG) --static-libs) -lSDL2_image -lpng16 -lz 
else
    SDL_LDFLAGS := $(shell $(SDL2_CONFIG) --static-libs) -lSDL2_image 
endif

$(info SDL_CFLAGS:	$(SDL_CFLAGS))
$(info SDL_LDFLAGS	$(SDL_LDFLAGS))

$(info CFLAGS		$(CFLAGS))

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(SYS_INC_DIRS) $(shell find $(SRC_DIRS) -type d)

INC_DIRS := $(INC_DIRS) coreutil/sources chaiscript/include

INC_FLAGS := $(addprefix -I,$(INC_DIRS))

EXTRA_DEFS += -DCHAISCRIPT_NO_THREADS=1
EXTRA_DEFS += -DHAS_IMAGE=1
EXTRA_DEFS += -DHAVE_OPENGL=1
EXTRA_DEFS += -DUSED_XXD=1

RESOBJS := $(RESOURCES:%=$(BUILD_DIR)/%.o)

$(info RESOBJS		$(RESOBJS))

CPPFLAGS ?= $(INC_FLAGS) $(SDL_CFLAGS) -MMD -MP $(EXTRA_DEFS)
LDFLAGS += $(BOOST_LDFLAGS) $(SDL_LDFLAGS) $(OPENGL_LDFLAGS) $(EXTRA_LIBS)

$(info LDFLAGS		$(LDFLAGS))

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) $(RESOBJS)
	$(CXX) $(OBJS) $(RESOBJS) -o $@ $(LDFLAGS)

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

