all:
	g++ -g -Wall main.cpp -lpthread -o cse4001_sync

clean:
	rm -f cse4001_sync main