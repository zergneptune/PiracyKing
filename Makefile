all : test server client
.PHONY : all
OBJ1 = test.o practice.o
OBJ2 = server.o
OBJ3 = client.o

test : $(OBJ1)
	g++ $(OBJ1) -o test
server : $(OBJ2)
	g++ $(OBJ2) -o server
client : $(OBJ3)
	g++ $(OBJ3) -o client

$(OBJ1): %.o: %.cpp
	g++ -c $< -o $@ -std=c++11

$(OBJ2): %.o: %.cpp
	g++ -c $< -o $@ -std=c++11

$(OBJ3): %.o: %.cpp
	g++ -c $< -o $@ -std=c++11

.PHONY : clean_o clean_exe

clean_o :
	rm *.o 

clean_exe :
	test server client
