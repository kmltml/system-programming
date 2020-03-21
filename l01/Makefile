all: out/init out/task1 out/task2 out/task3 out/task4 out/task5 out/task6

out/init: out init.c
	gcc -lpthread init.c -o out/init

out/task1: out task1.c
	gcc -lpthread task1.c -o out/task1

out/task2: out task2.c
	gcc -lpthread task2.c -o out/task2

out/task3: out task3.c
	gcc -lpthread task3.c -o out/task3

out/task4: out task4.c
	gcc -lpthread task4.c -o out/task4

out/task5: out task5.c
	gcc -lpthread task5.c -o out/task5

out/task6: out task6.c
	gcc -lpthread task6.c -o out/task6

out:
	mkdir out
