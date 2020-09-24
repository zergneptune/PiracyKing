.PHONY: all
all: test

OBJ1 = test.o

test: $(OBJ1)
	g++ $(OBJ1) -lavutil -lavdevice -lavformat -o test

$(OBJ1): %.o: %.cpp
	g++ -c -g $< -o $@ -std=c++11

.PHONY : clean

clean : clean_o clean_exe

clean_o :
	rm *.o 

clean_exe :
	rm test
