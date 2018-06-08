
for ASM:

32 bit
nasm -f elf hello.asm

64 bit
nasm -f elf64 hello.asm


for GAS

as -o hello.o hello.S


for executable

ld -s -o hello hello.o

