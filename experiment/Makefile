all:
	$(CXX) -g -MMD -MP -O2 -Wall -Wextra -std=c++11 -I../lib Ran.cpp ../obj/lib/jsoncpp.o ../obj/lib/Game.o -o Ran
	$(CXX) -g -MMD -MP -O2 -Wall -Wextra -std=c++11 -I../lib solver_greedy.cpp ../obj/lib/jsoncpp.o ../obj/lib/Game.o -o solver_greedy
	$(CXX) -g -MMD -MP -O2 -Wall -Wextra -std=c++11 -I../lib solver_japlj.cpp ../obj/lib/jsoncpp.o ../obj/lib/Game.o -o solver_japlj

.PHONY: clean
clean:
	rm *.d
	rm Ran
	rm solver_greedy
	rm solver_japlj
	
