/*
 * "PBX" server module.
 * Manages interaction with a client telephone unit (TU).
 */
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>

#include "debug.h"
#include "pbx.h"
#include "server.h"
// #include "pdata.h"

#include "signal.h"



int getInput(int connfd, FILE* stream);


/*
 * Thread function for the thread that handles interaction with a client TU.
 * This is called after a network connection has been made via the main server
 * thread and a new thread has been created to handle the connection.
 */
#if 1
void *pbx_client_service(void *arg) {
    debug("threa in");
    int connfd = *((int*)arg);
    free(arg);

    TU* tu = tu_init(connfd);
    pbx_register(pbx, tu, connfd);

    char* inputs = NULL;
    size_t len;
    FILE *stream = NULL;

    while(1)
    {
        stream = open_memstream(&inputs, &len);
        if(getInput(connfd, stream) == -1){return NULL;}
        fflush(stream);

        debug("input: %s", inputs);
        if(strcmp(inputs, "pickup") == 0)
        {
            tu_pickup(tu);
        }

        else if(strcmp(inputs, "hangup") == 0)
        {
            tu_hangup(tu);
        }
        else if(strncmp(inputs, "dial ", 5) == 0 && inputs[5] >= '0' && inputs[5] <= '9')
        {
            int i = atoi(inputs+5);
            if(i != 0)
            {
                debug("in_dial: %d", i);
                if(pbx_dial(pbx,tu,i) == -1) {
                    debug("err");
                }
            }
        }
        else if(strcmp(inputs, "chat") == 0)
        {
            tu_chat(tu, "");
        }
        else if(strncmp(inputs, "chat ", 5) == 0)
        {
            tu_chat(tu, inputs+5);
        }
    }
}
#endif

int getInput(int connfd, FILE* stream)
{
    debug("get input in");
    int i = 0;
    while(1)
    {
        char tempbuf;
        int r = read(connfd, &tempbuf, 1);
        // debug("%d", tempbuf);
        // debug("r = %d", r);
        // debug("c = %c", tempbuf);
        if(r == -1)
        {
            debug("ERR");
            return -1;
        }
        if(r == 0)
        {
            close(connfd);
            debug("%d, EOF", connfd);
            return -1;
        }
        if(tempbuf == (int)*("\r"))
        {
            debug("\\r");
            continue;
        }
        if(tempbuf == (int)*("\n"))
        {
            debug("\\n");
            break;
        }
        if(i >= 1000)
        {
            debug("exceed i");
            break;
        }
        i++;
        fprintf(stream, "%c", tempbuf);
    }
    return 0;
}
