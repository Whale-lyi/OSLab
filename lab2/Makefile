main:
	nasm -f elf32 my_print.asm
	g++ -m32 -std=c++11 main.cpp my_print.o -o main
.PHONY:clean
clean:
	-rm -f *.o
	-rm -f main 
