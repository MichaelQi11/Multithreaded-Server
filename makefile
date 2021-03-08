all:
	gcc -pthread -o main1 server_one_rwlock.c -std=gnu99
	gcc -pthread -o main2 server_rwlock_array.c -std=gnu99
	gcc -pthread -o main3 server_one_mutex.c -std=gnu99
	gcc -pthread -o main4 server_mutex_array.c -std=gnu99
	gcc -pthread -o client client.c -std=gnu99
	gcc -pthread -o attacker attacker.c -lm -std=gnu99
	
main1: server_one_rwlock.c
	gcc -pthread -o main1 server_one_rwlock.c -std=gnu99
main2: server_rwlock_array.c
	gcc -pthread -o main2 server_rwlock_array.c -std=gnu99
main3: server_one_mutex.c
	gcc -pthread -o main3 server_one_mutex.c -std=gnu99
main4: server_mutex_array.c
	gcc -pthread -o main4 server_mutex_array.c -std=gnu99
client: client.c common.h
	gcc -pthread -o client client.c -std=gnu99
attacker: attacker.c common.h
	gcc -pthread -o attacker attacker.c -lm -std=gnu99
clean:
	rm -f *.o
	rm -f main1
	rm -f main2
	rm -f main3
	rm -f main4
	rm -f client
	rm -f attacker