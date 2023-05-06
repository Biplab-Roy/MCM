# Matrix Chain Multiplication using client-server architecture
Matrix chain multiplication is the well known problem. This program aims to find the multiplication result instead of finding best order for multiplication. Matrix Multiplication follows associative property. This allows us to have multithreading in Matrix Chain Multiplication. We can go one step ahead to solve this problem is to use a server client architecture. 
## Server ## 
The main idea is to a server having `m` merices to multiply also server knows some `n` number of clients will connect to the server to solve this problem. Server evenly divides the multiplication task into `n` clients. Server sends the metrices to the client using a connection oriented protocol `SOCK_STREAM`. Client solves the sub-problem  of MCM and returns the result. Server recollects the result of all the clients then starts multiplying the results. For example consider the following mertices:
$$ A_1 A_2 A_3 ... A_m $$ 
is sent to m clients, so the results returned bu clients will be:
$$ C_1 C_2 ... C_n$$
Server multiplies the results using multi-threading.

## Client ## 
Client gets a subproblem of size `k`. Client again uses multi-threading to solve this problem so, `logk` numbers of time multiplications starts, at $ i^{th} $ step $ \frac{k}{2^i} $ number of threads started. This reduction steps go on until 1 matrix left, which is the result of multiplication and sent back to server.

## Experiment ##
To compile the program in linux enviroment use `gcc MCM.c -o mcm`. The generated executable can be run as both server and client. To run the executable as server one can use `-s` option and to run it as a client use `-c` option. Other options like port/file can be specified using command line options. 
```shell
./mcm -c/-s -f/--filename <filename> -i/--ip <ip_address> -p/--port <port_no> -n/-nClients <number_of_clients>
```
The specified file contains merices to be multiplied. Example file is uploaded [here](./input.txt). All these options are not mandatory. Here are the default values of these options.
```text
Arguments:
        -s\--server, -c\--client, -f\--filename, -p\--port, -i\--ip, -n\--nclients

Uses:
        -s\--server: Run the server
        -c\--client: Run the client
        -f\--filename: Specify the file name
        -p\--port: Specify the port number
        -i\--ip: Specify the ip address
        -n\--nClients: Specify the number of client

Default IP: 127.0.0.1
Default PORT: 8000
Default nClients: 2
Default Filename: input.txt
```
## Future contributions ##
Currently supported two matrix multiplication program is $ O(n^3) $ program. It can be extended to `strassen matrix multiplication`, which can help to optimize this more. 