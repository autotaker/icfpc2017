BUILD_DIR = ./bin
CXXFLAGS  = -g -MMD -MP -O2 -Wall -Wextra -std=c++11

JSONCPP   = jsoncpp/jsoncpp.cpp
LIB_SRCS  = $(shell ls ./lib/*.cpp) $(JSONCPP)
LIB_OBJS  = $(LIB_SRCS:.cpp=.o)
AI_SRCS   = $(wildcard ./AI/*.cpp)
AI_OBJS   = $(AI_SRCS:.cpp=.o)
AIS       = $(AI_OBJS:./AI/%.o=$(BUILD_DIR)/%)

TARGETS   = $(AIS) $(BUILD_DIR)/eval
DEPENDS   = $(LIB_OBJS:.o=.d) $(AI_OBJS:.o=.d)

all: $(TARGETS)

$(BUILD_DIR)/%: ./AI/%.o $(LIB_OBJS)
	$(CXX) -o $@ $(CXXFLAGS) $< $(filter-out %Evaluation.o,$(LIB_OBJS))

$(BUILD_DIR)/eval: $(LIB_OBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(LIB_OBJS)

clean:
	$(RM) $(TARGETS) $(LIB_OBJS) $(AI_OBJS) $(DEPENDS)

-include $(DEPENDS)

.PHONY: all clean
