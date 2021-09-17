#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <list>
#include <iostream>
#include "../commons/Packet.h"

/* Server port  */
#define PORT 4242

/* Buffer length */
#define BUFFER_LENGTH 4096

using namespace std;

void *func(void *_clientfd)
{
    int clientfd = *((int *)_clientfd);
    char buffer[BUFFER_LENGTH];

    Packet *hello = new Packet(1, "Hello client!");

    /* Sends the message to the client */
    if (send(clientfd, hello->toBytes(), PACKET_BUFFER_LEN, 0))
    {
        fprintf(stdout, "Client connected.\nWaiting for client message ...\n");

        Packet *packet;

        /* Communicates with the client until bye message come */
        do
        {
            /* Zeroing buffers */
            memset(buffer, 0x0, BUFFER_LENGTH);

            /* Receives client message */
            int message_len;
            if ((message_len = recv(clientfd, buffer, BUFFER_LENGTH, 0)) > 0)
            {
                packet = new Packet();
                packet->fromBytes(buffer);
                //buffer[message_len - 1] = '\0';
                cout << "Client says: " << packet->getPayload() << endl;
            }

            Packet *yep = new Packet(1, "Yep!");
            send(clientfd, yep->toBytes(), PACKET_BUFFER_LEN, 0);

        } while (true);
    }

    /* Client connection Close */
    close(clientfd);
}

/*
 * Main execution of the server program of the simple protocol
 */
int main(void)
{
    list<pthread_t *> *threads = new list<pthread_t *>();

    /* Client and Server socket structures */
    struct sockaddr_in client, server;

    /* File descriptors of client and server */
    int serverfd, clientfd;

    char buffer[BUFFER_LENGTH];

    fprintf(stdout, "Starting server\n");

    /* Creates a IPv4 socket */
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd == -1)
    {
        perror("Can't create the server socket:");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "Server socket created with fd: %d\n", serverfd);

    /* Defines the server socket properties */
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    memset(server.sin_zero, 0x0, 8);

    /* Handle the error of the port already in use */
    int yes = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR,
                   &yes, sizeof(int)) == -1)
    {
        perror("Socket options error:");
        return EXIT_FAILURE;
    }

    /* bind the socket to a port */
    if (bind(serverfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("Socket bind error:");
        return EXIT_FAILURE;
    }

    /* Starts to wait connections from clients */
    if (listen(serverfd, 1) == -1)
    {
        perror("Listen error:");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "Listening on port %d\n", PORT);

    while (true)
    {

        socklen_t client_len = sizeof(client);
        if ((clientfd = accept(serverfd,
                               (struct sockaddr *)&client, &client_len)) == -1)
        {
            perror("Accept error:");
            return EXIT_FAILURE;
        }

        pthread_t *th = new pthread_t();
        threads->push_back(th);

        if (pthread_create(th, NULL, func, &clientfd) != 0)
        {
            perror("pthread_create error:");
            return EXIT_FAILURE;
        }
    }

    // for (int i = 0; i < threads->size(); i++)
    // {
    //     pthread_join(*(threads[i]), NULL);
    // }

    /* Close the local socket */
    close(serverfd);

    printf("Connection closed\n\n");

    return EXIT_SUCCESS;
}
