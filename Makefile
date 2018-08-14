test : test.o
	g++ test.o -o test
test.o : test.cpp
	g++ -c test.cpp -std=c++11
clean :
	rm test.o test
