#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#include "pbx.h"
#include "server.h"
#include "debug.h"
#include "string.h"
#include "signal.h"

#include "pdata.h"
#include "csapp.h"


static void terminate(int status);

/*
 * "PBX" telephone exchange simulation.
 *
 * Usage: pbx <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

   
    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function pbx_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    //--------------------sigaction----------------------------------

    struct sigaction sa;
    sa.sa_handler = &terminate;

    sigaction(SIGHUP, &sa, NULL);

    //---------------------------------------------------------------

     // Perform required initialization of the PBX module.
    debug("Initializing PBX...");
    pbx = pbx_init();

    int port;
    if(argc != 3){
        debug("ERR argc");
        exit(0);
    }
    else
    {
        port = atoi(argv[2]);
        if(port == 0)
        {
            debug("ERR port#");
            exit(0);
        }
    }

    //-----------------------------------
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    listenfd = open_listenfd(argv[2]);
    debug("listenfd : %d", listenfd);
    if(listenfd == -1) {exit(0);}
    while(1)
    {
        debug("while in");
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        debug("connfd : %d", connfd);
        pthread_t t;
        int *pclient = malloc(sizeof(int));
        *pclient = connfd;
        pthread_create(&t, NULL, pbx_client_service, pclient);
        // pthread_detach(t);
    }


    fprintf(stderr, "You have to finish implementing main() "
	    "before the PBX server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    debug("Shutting down PBX...");
    pbx_shutdown(pbx);
    debug("PBX server terminating");
    exit(status);
}
