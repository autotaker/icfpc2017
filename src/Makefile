CXX = g++

OS = ${shell uname}
ifeq ($(OS), Darwin)
CXXFLAGS = -O2 -g -std=c++11 -stdlib=libc++
else
CXXFLAGS = -O2 -g -std=c++11
endif


JSONCPP_OBJ = jsoncpp.o
JSONCPP_SRC = ../jsoncpp/jsoncpp.cpp

EVAL_OBJS = Game.o Evaluation.o
EVAL_OUT = eval
$(EVAL_OUT): $(EVAL_OBJS) $(JSONCPP_OBJ)
	$(CXX) $(CXXFLAGS) -o $(EVAL_OUT) $(JSONCPP_OBJ) $(EVAL_OBJS)

RANDOM_OBJS = Game.o Random.o
RANDOM_OUT = random
$(RANDOM_OUT): $(RANDOM_OBJS) $(JSONCPP_OBJ)
	$(CXX) $(CXXFLAGS) -o $(RANDOM_OUT) $(JSONCPP_OBJ) $(RANDOM_OBJS)

all: $(EVAL_OUT) $(RANDOM_OUT)


$(JSONCPP_OBJ): $(JSONCPP_SRC)
	$(CXX) $(CXXFLAGS) -c $(JSONCPP_SRC) -o $(JSONCPP_OBJ)

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: clean
clean:
	rm -rf $(EVAL_OBJS)
	rm -rf $(RANDOM_OBJS)
	rm -rf $(JSONCPP_OBJ)
	rm -rf $(EVAL_OUT)
	rm -rf $(RANDOM_OUT)
