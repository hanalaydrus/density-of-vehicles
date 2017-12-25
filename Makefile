build:
	g++ -std=c++11 main.cpp IPM.cpp `pkg-config --libs --cflags opencv` -o main
run:
	./main