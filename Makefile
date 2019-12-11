all: $(wildcard src/*.cpp) $(wildcard src/*.hpp)
	g++ $(wildcard src/*.cpp) -g -Og -std=c++17 -Wall -Wextra -o test.out

