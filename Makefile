all: json_sample Game.o jsoncpp.o

jsoncpp.o: jsoncpp/jsoncpp.cpp
	$(CXX) -std=c++11 -o jsoncpp.o -c jsoncpp/jsoncpp.cpp

json_sample: json_sample.cpp jsoncpp.o
	$(CXX) -std=c++11 -o json_sample json_sample.cpp jsoncpp.o

Game.o: src/Game.cpp src/Game.h
	$(CXX) -std=c++11 -o Game.o -c src/Game.cpp
