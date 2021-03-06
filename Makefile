all: zad2 zad5 zad6 zad7 zad9

zad2: zad2.c
	gcc -std=gnu99 -pthread -Wall -Wextra zad2.c -o zad2
	
zad5: zad5.c
	gcc -std=gnu99 -pthread -Wall -Wextra zad5.c -o zad5
	
zad6: zad6.c
	gcc -std=gnu99 -pthread -Wall -Wextra zad6.c -o zad6 -lrt
	
zad7: zad7.c
	gcc -std=gnu99 -Wall -Wextra zad7.c -o zad7

zad9: zad9-client zad9-server

zad9-client: zad9-client.c
	gcc -std=gnu99 -Wall -Wextra zad9-client.c -o zad9-client

zad9-server: zad9-server.c
	gcc -std=gnu99 -Wall -Wextra zad9-server.c -o zad9-server
    
clean:
	rm -f zad2 zad5 zad6 zad7 zad9-client zad9-server
