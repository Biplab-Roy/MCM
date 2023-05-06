// Including Required files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

// Defining constants
#define MAX_MAT 1000
#define CLIENT_LIMIT_AT_ACCEPT 5
#define BUFFSIZE 100

// Typedef variables
typedef long VAL;

// used to store a matrix and it's info
typedef struct matrix
{
    VAL **mat;
    int row_count;
    int col_count;
} matrix;

// used to store a pair of matrix and it's info
typedef struct matrixPair
{
    matrix *mat1;
    matrix *mat2;
} matrixPair;

// used to store a list of matrix and it's info
typedef struct matrixList
{
    int n;
    matrix *list[MAX_MAT];
} matrixList;

typedef struct clientInfo
{
    matrixList *matList;
    int sockfd;
    int start;
    int end;
    int id;
} clientInfo;

// creating matrix structure and returning the address
matrix *newMatrix(int row_count, int col_count)
{
    matrix *mt = (matrix *)malloc(sizeof(matrix));
    // allocating memory for array of pointers
    VAL **mat = (VAL **)malloc(row_count * sizeof(VAL *));
    for (int i = 0; i < row_count; i++)
    {
        // each cell of array contains another pointer to an array
        mat[i] = (VAL *)malloc(col_count * sizeof(VAL));
    }
    // returning the new matrix with specified dimentions
    mt->mat = mat;
    mt->row_count = row_count;
    mt->col_count = col_count;
    return mt;
}

// creating matrixList structure and returning the address
matrixList *newMatrixList()
{
    return (matrixList *)malloc(sizeof(matrixList));
}

// creating clientinfo structure and returning the address
clientInfo *newClientInfo(matrixList *ml, int sockfd, int start, int end, int id)
{
    clientInfo *ci = (clientInfo *)malloc(sizeof(clientInfo));
    ci->matList = ml;
    ci->sockfd = sockfd;
    ci->start = start;
    ci->end = end;
    ci->id = id;
}

// creating pair of matrix
matrixPair *newmatrixPair(matrix *mat1, matrix *mat2)
{
    matrixPair *mp = (matrixPair *)malloc(sizeof(matrixPair));
    mp->mat1 = mat1;
    mp->mat2 = mat2;
    return mp;
}

// deleting a matrix memory
void freeMatrix(matrix *m)
{
    for (int i = 0; i < m->row_count; i++)
        free(m->mat[i]);
    free(m->mat);
    free(m);
}

// deleteing list of matrix
void freeMatrixList(matrixList *ml)
{
    for (int i = 0; i < ml->n; i++)
    {
        freeMatrix(ml->list[i]);
    }
}

// Prints the matrix from argument
void printMatrix(matrix *mat)
{
    for (int i = 0; i < mat->row_count; i++)
    {
        printf("\t\t");
        for (int j = 0; j < mat->col_count; j++)
        {
            printf("%ld\t", mat->mat[i][j]);
        }
        printf("\n");
    }
}

// Multiply two matrices
void *multiplyMatrix(void *__mp)
{
    matrixPair *mp = (matrixPair *)__mp;
    // extracting two matrices
    matrix *mat1 = mp->mat1;
    matrix *mat2 = mp->mat2;
    // creating resultant matrix with proper dimention 
    matrix *res = newMatrix(mat1->row_count, mat2->col_count);
    for (int i = 0; i < mat1->row_count; i++)
    {
        for (int j = 0; j < mat2->col_count; j++)
        {
            VAL tot_sum = 0;
            for (int k = 0; k < mat1->col_count; k++)
            {
                tot_sum += (mat1->mat[i][k]) * (mat2->mat[k][j]);
            }
            res->mat[i][j] = tot_sum;
        }
    }
    // exiting thread returning the multiplication result
    pthread_exit(res);
}

