/*
 * PBX: simulates a Private Branch Exchange.
 */
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

#include "pbx.h"
#include "debug.h"


typedef struct pbx
{
    TU* tulist[FD_SETSIZE];
    sem_t semaphore;
    int status;
}PBX;


/*
 * Initialize a new PBX.
 *
 * @return the newly initialized PBX, or NULL if initialization fails.
 */
#if 1
PBX *pbx_init() {

    PBX *_pbx = malloc(sizeof(PBX));

    if(sem_init(&_pbx->semaphore, 0, 1) == -1)
    {
        return NULL;
    }
    for(int i = 0; i < FD_SETSIZE; i++)
    {
        _pbx->tulist[i] = NULL;
    }
    _pbx->status = 1;
    return _pbx;
}
#endif

/*
 * Shut down a pbx, shutting down all network connections, waiting for all server
 * threads to terminate, and freeing all associated resources.
 * If there are any registered extensions, the associated network connections are
 * shut down, which will cause the server threads to terminate.
 * Once all the server threads have terminated, any remaining resources associated
 * with the PBX are freed.  The PBX object itself is freed, and should not be used again.
 *
 * @param pbx  The PBX to be shut down.
 */
#if 1
void pbx_shutdown(PBX *pbx) {
    sem_wait(&pbx->semaphore);
    for(int i = 0; i < FD_SETSIZE; i++)
    {
        if(pbx->tulist[i] != NULL)
        {
            pbx_unregister(pbx, pbx->tulist[i]);
        }
    }
    sem_post(&pbx->semaphore);
    pthread_exit(0);
}
#endif


 // * Register a telephone unit with a PBX at a specified extension number.
 // * This amounts to "plugging a telephone unit into the PBX".
 // * The TU is initialized to the TU_ON_HOOK state.
 // * The reference count of the TU is increased and the PBX retains this reference
 // *for as long as the TU remains registered.
 // * A notification of the assigned extension number is sent to the underlying network
 // * client.
 // *
 // * @param pbx  The PBX registry.
 // * @param tu  The TU to be registered.
 // * @param ext  The extension number on which the TU is to be registered.
 // * @return 0 if registration succeeds, otherwise -1.
 
#if 1
int pbx_register(PBX *pbx, TU *tu, int ext) {

    debug("pbx regi in");
    if(pbx == NULL || tu == NULL || ext >= FD_SETSIZE)
    {
        printf("no pbx or tu\n");
        return -1;
    }

    sem_wait(&pbx->semaphore);
        pbx->tulist[ext] = tu;
    sem_post(&pbx->semaphore);
    tu_ref(tu, "tu registered.");
    tu_set_extension(tu, ext);
    debug("addr:%p",tu);
    debug("ext: %d", ext);

    return 0;
}
#endif

/*
 * Unregister a TU from a PBX.
 * This amounts to "unplugging a telephone unit from the PBX".
 * The TU is disassociated from its extension number.
 * Then a hangup operation is performed on the TU to cancel any
 * call that might be in progress.
 * Finally, the reference held by the PBX to the TU is released.
 *
 * @param pbx  The PBX.
 * @param tu  The TU to be unregistered.
 * @return 0 if unregistration succeeds, otherwise -1.
 */
#if 1
int pbx_unregister(PBX *pbx, TU *tu) {
    if(pbx == NULL || tu == NULL)
    {
        printf("no pbx or tu\n");
        return -1;
    }
    int ext = tu_extension(tu);

    sem_wait(&pbx->semaphore);
    pbx->tulist[ext] = NULL;
    sem_post(&pbx->semaphore);

    tu_unref(tu, "tu unregistered.");

    return 0;
}
#endif

/*
 * Use the PBX to initiate a call from a specified TU to a specified extension.
 *
 * @param pbx  The PBX registry.
 * @param tu  The TU that is initiating the call.
 * @param ext  The extension number to be called.
 * @return 0 if dialing succeeds, otherwise -1.
 */
#if 1
int pbx_dial(PBX *pbx, TU *tu, int ext) {

    debug("in_dial");

    if(pbx == NULL || tu == NULL || ext < 0)
    {
        debug("err1");
        return -1;
    }
    if(pbx->tulist[ext] == NULL)
    {
        debug("NIL");
        return -1;
    }
    // sem_wait(&pbx->semaphore);
        debug("in_idal 2");
        debug("addr:%p", pbx->tulist[ext]);
        if(tu_dial(tu, pbx->tulist[ext]) == -1)
        {
            sem_post(&pbx->semaphore);
            debug("err2");
            return -1;
        }
        debug("in_dial 3");
    // sem_post(&pbx->semaphore);
    return 0;
}
#endif
