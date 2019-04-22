main:	main.o
	g++ -lpthread -lncurses main.o -o main

main.o: main.cpp
	g++ -c main.cpp

clean:
	rm *.o
