/*
 * TU: simulates a "telephone unit", which interfaces a client with the PBX.
 */
#include <stdlib.h>
#include <semaphore.h>

#include "pbx.h"
#include "debug.h"
#include "server.h"
#include "csapp.h"


void tu_change_state(TU *tu, TU_STATE state, int hold);
int tu_send_message(TU *tu, TU_STATE state);
int tryDial(TU *tu, TU *target);
int tryHangup_call(TU *tu);
int tu_send_chat(TU *tu, char *msg);

typedef struct tu{
    int connfd;
    int extension;
    int reference;
    struct tu* peer;
    TU_STATE current_state;
    sem_t semaphore;
}TU;


/*
 * Initialize a TU
 *
 * @param fd  The file descriptor of the underlying network connection.
 * @return  The TU, newly initialized and in the TU_ON_HOOK state, if initialization
 * was successful, otherwise NULL.
 */
#if 1
TU *tu_init(int fd) {
    if(fd < 0) { return NULL;}

    TU *_tu = malloc(sizeof(TU));
    _tu->connfd = fd;
    _tu->extension = fd;
    _tu->reference = 1;
    _tu->peer = NULL;
    _tu->current_state = TU_ON_HOOK;

    if(sem_init(&_tu->semaphore, 0, 1) == -1)
    {
        return NULL;
    }
    return _tu;
}
#endif

/*
 * Increment the reference count on a TU.
 *
 * @param tu  The TU whose reference count is to be incremented
 * @param reason  A string describing the reason why the count is being incremented
 * (for debugging purposes).
 */
#if 1
void tu_ref(TU *tu, char *reason) {
    if(tu == NULL) {debug("ERR"); return;}
    sem_wait(&tu->semaphore);
    debug("ref++ : %s", reason);
    tu->reference += 1;
    sem_post(&tu->semaphore);
}
#endif

/*
 * Decrement the reference count on a TU, freeing it if the count becomes 0.
 *
 * @param tu  The TU whose reference count is to be decremented
 * @param reason  A string describing the reason why the count is being decremented
 * (for debugging purposes).
 */
#if 1
void tu_unref(TU *tu, char *reason) {
    if(tu == NULL) {debug("ERR"); return;}
    debug("ref-- : %s", reason);
    sem_wait(&tu->semaphore);
    tu->reference -= 1;
    sem_post(&tu->semaphore);
    debug("ref: %d", tu->reference);
    if(tu->reference == 0)
    {
        free(tu);
    }
}
#endif

/*
 * Get the file descriptor for the network connection underlying a TU.
 * This file descriptor should only be used by a server to read input from
 * the connection.  Output to the connection must only be performed within
 * the PBX functions.
 *
 * @param tu
 * @return the underlying file descriptor, if any, otherwise -1.
 */
#if 1
int tu_fileno(TU *tu) {
    if(tu == NULL) {debug("ERR"); return -1;}
    return tu->connfd;
}
#endif

/*
 * Get the extension number for a TU.
 * This extension number is assigned by the PBX when a TU is registered
 * and it is used to identify a particular TU in calls to tu_dial().
 * The value returned might be the same as the value returned by tu_fileno(),
 * but is not necessarily so.
 *
 * @param tu
 * @return the extension number, if any, otherwise -1.
 */
#if 1
int tu_extension(TU *tu) {
    if(tu == NULL) {debug("ERR"); return -1;}
    return tu->extension;
}
#endif

/*
 * Set the extension number for a TU.
 * A notification is set to the client of the TU.
 * This function should be called at most once one any particular TU.
 *
 * @param tu  The TU whose extension is being set.
 */
#if 1
int tu_set_extension(TU *tu, int ext) {
    if(tu == NULL || ext < 0) {debug("ERR"); return -1;}
    sem_wait(&tu->semaphore);
    tu->extension = ext;
    sem_post(&tu->semaphore);
    tu_send_message(tu, TU_ON_HOOK);
    return 1;
}
#endif

