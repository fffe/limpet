CC=clang
OBJCOPY=llvm-objcopy
CFLAGS=-fPIC -Wall

LLVMCONFIG := $(if $(LLVMCONFIG),$(LLVMCONFIG),llvm-config)
export PATH:=$(shell $(LLVMCONFIG) --bindir):$(PATH)
 
# There's probably a better way to do this, but... this works
HAVE_AARCH64=$(shell PATH='$(subst ','\'',$(PATH))' clang -print-targets | egrep "\baarch64\b")
HAVE_X86=$(shell PATH='$(subst ','\'',$(PATH))' clang -print-targets | egrep "\bx86\b")
HAVE_X86-64=$(shell PATH='$(subst ','\'',$(PATH))' clang -print-targets | egrep "\bx86-64\b")

ifneq ($(HAVE_AARCH64),)
	ARCH_DEFINES += -DHAVE_AARCH64
	_PAYLOADS += payload_aarch64.bin
	_STUB_HEADERS += stub_aarch64.h
endif
ifneq ($(HAVE_X86),)
	ARCH_DEFINES += -DHAVE_X86
	_PAYLOADS += payload_x86.bin
	_STUB_HEADERS += stub_x86.h
endif
ifneq ($(HAVE_X86-64),)
	ARCH_DEFINES += -DHAVE_X86_64
	_PAYLOADS += payload_x86-64.bin
	_STUB_HEADERS += stub_x86-64.h
endif
STUB_HEADERS=$(patsubst %,src/stubs/%,$(_STUB_HEADERS))
PAYLOADS=$(patsubst %,src/payloads/%,$(_PAYLOADS))

_OBJ=limpet.o elftoolkit.o
OBJ=$(patsubst %,src/%,$(_OBJ))

all: stubs payloads limpet target

limpet: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(ARCH_DEFINES)

src/%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(ARCH_DEFINES)

src/%.o: src/%.s
	$(CC) -c -o $@ $< $(CFLAGS)

target: src/target.c
	$(CC) -o $@ $^ $(CFLAGS)


# Stub headers
#
stubs: $(STUB_HEADERS)

src/stubs/%.h: src/stubs/%.bin
	xxd -i $< | sed -re 's/src_stubs_(.+)_bin/\1/' > $@

src/stubs/%.bin: src/stubs/%.o
	$(OBJCOPY) -O binary -j .text $< $@

src/stubs/stub_aarch64.o: src/stubs/stub_aarch64.s
	$(CC) -c -o $@ $< -target aarch64-unknown-linux -integrated-as $(CFLAGS)

src/stubs/stub_x86.o: src/stubs/stub_x86.s
	$(CC) -c -o $@ $< -target i386-unknown-linux -integrated-as $(CFLAGS)

src/stubs/stub_x86-64.o: src/stubs/stub_x86-64.s
	$(CC) -c -o $@ $< -target x86_64-unknown-linux -integrated-as $(CFLAGS)


# Payloads
#
payloads: $(PAYLOADS)

src/payloads/%.bin: src/payloads/%.o
	$(OBJCOPY) -O binary -j .text $< $@

src/payloads/payload_aarch64.o: src/payloads/payload_aarch64.s
	$(CC) -c -o $@ $^ -target aarch64-unknown-linux -integrated-as $(CFLAGS)

src/payloads/payload_x86.o: src/payloads/payload_x86.s
	$(CC) -c -o $@ $^ -target i386-unknown-linux -integrated-as $(CFLAGS)

src/payloads/payload_x86-64.o: src/payloads/payload_x86-64.s
	$(CC) -c -o $@ $^ -target x86_64-unknown-linux -integrated-as $(CFLAGS)


.PHONY: clean 

clean:
	rm -f src/*.o src/stubs/*.o src/stubs/*.bin src/stubs/*.h src/payloads/*.o src/payloads/*.bin limpet target
