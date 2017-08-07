
LIBPROFILER =
ifeq ($(LIBPROFILER),-lprofiler)
DEFINES = -DHAVE_CPU_PROFILER
else
DEFINES =
endif

PROFILER =
CXXFLAGS = -g -MMD -MP -O2 -Wall -Wextra -std=c++11 -I./lib $(DEFINES)

LIB_SRCS = $(wildcard ./lib/*.cpp)
LIB_OBJS = $(LIB_SRCS:./lib/%.cpp=./obj/lib/%.o)
LIB_BINS = $(patsubst ./lib/%_main.cpp,./bin/lib/%,$(filter %_main.cpp,$(LIB_SRCS)))

AI_SRCS  = $(wildcard ./AI/*.cpp)
AI_OBJS  = $(AI_SRCS:./AI/%.cpp=./obj/%.o)
AI_BINS  = $(AI_SRCS:./AI/%.cpp=./bin/%)

TARGETS  = $(LIB_OBJS) $(LIB_BINS) $(AI_OBJS) $(AI_BINS)
DEPENDS  = $(LIB_OBJS:.o=.d) $(AI_OBJS:.o=.d)


all: $(TARGETS)

# objects dependency
BASE_OBJS = ./obj/lib/jsoncpp.o ./obj/lib/Game.o
USE_MCTS  = ./bin/MCTS ./bin/MCTS_weak ./bin/MCTS_greedy ./bin/Otome ./bin/Yurika ./bin/KakeUdon ./bin/MCUdon ./bin/MClight ./bin/NegAInoido ./bin/MCSlowLight ./bin/MCGreedy2 ./bin/KimeraTest 
USE_FLOWLIGHT = ./bin/Flowlight ./bin/TrueGreedy2Flowlight ./bin/Genocide ./bin/KimeraTest
./bin/lib/eval: $(BASE_OBJS)
$(USE_MCTS): ./obj/lib/MCTS_core.o
$(USE_FLOWLIGHT): ./obj/lib/FlowlightUtil.o

./obj/lib/%.o: ./lib/%.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) -c -o $@ $<

./bin/lib/%: ./obj/lib/%_main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBPROFILER)

./obj/%.o: ./AI/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

./bin/%: ./obj/%.o $(BASE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBPROFILER)

clean:
	$(RM) $(TARGETS) $(DEPENDS)

-include $(DEPENDS)

.PHONY: all clean
