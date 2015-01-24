cc = gcc
edit: server.o client.o
	cc -o server server.o -lpthread
	cc -o client client.o -lpthread
server.o: server.c file_ops.h
		cc -c server.c

client.o: client.c file_ops.h
		cc -c client.c

clean:
		rm client server new_server.o new_client.o
