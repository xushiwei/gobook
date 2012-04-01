all:
	g++ fiber_test.cpp -o fiber_test.exe
	g++ goroutine_test.cpp -o goroutine_test.exe
	g++ server_test.cpp -o server_test.exe -lws2_32 -lmswsock
	g++ client_test.cpp -o client_test.exe -lws2_32 -lmswsock

install: all

clean:
	rm *.exe
