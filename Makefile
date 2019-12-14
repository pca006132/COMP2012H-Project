all: $(wildcard src/*.cpp) $(wildcard src/*.hpp)
	clang++ $(wildcard src/*.cpp) -g -Og -std=c++17 -Wall -Wextra -o test.out

test: all
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./test.out
	@valgrind --tool=massif ./test.out

