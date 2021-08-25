CXX = g++
CXX_FLAGS = -O2 --std=c++17 -Wall -Wextra

all:
	$(CXX) test/main.cc $(CXX_FLAGS) -o main