// Handles actual thread parallism in matrix multiplication
void *executeThread(matrixList *ml)
{
    // Thread array: each multiplication of two matrix will be done by one thread
    pthread_t t1[ml->n / 2];
    // continue till there are more than one matrices to multiply
    while (ml->n != 1)
    {
        matrixList *result = (matrixList *)malloc(sizeof(matrixList));
        int j = 0;
        // Assigned address to left if there is odd number of matrices
        matrix *left = NULL;
        // Multiplying each pair of matrix using thread
        for (int i = 0; i < ml->n; i += 2)
        {
            if (i + 1 < ml->n)
            {
                // creating pair of matrix for multiplication and sending it to thread
                matrixPair *mat_pair = newmatrixPair(ml->list[i], ml->list[i + 1]);
                pthread_create(&t1[j], NULL, multiplyMatrix, mat_pair);
                j++;
            }
            else
            {
                // Restricting the left one to be freed and storing it.
                left = ml->list[i];
                ml->n--;
            }
        }
        // waiting for all thread result
        for (int k = 0; k < j; k++)
        {
            void *res;
            pthread_join(t1[k], &res);
            result->list[result->n++] = (matrix *)res;
        }
        if (left)
        {
            result->list[result->n++] = left;
        }
        // Free the previous matrices and store new results in the same pointer
        freeMatrixList(ml);
        ml = result;
    }
    // returning the result
    return ml->list[0];
}

//Driver function for processing thread matrix multiplication
matrix *process_matrix_multiplication(matrixList *ml)
{
    return executeThread(ml);
}

// Returns minimium of two numbers
int min(VAL a, VAL b)
{
    return a < b ? a : b;
}

// Starting the server
int startServer(char *ip_addr, int PORT)
{
    // Declearing Variables
    int sockfd;
    struct sockaddr_in serv_addr;

    // Creating Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Can not create socket\n");
        exit(0);
    }

    // Specifying server address propery
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip_addr);
    serv_addr.sin_port = PORT;
    // Binding the socket
    int err = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (err < 0)
    {
        printf("Unable to bind local addresses\n");
        exit(0);
    }

    // Listening for connection
    listen(sockfd, CLIENT_LIMIT_AT_ACCEPT);

    return sockfd;
}

// Reading input (List of matrices) from file
void readInput(FILE *fp, matrixList *ml)
{
    // Total number of matrices
    int n;
    fscanf(fp, "%d", &n);
    for (int k = 0; k < n; k++)
    {
        // Number of rows and columns for each matrices
        int r, c;
        fscanf(fp, "%d %d", &r, &c);
        if (r <= 0 && c <= 0)
        {
            printf("Matrix dimension is not proper.\n");
            exit(0);
        }
        // reading each element of matrices
        matrix *mt = newMatrix(r, c);
        for (int i = 0; i < r; i++)
        {
            for (int j = 0; j < c; j++)
            {
                fscanf(fp, "%ld", &mt->mat[i][j]);
            }
        }
        ml->list[ml->n++] = mt;
    }
}

// Communicate with client
void *clientConnection(void *arg)
{
    // Declaring neccesary variables
    char buff[BUFFSIZE];
    clientInfo *ci = (clientInfo *)arg;
    matrixList *ml = ci->matList;
    int sockfd = ci->sockfd;

    // Total number of matrices will be sent to client
    int ele = ci->end - ci->start;
    printf("Serving Client[%d] with %d matrices(Index: %d - %d)\n", ci->id, ele, ci->start, ci->end - 1);
    send(sockfd, &ele, sizeof(int), 0);
    // Seding the matrices
    for (int i = ci->start; i < ci->end; i++)
    {
        matrix *mt = ml->list[i];
        int r = mt->row_count, c = mt->col_count;
        send(sockfd, &r, sizeof(r), 0);
        send(sockfd, &c, sizeof(c), 0);
        for (int i = 0; i < r; i++)
        {
            for (int j = 0; j < c; j++)
            {
                send(sockfd, &mt->mat[i][j], sizeof(VAL), 0);
            }
        }
    }

    // Waiting for client's reply
    printf("Waiting for client[%d]'s reply...\n", ci->id);
    int r, c;
    recv(sockfd, &r, sizeof(int), 0);
    recv(sockfd, &c, sizeof(int), 0);
    matrix *mt = newMatrix(r, c);
    for (int i = 0; i < r; i++)
    {
        for (int j = 0; j < c; j++)
        {
            recv(sockfd, &mt->mat[i][j], sizeof(VAL), 0);
        }
    }

    // Closing socket after getting result from client
    close(sockfd);
    printf("Client[%d] sent result matrix...\n\n", ci->id);
    pthread_exit(mt);
}

