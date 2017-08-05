CXXFLAGS = -g -MMD -MP -O2 -Wall -Wextra -std=c++11 -I./lib

BASE_OBJS = ./obj/lib/jsoncpp.o ./obj/lib/Game.o

LIB_SRCS = $(wildcard ./lib/*.cpp)
AI_SRCS  = $(wildcard ./AI/*.cpp)
LIB_OBJS = $(LIB_SRCS:./lib/%.cpp=./obj/lib/%.o)
AI_OBJS  = $(AI_SRCS:./AI/%.cpp=./obj/%.o)
LIB_BINS = $(patsubst ./obj/lib/%_main.o,./bin/lib/%,$(filter %_main.o,$(LIB_OBJS)))
AI_BINS  = $(AI_SRCS:./AI/%.cpp=./bin/%)

TARGETS  = $(LIB_OBJS) $(LIB_BINS) $(AI_OBJS) $(AI_BINS)
DEPENDS  = $(LIB_OBJS:.o=.d) $(AI_OBJS:.o=.d)


all: $(TARGETS)

./obj/lib/%.o: ./lib/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

./obj/%.o: ./AI/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

./bin/lib/%: ./obj/lib/%_main.o $(BASE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

./bin/%: ./obj/%.o $(BASE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# add dependency manually
# ./bin/my_super_ai: ./obj/lib/my_super_lib.o

clean:
	$(RM) $(TARGETS) $(DEPENDS)

-include $(DEPENDS)

.PHONY: all clean
