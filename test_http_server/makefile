.PHONY : all
all : webhook_server

OBJ1 = test_http_client.cpp test_http_server.cpp

webhook_server : $(OBJ1)
	g++ $(OBJ1) -std=c++11 -ljsoncpp -levent -lpthread -o webhook_server

.PHONY : clean

clean : clean_o clean_exe

clean_o :
	rm *.o 

clean_exe :
	rm webhook_server
