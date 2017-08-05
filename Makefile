
LIBPROFILER =
ifeq ($(LIBPROFILER),-lprofiler)
DEFINES = -DHAVE_CPU_PROFILER
else
DEFINES =
endif

PROFILER =
CXXFLAGS = -g -MMD -MP -O2 -Wall -Wextra -std=c++11 -I./lib $(DEFINES)

BASE_OBJS = ./obj/lib/jsoncpp.o ./obj/lib/Game.o ./obj/lib/MCTS_core.o

LIB_SRCS = $(wildcard ./lib/*.cpp)
LIB_OBJS = $(LIB_SRCS:./lib/%.cpp=./obj/lib/%.o)
LIB_BINS = $(patsubst ./lib/%_main.cpp,./bin/lib/%,$(filter %_main.cpp,$(LIB_SRCS)))

AI_SRCS  = $(wildcard ./AI/*.cpp)
AI_OBJS  = $(AI_SRCS:./AI/%.cpp=./obj/%.o)
AI_BINS  = $(AI_SRCS:./AI/%.cpp=./bin/%)

TARGETS  = $(LIB_OBJS) $(LIB_BINS) $(AI_OBJS) $(AI_BINS)
DEPENDS  = $(LIB_OBJS:.o=.d) $(AI_OBJS:.o=.d)


all: $(TARGETS)

./obj/lib/%.o: ./lib/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) -c -o $@ $<

./bin/lib/%: ./obj/lib/%_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

./obj/%.o: ./AI/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

./bin/%: ./obj/%.o $(BASE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBPROFILER)

# add dependency manually
./bin/lib/eval: $(BASE_OBJS)
./bin/MCTS_greedy ./bin/MCTS ./bin/MCTS_weak: ./obj/lib/MCTS_core.o
# ./bin/my_super_AI: ./obj/lib/my_super_lib.o

clean:
	$(RM) $(TARGETS) $(DEPENDS)

-include $(DEPENDS)

.PHONY: all clean
