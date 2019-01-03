#CXXFLAGS=-std=c++11 -O2 -Wall 
#CXXFLAGS=-std=c++11 -Wall -g 
CXXFLAGS=-Wall -g
CXX=/usr/bin/clang++
LD=${CXX} 
CC=${CXX}

main: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o
	for i in *.cpp; do rm -fv "$${i%.cpp}"; done
