OBJS = prodcons.o 

all: prodcons

CC = gcc
FLAGS = -c

gsp: $(OBJS)
	$(CC) -o prodcons $(OBJS)

prodcons.o: prodcons.c
	$(CC) $(FLAGS) prodcons.c

clean:
	rm -f prodcons $(OBJS)