// Running the server
int runServer(char *fileName, char *ip_addr, int PORT, int nClients)
{
    // Declearing Variables
    int sockfd, newsockfd;
    int client_len, clients = 0;
    struct sockaddr_in cli_addr;
    pthread_t th[nClients];
    matrixList *results = newMatrixList();
    matrixList *ml = newMatrixList();

    // Taking inputs
    FILE *fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        printf("Invalid file name\n");
        exit(0);
    }
    readInput(fp, ml);

    // Validating Matrix dimentions
    for (int i = 1; i < ml->n; i++)
    {
        if (ml->list[i - 1]->col_count != ml->list[i]->row_count)
        {
            printf("Matrix dimension is not proper.\n");
            exit(0);
        }
    }
    // Returning if clients are more than number of matrices
    if (nClients > ml->n)
    {
        printf("Too many clients.\n");
        exit(0);
    }

    // Starting the sever
    sockfd = startServer(ip_addr, PORT);

    // Accepting the request and sending clients part of work
    // Choosing the problem size for each client
    int client_inp_size;
    if (ml->n % nClients == 0)
    {
        client_inp_size = ml->n / nClients;
    }
    else
    {
        client_inp_size = ml->n / nClients + 1;
    }
    // Clients <pc will get `client_inp_size` number of problems all other will get `client_inp_size-1` size of problem
    int pc = ml->n - nClients * client_inp_size + nClients;
    int last_index = 0;
    while (clients < nClients)
    {
        printf("Waiting for client to connect...\n\n");
        client_len = sizeof(cli_addr);
        // Accept the connection
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &client_len);
        if (newsockfd < 0)
        {
            printf("Acceptance Error\n");
            exit(0);
        }

        // Building structure for thread
        clientInfo *ci = NULL;
        if (clients < pc)
        {
            ci = newClientInfo(ml, newsockfd, last_index, last_index + client_inp_size, clients);
            last_index += client_inp_size;
        }
        else
        {
            ci = newClientInfo(ml, newsockfd, last_index, last_index + client_inp_size - 1, clients);
            last_index += client_inp_size - 1;
        }
        // Creating the thread for communicating with the client
        pthread_create(&th[clients], NULL, clientConnection, ci);
        clients++;
    }
    // Gather all answers from clients after finishing their execution
    for (int i = 0; i < nClients; i++)
    {
        void *ret;
        pthread_join(th[i], &ret);
        results->list[results->n++] = (matrix *)ret;
    }
    // Calculating and printing final result
    printf("All client served their work.\n");
    printf("Final matrix multiplication in under progress...\n\n");
    matrix *mt = process_matrix_multiplication(results);
    printf("Resultant matrix is:\n");
    printMatrix(mt);
}

// client's side communication with server
void processCommunications(int sockfd)
{

    // Getting matrix List input from server
    matrixList *ml = newMatrixList();
    printf("Waiting for server's message...\n");
    int n;
    recv(sockfd, &n, sizeof(n), 0);
    ml->n = n;
    for (int i = 0; i < n; i++)
    {
        int r, c;
        recv(sockfd, &r, sizeof(r), 0);
        recv(sockfd, &c, sizeof(c), 0);
        ml->list[i] = newMatrix(r, c);
        matrix *mt = ml->list[i];
        // Receiving the ith matrix
        for (int i = 0; i < r; i++)
        {
            for (int j = 0; j < c; j++)
            {
                recv(sockfd, &mt->mat[i][j], sizeof(VAL), 0);
            }
        }
    }

    // Processing the input from server
    printf("Got matrix List of size %d\n", n);
    for (int i = 0; i < n; i++)
    {
        printf("%d %d\n", ml->list[i]->row_count, ml->list[i]->col_count);
    }
    printf("Matrix multipliaction is under progress...\n");
    matrix *mt = process_matrix_multiplication(ml);

    // Sending back the result to server
    printf("Sending multiplication result to server...\n");
    send(sockfd, &mt->row_count, sizeof(int), 0);
    send(sockfd, &mt->col_count, sizeof(int), 0);
    // Sending result matrix back
    for (int i = 0; i < mt->row_count; i++)
    {
        for (int j = 0; j < mt->col_count; j++)
        {
            send(sockfd, &mt->mat[i][j], sizeof(VAL), 0);
        }
    }
    printf("Returns %d x %d\n", mt->row_count, mt->col_count);
    printf("Result has been sent. Exiting\n");
}