/*
 * Initiate a call from a specified originating TU to a specified target TU.
 *   If the originating TU is not in the TU_DIAL_TONE state, then there is no effect.
 *   If the target TU is the same as the originating TU, then the TU transitions
 *     to the TU_BUSY_SIGNAL state.
 *   If the target TU already has a peer, or the target TU is not in the TU_ON_HOOK
 *     state, then the originating TU transitions to the TU_BUSY_SIGNAL state.
 *   Otherwise, the originating TU and the target TU are recorded as peers of each other
 *     (this causes the reference count of each of them to be incremented),
 *     the target TU transitions to the TU_RINGING state, and the originating TU
 *     transitions to the TU_RING_BACK state.
 *
 * In all cases, a notification of the resulting state of the originating TU is sent to
 * to the associated network client.  If the target TU has changed state, then its client
 * is also notified of its new state.
 *
 * If the caller of this function was unable to determine a target TU to be called,
 * it will pass NULL as the target TU.  In this case, the originating TU will transition
 * to the TU_ERROR state if it was in the TU_DIAL_TONE state, and there will be no
 * effect otherwise.  This situation is handled here, rather than in the caller,
 * because here we have knowledge of the current TU state and we do not want to introduce
 * the possibility of transitions to a TU_ERROR state from arbitrary other states,
 * especially in states where there could be a peer TU that would have to be dealt with.
 *
 * @param tu  The originating TU.
 * @param target  The target TU, or NULL if the caller of this function was unable to
 * identify a TU to be dialed.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_dial(TU *tu, TU *target) {
    debug("in tu_dial");
    if(tu == NULL) {return -1;}
    if(tu->current_state != TU_DIAL_TONE) { return 0; }

    if(target == NULL)
    {
        debug("err: in tu_dial");
        tu_change_state(tu, TU_ERROR, 1);
        tu_send_message(tu, TU_ERROR);
        return -1;
    }
    if(tu->current_state == TU_RING_BACK)
    {
        tu_send_message(tu, TU_RING_BACK);
        return 0;
    }
    if( tu->extension == target->extension   ||
        target->extension == -1        ||
        target->current_state != TU_ON_HOOK)
    {
        debug("busy");
        tu_change_state(tu, TU_BUSY_SIGNAL, 1);
        tu_send_message(tu, TU_BUSY_SIGNAL);
        return 0;
    }
    else
    {
        if(tryDial(tu, target) == -1) { return -1; }
        return 0;
    }

}
#endif

int tryDial(TU *tu, TU *target)
{
    debug("in trydial");
    if(tu == NULL || target == NULL) { return -1; }

    TU *t1, *t2;
    if(tu->extension < target->extension)
    {
        t1 = tu;
        t2 = target;
    }
    else
    {
        t1 = target;
        t2 = tu;
    }

    sem_wait(&t1->semaphore);
    sem_wait(&t2->semaphore);

        tu->peer = target;
        tu->current_state = TU_RING_BACK;
        tu->reference += 1;
        tu_send_message(tu, TU_RING_BACK);

        target->peer = tu;
        target->current_state = TU_RINGING;
        target->reference += 1;
        tu_send_message(target, TU_RINGING);

    sem_post(&t1->semaphore);
    sem_post(&t2->semaphore);

    return 0;
}


 //  Take a TU receiver off-hook (i.e. pick up the handset).
 // *   If the TU is in neither the TU_ON_HOOK state nor the TU_RINGING state,
 // *     then there is no effect.
 // *   If the TU is in the TU_ON_HOOK state, it goes to the TU_DIAL_TONE state.
 // *   If the TU was in the TU_RINGING state, it goes to the TU_CONNECTED state,
 // *     reflecting an answered call.  In this case, the calling TU simultaneously
 // *     also transitions to the TU_CONNECTED state.
 // *
 // * In all cases, a notification of the resulting state of the specified TU is sent to
 // * to the associated network client.  If a peer TU has changed state, then its client
 // * is also notified of its new state.
 // *
 // * @param tu  The TU that is to be picked up.
 // * @return 0 if successful, -1 if any error occurs that results in the originating
 // * TU transitioning to the TU_ERROR state. 
 

#if 1
int tu_pickup(TU *tu) {
    debug("tu tu_pickup");
    //req: on hook -> dial tone, ringing->connected
    if(tu == NULL) {return -1;}

    if(tu->current_state == TU_ON_HOOK)
    {
        tu_change_state(tu, TU_DIAL_TONE, 1);
        tu_send_message(tu, TU_DIAL_TONE);
        return 0;
    }
    if(tu->current_state == TU_RINGING)
    {
        tu_change_state(tu, TU_CONNECTED, 1);
        tu_send_message(tu, TU_CONNECTED);

        tu_change_state(tu->peer, TU_CONNECTED, 1);
        tu_send_message(tu->peer, TU_CONNECTED);
        return 0;
    }
    if(tu->current_state == TU_DIAL_TONE)
    {
        tu_send_message(tu, TU_DIAL_TONE);
    }
    else
    {
        tu_change_state(tu, TU_ERROR, 1);
        tu_send_message(tu, TU_ERROR);
    }
    return 0;
}
#endif

/*
 * Hang up a TU (i.e. replace the handset on the switchhook).
 *
 *   If the TU is in the TU_CONNECTED or TU_RINGING state, then it goes to the
 *     TU_ON_HOOK state.  In addition, in this case the peer TU (the one to which
 *     the call is currently connected) simultaneously transitions to the TU_DIAL_TONE
 *     state.
 *   If the TU was in the TU_RING_BACK state, then it goes to the TU_ON_HOOK state.
 *     In addition, in this case the calling TU (which is in the TU_RINGING state)
 *     simultaneously transitions to the TU_ON_HOOK state.
 *   If the TU was in the TU_DIAL_TONE, TU_BUSY_SIGNAL, or TU_ERROR state,
 *     then it goes to the TU_ON_HOOK state.
 *
 * In all cases, a notification of the resulting state of the specified TU is sent to
 * to the associated network client.  If a peer TU has changed state, then its client
 * is also notified of its new state.
 *
 * @param tu  The tu that is to be hung up.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
#if 1
int tu_hangup(TU *tu) {
    debug("tu tu_hangup");
    if(tu == NULL) {return -1;}
    if(tu->current_state == TU_ON_HOOK)
    {
        tu_send_message(tu, TU_ON_HOOK);
        return 0;
    }
    if(tu->current_state == TU_CONNECTED ||
        tu->current_state == TU_RINGING  ||
        tu->current_state == TU_RING_BACK)
    {
        tryHangup_call(tu);
        return 0;
    }

    if(tu->current_state == TU_DIAL_TONE ||
        tu->current_state == TU_BUSY_SIGNAL ||
        tu->current_state == TU_ERROR)
    {
        tu_change_state(tu, TU_ON_HOOK, 1);
        tu_send_message(tu, TU_ON_HOOK);
        return 0;
    }
    return -1;
}
#endif

int tryHangup_call(TU *tu)
{
    debug("tu tryHangup_call");
    if(tu == NULL) { return -1; }

    TU *t1, *t2;
    if(tu->extension < tu->peer->extension)
    {
        t1 = tu;
        t2 = tu->peer;
    }
    else
    {
        t1 = tu->peer;
        t2 = tu;
    }

    sem_wait(&t1->semaphore);
    sem_wait(&t2->semaphore);
        if(tu->current_state == TU_CONNECTED || tu->current_state == TU_RINGING)
        {
            tu_change_state(tu, TU_ON_HOOK, 0);
            tu_send_message(tu, TU_ON_HOOK);

            tu_change_state(tu->peer, TU_DIAL_TONE, 0);
            tu_send_message(tu->peer, TU_DIAL_TONE);
        }
        else if(tu->current_state == TU_RING_BACK)
        {
            tu_change_state(tu, TU_ON_HOOK, 0);
            tu_send_message(tu, TU_ON_HOOK);

            tu_change_state(tu->peer, TU_ON_HOOK, 0);
            tu_send_message(tu->peer, TU_ON_HOOK);
        }

        tu->reference -= 1;
        tu->peer->reference -= 1;

        tu->peer->peer = NULL;
        tu->peer = NULL;
    sem_post(&t1->semaphore);
    sem_post(&t2->semaphore);

    return 0;
}

/*
 * "Chat" over a connection.
 *
 * If the state of the TU is not TU_CONNECTED, then nothing is sent and -1 is returned.
 * Otherwise, the specified message is sent via the network connection to the peer TU.
 * In all cases, the states of the TUs are left unchanged and a notification containing
 * the current state is sent to the TU sending the chat.
 *
 * @param tu  The tu sending the chat.
 * @param msg  The message to be sent.
 * @return 0  If the chat was successfully sent, -1 if there is no call in progress
 * or some other error occurs.
 */
