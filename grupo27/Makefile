

default: server client

clean:
	rm -f fifo_* bin/orchestrator bin/client output/*

client: client.c
	gcc client.c -o bin/client

server: orchestrator.c
	gcc orchestrator.c -o bin/orchestrator

run: run_client_u run_client_p

run_client_u:
	./bin/client execute 100 -u "cp teste.txt teste2.txt"

run_client_p:
	./bin/client execute 100 -p "cat teste.txt | grep "palavra" | wc -l"

run_server:
	./bin/orchestrator "./output" 4
