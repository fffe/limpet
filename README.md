# limpet - an elf injection tool

limpet is a tool for injecting arbitrary (subject to size constraints) payloads into ELF binaries.

It supports 32 and 64 bit ELFs, static and dynamic, PIE and normal binaries.

It currently supports x86, x86-64 and aarch64 targets on Linux.

## what doesn't it do?

- support injection methods other than segment padding (yet)
- support execution hooking methods other than hijacking the ELF entry point (yet)

## requirements

- clang. pass llvm-config via the LLVMCONFIG make variable
- vim (xxd)

## usage

```
$ make LLVMCONFIG=llvm-config-14 clean
rm -f src/*.o src/stubs/*.o src/stubs/*.bin src/stubs/*.h src/payloads/*.o src/payloads/*.bin limpet target

$ make LLVMCONFIG=llvm-config-14
clang -c -o src/stubs/stub_aarch64.o src/stubs/stub_aarch64.s -target aarch64-unknown-linux -integrated-as -fPIC -Wall
llvm-objcopy -O binary -j .text src/stubs/stub_aarch64.o src/stubs/stub_aarch64.bin
xxd -i src/stubs/stub_aarch64.bin | sed -re 's/src_stubs_(.+)_bin/\1/' > src/stubs/stub_aarch64.h
clang -c -o src/stubs/stub_x86.o src/stubs/stub_x86.s -target i386-unknown-linux -integrated-as -fPIC -Wall
llvm-objcopy -O binary -j .text src/stubs/stub_x86.o src/stubs/stub_x86.bin
xxd -i src/stubs/stub_x86.bin | sed -re 's/src_stubs_(.+)_bin/\1/' > src/stubs/stub_x86.h
clang -c -o src/stubs/stub_x86-64.o src/stubs/stub_x86-64.s -target x86_64-unknown-linux -integrated-as -fPIC -Wall
llvm-objcopy -O binary -j .text src/stubs/stub_x86-64.o src/stubs/stub_x86-64.bin
xxd -i src/stubs/stub_x86-64.bin | sed -re 's/src_stubs_(.+)_bin/\1/' > src/stubs/stub_x86-64.h
clang -c -o src/payloads/payload_aarch64.o src/payloads/payload_aarch64.s -target aarch64-unknown-linux -integrated-as -fPIC -Wall
llvm-objcopy -O binary -j .text src/payloads/payload_aarch64.o src/payloads/payload_aarch64.bin
clang -c -o src/payloads/payload_x86.o src/payloads/payload_x86.s -target i386-unknown-linux -integrated-as -fPIC -Wall
llvm-objcopy -O binary -j .text src/payloads/payload_x86.o src/payloads/payload_x86.bin
clang -c -o src/payloads/payload_x86-64.o src/payloads/payload_x86-64.s -target x86_64-unknown-linux -integrated-as -fPIC -Wall
llvm-objcopy -O binary -j .text src/payloads/payload_x86-64.o src/payloads/payload_x86-64.bin
clang -c -o src/limpet.o src/limpet.c -fPIC -Wall -DHAVE_AARCH64 -DHAVE_X86 -DHAVE_X86_64
clang -c -o src/elftoolkit.o src/elftoolkit.c -fPIC -Wall -DHAVE_AARCH64 -DHAVE_X86 -DHAVE_X86_64
clang -o limpet src/limpet.o src/elftoolkit.o -fPIC -Wall -DHAVE_AARCH64 -DHAVE_X86 -DHAVE_X86_64
clang -o target src/target.c -fPIC -Wall
rm src/stubs/stub_x86.bin src/stubs/stub_aarch64.bin src/stubs/stub_x86-64.bin

$ ./target
hello world

$ ./limpet target src/payloads/payload_aarch64.bin

$ ./target
hello world
hello from the payload
$
```

The fexecve payload for x64 can be used to inject an ELF into an ELF.

To use it, add the payload to the target, append the ELF to the target, and append the length as a little endian 32bit.

The example below adds /bin/ls to the target.

```
$ make LLVMCONFIG=llvm-config-14
[...]

$ ./target
hello world

$ ./limpet target src/payloads/payload_x86-64_fexecve.bin

$ cat /bin/ls >> target

$ ls -l /bin/ls
-rwxr-xr-x 1 root root 142144 Sep  5  2019 /bin/ls*

# there's probably an easier way to do this but whatever...
$ echo 142144 | perl -pe '$_=join("", map(sprintf("\\x%02x", ord($_)), split(//,pack("I", $_))))."\n"'
\x40\x2b\x02\x00

$ echo -ne "\x40\x2b\x02\x00" >> target

$ ./target
hello world
Makefile  README.md  limpet  src  target
```
