test : test.o practice.o
	g++ test.o practice.o -o test
server : server.o
	g++ server.o -o server
client : client.o
	g++ client.o -o client
test.o : test.cpp
	g++ -c test.cpp -std=c++11
practice.o : practice.cpp
	g++ -c practice.cpp -std=c++11
server.o : server.cpp
	g++ -c server.cpp -std=c++11
client.o : client.cpp
	g++ -c client.cpp -std=c++11
clean :
	rm server.o client.o test.o practice.o test
