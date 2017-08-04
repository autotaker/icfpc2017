CXX = g++
CXXFLAGS = -O2 -g -std=c++11 -stdlib=libc++

JSONCPP_OBJ = jsoncpp.o
JSONCPP_SRC = ../jsoncpp/jsoncpp.cpp

EVAL_OBJS = Game.o Evaluation.o
EVAL_OUT = eval
$(EVAL_OUT): $(EVAL_OBJS) $(JSONCPP_OBJ)
	$(CXX) $(CXXFLAGS) -o $(EVAL_OUT) $(JSONCPP_OBJ) $(EVAL_OBJS) 

$(JSONCPP_OBJ): $(JSONCPP_SRC)
	$(CXX) $(CXXFLAGS) -c $(JSONCPP_SRC) -o $(JSONCPP_OBJ)

.SUFFIXES: .cpp .o

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: clean
clean:
	rm $(EVAL_OBJS)
	rm $(JSONCPP_OBJ)
	rm $(EVAL_OUT)
