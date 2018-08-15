test : test.o practice.o
	g++ test.o practice.o -o test
test.o : test.cpp
	g++ -c test.cpp -std=c++11
practice.o : practice.cpp
	g++ -c practice.cpp -std=c++11
clean :
	rm test.o practice.o test
