Below are the use cases of the software:
1. Server: To start the HTTP server, we perform below steps in project directory:
	$make
	$pwd
	$export LD_LIBRARY_PATH=<O/P of pwd>/lib
	$make server
2. connoverflow: Make multiple client requests to the server, where number of requests is greater
than the max_connection_threshold value perform below steps in project directory:
	$make
	$pwd
	$export LD_LIBRARY_PATH=<O/P of pwd>/lib
	$make connoverflow
3. client1: To send a HTTP1.0 request from client, perform below steps in project directory:
	$make
	$pwd
	$export LD_LIBRARY_PATH=<O/P of pwd>/lib
	$make client1
4. client1_1: To send a HTTP1.1 request from client, perform below steps in project directory:
	$make
	$pwd
	$export LD_LIBRARY_PATH=<O/P of pwd>/ lib
	$make client1_1
5. plot: To view the transmission Time comparison graph of various size of data on both HTTP
1.0 and HTTP 1.1 connections. , perform below steps in project directory:
	$make
	$make plot
