OBJS = \
	main.o

BIN = \
	test

CFLAGS = \
	-I../src \
	-Wno-unused-parameter \
	-Wall \
	-Wextra \
	-fPIC \
	-O3

LIBS = \
	../bin/linux/x86_64/libmatoya.a \
	-lpthread \
	-lm \
	-ldl

all: clean clear $(OBJS)
	$(CC) -o $(BIN) $(OBJS) $(LIBS)
	@./test

clean:
	-rm -f $(BIN)
	-rm -f *.o

clear:
	clear
