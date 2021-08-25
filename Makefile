CXX = clang++
CXX_FLAGS = -O2 --std=c++17

all:
	$(CXX) test/main.cc $(CXX_FLAGS) -o main
