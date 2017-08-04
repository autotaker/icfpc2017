all: json_sample Game.o jsoncpp.o bin/AoI

bin:
	mkdir bin

jsoncpp.o: jsoncpp/jsoncpp.cpp
	$(CXX) -std=c++11 -o jsoncpp.o -c jsoncpp/jsoncpp.cpp

json_sample: json_sample.cpp jsoncpp.o
	$(CXX) -std=c++11 -o json_sample json_sample.cpp jsoncpp.o

Game.o: src/Game.cpp src/Game.h
	$(CXX) -std=c++11 -o Game.o -c src/Game.cpp

AoI.o: AI/AoI.cpp
	$(CXX) -std=c++11 -o AoI.o -c AI/AoI.cpp 

bin/AoI: bin AoI.o
	$(CXX) -std=c++11 -o bin/AoI AoI.o Game.o jsoncpp.o
