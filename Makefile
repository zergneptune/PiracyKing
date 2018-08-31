test : test.o practice.o net.o
	g++ test.o practice.o net.o -o test
test.o : test.cpp
	g++ -c test.cpp -std=c++11
practice.o : practice.cpp
	g++ -c practice.cpp -std=c++11
net.o : net.cpp
	g++ -c net.cpp -std=c++11
clean :
	rm test.o practice.o net.o test
