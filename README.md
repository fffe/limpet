# limpet - an elf injection tool

limpet is a tool for injecting arbitrary (subject to size constraints) payloads into ELF binaries.

It supports 32 and 64 bit ELFs, static and dynamic, PIE and normal binaries.

## what doesn't it do?

- support architectures other than x86/x64 (yet)
- support injection methods other than segment padding (yet)
- support execution hooking methods other than hijacking the ELF entry point (yet)

## usage

```
$ make
gcc -c -o src/limpet.o src/limpet.c -fPIC -Wall
gcc -c -o src/elftoolkit.o src/elftoolkit.c -fPIC -Wall
gcc -c -o src/payload_prefix_intel.o src/payload_prefix_intel.s -fPIC -Wall
gcc -o limpet src/limpet.o src/elftoolkit.o src/payload_prefix_intel.o -fPIC -Wall
gcc -o payload_x86-32 src/payload_x86-32.s -nostartfiles -nostdlib -m32 -fPIC -Wall
gcc -o payload_x86-64 src/payload_x86-64.s -nostartfiles -nostdlib -m64 -fPIC -Wall
gcc -o target src/target.c -fPIC -Wall

$ ./target
hello world

$ ./limpet target payload_x86-64

$ ./target
hello world
hello
```
