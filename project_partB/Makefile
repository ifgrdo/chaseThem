server:
	make -f makes/server.mk

client:
	make -f makes/client.mk



run_server:
	./server 

run_client: 
	./client 



clean_server:
	rm chase_server/server || true
	rm -rf chase_server/*.dSYM || true

clean_client:
	rm chase_client/client || true
	rm -rf chase_client/*.dSYM || true



clean:
	rm server_exec || true
	rm -rf *.dSYM || true
	rm client_exec || true
	rm -rf *.dSYM || true
	