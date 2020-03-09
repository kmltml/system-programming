out/init: out init.c
	gcc -lpthread init.c -o out/init

out/task1: out task1.c
	gcc -lpthread task1.c -o out/task1

out/task2: out task2.c
	gcc -lpthread task2.c -o out/task2

out:
	mkdir out

all: out/init out/task1 out/task2