#if 1
int tu_chat(TU *tu, char *msg) {
    debug("tu tu_chat");
    if(tu == NULL) {return -1;}
    if(tu->current_state != TU_CONNECTED)
    {
        tu_send_message(tu, tu->current_state);
        return -1;
    }
    if(tu->peer == NULL) {return -1;}


    if(tu_send_message(tu, TU_ON_HOOK) == -1) return -1;
    if(tu_send_chat(tu->peer, msg) == -1) return -1;
    return 0;

}
#endif


void tu_change_state(TU *tu, TU_STATE state, int hold)
{
    if(hold == 1)
    {
        sem_wait(&tu->semaphore);
    }

    tu->current_state = state;

    if(hold == 1)
    {
        sem_post(&tu->semaphore);
    }
}

int tu_send_chat(TU *tu, char *msg)
{
    int fd = tu->connfd;
    int i = 0;

    if(rio_writen(fd, "CHAT ", 5) == -1) {return -1;}
    while(1)
    {
        if(*(msg+i) == '\0')
        {
            if(rio_writen(fd, "\n", 1) == -1) {return -1;}
            debug("chat msg end");
            break;
        }
        if(rio_writen(fd, msg+i, 1) == -1) {return -1;}
        i++;
    }
    return 1;
}

int tu_send_message(TU *tu, TU_STATE state)
{
    if(tu == NULL) {return -1;}
    int fd = tu->connfd;
    int ext = tu->extension;
    char *msg = malloc(15 * sizeof(char));
    if(state == TU_ON_HOOK)
    {
        sprintf(msg, "ON HOOK %d\n", ext);
        debug("%s",msg);
    }
    else if(state == TU_RINGING)
    {
        sprintf(msg, "RINGING\n");
        debug("%s",msg);
    }
    else if(state == TU_DIAL_TONE)
    {
        sprintf(msg, "DIAL TONE\n");
        debug("%s",msg);
    }
    else if(state == TU_RING_BACK)
    {
        sprintf(msg, "RING BACK\n");
        debug("%s",msg);
    }
    else if(state == TU_CONNECTED)
    {
        sprintf(msg, "CONNECTED %d\n", ext);
        debug("%s",msg);
    }
    else if(state == TU_ERROR)
    {
        sprintf(msg, "ERROR\n");
        debug("%s",msg);
    }
    else if(state == TU_BUSY_SIGNAL)
    {
        sprintf(msg, "BUSY SIGNAL\n");
        debug("%s",msg);
    }
    else
    {
        debug("notiong");
        return 0;
    }
    // if(rio_writen(fd, msg, 15) == -1) {return -1;}
    // if(rio_writen(fd, "\n", 1) == -1) {return -1;}

    send(fd, msg, strlen(msg), 0);
    free(msg);
    return 0;
}