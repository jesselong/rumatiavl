CC		= gcc
CFLAGS		= -Wall -Wextra -pedantic
CFLAGS_TEST	= -g
OBJECTS		= avl.o
STATIC_LIB	= librumatiavl.a

all: $(STATIC_LIB)

clean:
	rm -f *.o *.a avltest

test:
	$(CC) -g $(CFLAGS) $(CFLAGS_TEST) -o avltest avltest.c
	./avltest

$(STATIC_LIB): $(OBJECTS)
	ar crs $(STATIC_LIB) $(OBJECTS)
