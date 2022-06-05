LIB_NAME = librazor.a
TEST_EXE = razortest

SRC_FILES = $(wildcard src/*.cpp)
HEADER_FILES = $(wildcard src/*.h)
OBJ_FILES := $(patsubst src/%.cpp, obj/%.o, $(SRC_FILES))
D_FILES := $(patsubst src/%.cpp, obj/%.d, $(SRC_FILES))

CC = g++-10
AR = ar

GITCOMMIT = $(shell git log -1 --pretty=format:"%H")

COMPILER_FLAGS = -MMD -w -std=c++17 -DGIT_COMMIT=\"$(GITCOMMIT)\" -Iinclude -fsanitize-undefined-trap-on-error 
COMPILER_FLAGS_DEBUG = -ggdb -g

LINKER_FLAGS = -lpthread -lSDL2 -lSDL2_net -lrazor

ifeq ($(OS),Windows_NT)
	CC = g++
	COMPILER_FLAGS_RELEASE += -mwindows
	LINKER_FLAGS += -lmsvcrt
else
#	LINKER_FLAGS += -lGL -lGLEW -lGLU -ldl -lsteam_api
endif

obj/%.o: src/%.cpp
	$(CC) $(COMPILER_FLAGS) $(COMPILER_FLAGS_DEBUG) -c -o $@ $<

$(LIB_NAME): $(OBJ_FILES)
	$(AR) rcs $@ $^

clean :
	-rm $(OBJ_FILES) $(LIB_NAME) $(D_FILES)

release: COMPILER_FLAGS += -O3 -ffast-math -pipe
release: COMPILER_FLAGS_DEBUG = 

native: COMPILER_FLAGS += -O3 -ffast-math -pipe -mtune=native
native: COMPILER_FLAGS_DEBUG = 

native release: $(LIB_NAME)

test: test/main.cpp
	$(CC) $(COMPILER_FLAGS) $(COMPILER_FLAGS_DEBUG) -o $(TEST_EXE) $< $(LINKER_FLAGS)

# this has to do with -MMD and generates a depedency graph for objects
-include $(OBJ_FILES:.o=.d)