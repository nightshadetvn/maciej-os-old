TARGETS = kasm.o kc.o kernel

main:
	nasm -f elf32 kernel.asm -o kasm.o
	gcc -fno-stack-protector -m32 -c kernel.c -o bin/kc.o
	ld -m elf_i386 -T linker.ld -o kernel.elf kasm.o kc.o
	rm kasm.o kc.o

run: main
	qemu-system-i386 -kernel kernel.elf

clean:
	rm kasm.o kc.o
