all : test server client
.PHONY : all
OBJ1 = test.o practice.o
OBJ2 = server.o
OBJ3 = client.o
OBJ4 = game.o
OBJ5 = utility.o

test : $(OBJ1)
	g++ $(OBJ1) -o test
server : $(OBJ2) $(OBJ5)
	g++ $(OBJ2) $(OBJ5) -o server
client : $(OBJ3) $(OBJ4) $(OBJ5)
	g++ $(OBJ3) $(OBJ4) $(OBJ5) -o client

$(OBJ1): %.o: %.cpp
	g++ -c $< -o $@ -std=c++11

$(OBJ2): %.o: %.cpp
	g++ -c $< -o $@ -std=c++11

$(OBJ3): %.o: %.cpp
	g++ -c $< -o $@ -std=c++11

$(OBJ4): %.o: %.cpp
	g++ -c $< -o $@ -std=c++11

$(OBJ5): %.o: %.cpp
	g++ -c $< -o $@ -std=c++11

.PHONY : clean

clean : clean_o clean_exe

clean_o :
	rm *.o 

clean_exe :
	rm test server client
