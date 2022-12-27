CC=gcc
AS=as
CFLAGS=-fPIC -Wall

_OBJ=limpet.o elftoolkit.o payload_prefix_intel.o
OBJ=$(patsubst %,src/%,$(_OBJ))

src/%.o: src/%.c
		$(CC) -c -o $@ $< $(CFLAGS)

src/%.o: src/%.s
		$(CC) -c -o $@ $< $(CFLAGS)

all: limpet payload_x86-32 payload_x86-64 target

limpet: $(OBJ)
		$(CC) -o $@ $^ $(CFLAGS)

target: src/target.c
		$(CC) -o $@ $^ $(CFLAGS)

payload_x86-32: src/payload_x86-32.s
		$(CC) -o $@ $^ -nostartfiles -nostdlib -m32 $(CFLAGS)

payload_x86-64: src/payload_x86-64.s
		$(CC) -o $@ $^ -nostartfiles -nostdlib -m64 $(CFLAGS)
 
.PHONY: clean

clean:
		rm -f src/*.o limpet target payload_x86-32 payload_x86-64 
