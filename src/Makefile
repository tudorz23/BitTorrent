CC = mpic++
CFLAGS = -Wall -g

TARGETS = tema2

build: $(TARGETS)

client.o: Client.cpp
	$(CC) -c $(CFLAGS) Client.cpp -o client.o -lpthread

tracker.o: Tracker.cpp
	$(CC) -c $(CFLAGS) Tracker.cpp -o tracker.o -lpthread

main.o: main.cpp
	$(CC) -c $(CFLAGS) main.cpp -o main.o -lpthread

tema2: main.o client.o tracker.o
	$(CC) $(CFLAGS) main.o client.o tracker.o -o tema2 -lpthread

clean:
	rm -rf *.o $(TARGETS)

.PHONY: build clean pack