// Function to run the client
int runClient(char *ip_addr, int PORT)
{
    // Declearing Variables
    int sockfd, newsockfd;
    int client_len;
    struct sockaddr_in cli_addr, serv_addr;
    int i;
    char buff[BUFFSIZE];

    // Creating Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("Can not create socket\n");
        exit(0);
    }

    // Defining properties of connection
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip_addr);
    serv_addr.sin_port = PORT;

    // Connecting to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Unable to connect to server\n");
        exit(0);
    }

    // Start communication with the server and compute results
    processCommunications(sockfd);

    // Closing socket the exiting the client
    close(sockfd);
}

// Driver Code
int main(int argc, char *argv[])
{
    // Default values of arguments
    char *ip_addr = "127.0.0.1";
    int PORT = 8000;
    int nClients = 2;
    char *fileName = "inp.in";
    int server = 0;
    int client = 0;
    int help = 0;

    // Storing the arguments in required variables
    for (int i = 1; i < argc; i++)
    {
        // -s/--server denotes the program should run as a server
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--server") == 0)
        {
            server = 1;
        }
        // -c/--client denotes the program should run as a client
        else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--client") == 0)
        {
            client = 1;
        }
        // -f/--filename option should specify the filename
        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filename") == 0)
        {
            fileName = argv[i + 1];
            i++;
        }
        // -i/--ip denotes the ip address server should listen or client should connect
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--ip") == 0)
        {
            ip_addr = argv[i + 1];
            i++;
        }
        // -p/--port denotes the port number of the application
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0)
        {
            PORT = atoi(argv[i + 1]);
            i++;
        }
        // -n/--nClients denotes the number of clients that will serve the server
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nClients") == 0)
        {
            nClients = atoi(argv[i + 1]);
            i++;
        }
        // help: detailed overview of arguments
        else if (strcmp(argv[i], "help") == 0)
        {
            help = 1;
            break;
        }
    }
    if (help == 1)
    {
        printf("\nThis is a example program for ditributed matrix multiplication\n");
        printf("Arguments:\n");
        printf("\t-s\\--server, -c\\--client, -f\\--filename, -p\\--port, -i\\--ip, -n\\--nclients\n\n");
        printf("Uses:\n");
        printf("\t-s\\--server: Run the server\n");
        printf("\t-c\\--client: Run the client\n");
        printf("\t-f\\--filename: Specify the file name\n");
        printf("\t-p\\--port: Specify the port number\n");
        printf("\t-i\\--ip: Specify the ip address\n");
        printf("\t-n\\--nClients: Specify the number of client\n");
        printf("\n\nDefault IP: 127.0.0.1\n");
        printf("\nDefault PORT: 8000\n");
        printf("\nDefault nClients: 2\n");
        printf("\nDefault Filename: input.txt\n");
        printf("\nserver/client is mandatory to specify.\n");
    }
    // validating arguments server, clients, nClients
    else if (server == 1 && client == 1)
    {
        printf("Only server or client can run in a process, not both.\n");
        exit(0);
    }
    else if (server == 0 && client == 0)
    {
        printf("Please mention server/client. run `executable help` for help.\n");
        exit(0);
    }
    else if (server == 1 && nClients < 1)
    {
        printf("Number of clients should be 1 or more.\n");
        exit(0);
    }
    else if (server == 1)
    {
        // running the server
        runServer(fileName, ip_addr, PORT, nClients);
    }
    else if (client == 1)
    {
        // running the client
        runClient(ip_addr, PORT);
    }
}
