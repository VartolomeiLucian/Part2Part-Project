all:
	g++ CentralServer.cpp -o server
	g++ Client.cpp -o client
clean:
	rm -f *~server client

