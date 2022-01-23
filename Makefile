CC = gcc
CFLAGS = -fPIC -Wall -Wextra -c -g
LFLAGS = -shared -lpthread

all: build

build: libscheduler.so

libscheduler.so: so_scheduler.o pq.o sch_thread.o
	$(CC) $(LFLAGS) -o $@ $^

# so_scheduler.o: so_scheduler.c
# 	$(CC) $(CFLAGS) -o $@ $^

# pq.o: pq.c
# 	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -rf *.so *~ *.o
