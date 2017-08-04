all: json_sample Game.o AoI

json_sample: json_sample.cpp
	$(CXX) -std=c++11 -o json_sample json_sample.cpp jsoncpp/jsoncpp.cpp

Game.o: src/Game.cpp src/Game.h
	$(CXX) -std=c++11 -o Game.o -c src/Game.cpp

AoI.o: AI/AoI.cpp
	$(CXX) -std=c++11 -o AoI.o -c AI/AoI.cpp 

AoI: AoI.o
	$(CXX) -std=c++11 -o AoI AoI.o Game.o
