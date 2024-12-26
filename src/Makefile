CC = mpic++
CFLAGS = -Wall -g

TARGETS = tema2

build: $(TARGETS)

client.o: Client.cpp
	$(CC) -c $(CFLAGS) Client.cpp -o client.o -lpthread

tracker.o: Tracker.cpp
	$(CC) -c $(CFLAGS) Tracker.cpp -o tracker.o

helper_objects.o: helper_objects.cpp
	$(CC) -c $(CFLAGS) helper_objects.cpp -o helper_objects.o

main.o: main.cpp
	$(CC) -c $(CFLAGS) main.cpp -o main.o

tema2: main.o client.o tracker.o helper_objects.o
	$(CC) $(CFLAGS) main.o client.o tracker.o helper_objects.o -o tema2

clean:
	rm -rf *.o $(TARGETS)

pack:
	zip -FSr 333CA_ZahariaMarius-Tudor_Tema2.zip Makefile *.cpp *.h README.md

.PHONY: build clean pack
