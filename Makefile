zad9: zad9.c
	gcc -std=gnu99 -Wall -Wextra zad9.c -o zad9
    
clean:
	find . -maxdepth 1 -type f | grep -v ".c\|Makefile" | xargs rm
