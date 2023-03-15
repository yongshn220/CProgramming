/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "errno.h"


#define HDtoBLK(h) (sf_block*)(h - 1)

#define MG(v)  (v ^ MAGIC)
#define uMG(v) (v ^ MAGIC)

#define HD(p)      ((*p).header)
#define PLDadd(p)  (&HD(p) + 1)
#define FTadd(p)   (&((*p).header) + (GET_BLK_SIZE(&HD(p))/8) - 1)
#define PREV_FT(p) ((*p).prev_footer)

#define SET_PL_SIZE(h, v)      (*h = MG( ((v << 32) + GET_BLK_SIZE(h) + GET_OP(h)) ))
#define SET_BLK_SIZE(h, v)     (*h = MG( ((GET_PL_SIZE(h) << 32) + v + GET_OP(h)) ))
#define SET_OP(h, v)           (*h = MG( ((GET_PL_SIZE(h) << 32) + GET_BLK_SIZE(h) + v) ))
#define SET_OP_ALLOC(h, v)     (*h = MG( ((GET_PL_SIZE(h) << 32) + GET_BLK_SIZE(h) + (v << 2) + (GET_OP_PREVALLOC(h)<<1) + GET_OP_INQKLST(h)) ))
#define SET_OP_PREVALLOC(h, v) (*h = MG( ((GET_PL_SIZE(h) << 32) + GET_BLK_SIZE(h) + (GET_OP_ALLOC(h)<<2) + (v << 1) + GET_OP_INQKLST(h)) ))
#define SET_OP_INQKLST(h, v)   (*h = MG( ((GET_PL_SIZE(h) << 32) + GET_BLK_SIZE(h) + (GET_OP_ALLOC(h)<<2) + (GET_OP_PREVALLOC(h)<<1) + v) ))

#define SET_NEXT_PREALLOC(h, v) (SET_OP_PREVALLOC(GET_NEXT_HDadd(h), v))

#define GET_PL_SIZE(h)      ((uMG(*h) & 0xffffffff00000000) >> 32)
#define GET_BLK_SIZE(h)     (uMG(*h) & 0x00000000fffffff0)
#define GET_OP(h)           (uMG(*h) & 0xf)
#define GET_OP_ALLOC(h)     ((uMG(*h) & 0x4) >> 2)
#define GET_OP_PREVALLOC(h) ((uMG(*h) & 0x2) >> 1)
#define GET_OP_INQKLST(h)   (uMG(*h) & 0x1)

#define GET_PREV_BLK(p)   (sf_block*)((sf_header*)p - (GET_BLK_SIZE(&PREV_FT(p)) /8 ))
#define GET_PREV_ALLOC(h) (GET_OP_ALLOC((h - 1)))
#define GET_NEXT_BLK(p)   (sf_block*)(&HD(p) + (GET_BLK_SIZE(&HD(p))/8) - 1)
#define GET_NEXT_HDadd(h) (h + (GET_BLK_SIZE(h) / 8))
#define GET_NEXT_ALLOC(h) (GET_OP_ALLOC(GET_NEXT_HDadd(h)))

int firstMallocCall = -1;

double maxUtilization = 0.0;

int tryInit();
void initFreeLists();
int first_mem_grow();
void first_freeblk(sf_block* freeblk);
int set_template();

void set_header(sf_header* hd, uint64_t pl, int sz, int al, int pal, int iq);

int try_mem_grow();

sf_block* coalecing(sf_block* freeblk);

int is_prev_alloc(sf_block* freeblk);

void set_prologue(sf_header* hd);
void set_epilogue();

int set_freeblk(sf_block* freeblk);
int set_footer(sf_block* blk);
void set_nextBlk_prevAlloc(sf_block* blk);
void set_freeblk_header(sf_block* freeblk, sf_size_t size);

int push_to_freelists(sf_block* freeblk);
int push_freeblk(sf_block* freeblk, int i);
void unlink_freeblk(sf_block* freeblk);

sf_block* search_quicklists(sf_size_t size, sf_size_t padding);
sf_block* alloc_quickblk(sf_size_t size, sf_size_t padding, int i);
// sf_block* alloc_blk(sf_block* blk, sf_size_t size, sf_size_t padding);

sf_block* search_freelists(sf_size_t size, sf_size_t padding);
int alloc_freeblk(sf_block* freeblk, sf_size_t size, sf_size_t padding);
void alloc_freeblk_header(sf_block* freeblk, sf_size_t size, sf_size_t padding);
void push_quickBlk(sf_block* nfreeBlk, int i);

int flushing_quicklist(int i);

int alignCheck(void* pp);

int alignCheck(void* pp)
{
    sf_header* memStart = sf_mem_start();

    int diff = (((sf_header*)pp - memStart)*8) % 16;

    if(diff == 0) { return 0;}
    return -1;
}

int tryInit()
{
    if(firstMallocCall == -1)
    {
        firstMallocCall = 1;

        initFreeLists();

        if(first_mem_grow() == -1) { return -1; }
    }
    return 0;
}

void initFreeLists()
{
    for(int i = 0; i < 10; i++)
    {
        // sf_free_list_heads[i] = { .body.links.next = NULL};
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
}

int first_mem_grow()
{
    sf_header* memStart = sf_mem_start();
    sf_header* prevEnd = sf_mem_grow();

    if(prevEnd == NULL) return -1;
    set_template();
    sf_block* freeblk = (sf_block*)(memStart + 4); //mem_start_address + 32bytes => first blk

    set_epilogue();

    first_freeblk(freeblk); // 0: new block, -1: not new block

    return 0;
}

void set_header(sf_header* hd, uint64_t pl, int sz, int al, int pal, int iq)
{
    if(hd == NULL) {return;}

    if(pl >= 0)
    {
        SET_PL_SIZE(hd, pl);
    }
    if(sz >= 0)
    {
        SET_BLK_SIZE(hd, sz);
    }
    if(al >= 0)
    {
        SET_OP_ALLOC(hd, al);
    }
    if(pal >= 0)
    {
        SET_OP_PREVALLOC(hd, pal);
    }
    if(iq >= 0)
    {
        SET_OP_INQKLST(hd, iq);
    }
}

void first_freeblk(sf_block* freeblk)
{
    sf_header* memEnd = sf_mem_end();
    int size = ((memEnd-1) - &HD(freeblk)) * 8; //epilogue address - freeblk address

    set_header(&HD(freeblk),0,size,0,1,0);

    set_footer(freeblk);

    push_to_freelists(freeblk);
}

int set_template()
{
    sf_header* blk1 = sf_mem_start();
    *blk1 = 0;

    sf_header* prologue = blk1 + 1;
    set_prologue(prologue);

    return 0;
}

void set_prologue(sf_header* hd)
{
    set_header(hd, 0, 32, 1, 0, 0);
}

void set_epilogue()
{
    sf_header* memEnd = sf_mem_end();
    sf_header* epilogue = memEnd - 1;

    set_header(epilogue, 0,0,1,0,0);
}

//copy header and create footer.
int set_footer(sf_block* freeblk)
{
    sf_header* hd = &HD(freeblk);

    sf_header* ft = FTadd(freeblk);

    if(memcpy(ft, hd, sizeof(HD(freeblk))) == NULL)
    {
        return -1;
    }
    return 0;
}

int try_mem_grow()
{

    sf_header* prevEnd = sf_mem_grow();

    if(prevEnd == NULL) { return -1; } // mem_grow fail

    sf_header* prevEpilogue = prevEnd - 1;
    sf_header* curEnd = sf_mem_end();

    sf_block* freeblk = (sf_block*)(prevEnd - 2); // one block behind from prev end.

    sf_size_t size = ((curEnd-1) - &HD(freeblk)) * 8; //epilogue address - freeblk address

    set_epilogue();

    set_header(&HD(freeblk), 0, size, 0, GET_OP_PREVALLOC(prevEpilogue), 0);

    if(set_freeblk(freeblk) == -1) {return -1;}

    return 0;
}

int set_freeblk(sf_block* freeblk)
{

    freeblk = coalecing(freeblk); // need to modify

    if(push_to_freelists(freeblk) == -1) { return -1;}

    return 0;
}


void blkCheck(sf_block* blk)
{
    sf_header* hd = &HD(blk);
    sf_header* memEnd = sf_mem_end();

    printf("hd from memEnd : %ld\n", memEnd - hd);

    uint64_t pl = GET_PL_SIZE(hd);
    uint32_t sz = GET_BLK_SIZE(hd);
    int al = GET_OP_ALLOC(hd);
    int preval = GET_OP_PREVALLOC(hd);
    int inqck = GET_OP_INQKLST(hd);

    printf("pl : %lu\n", pl);
    printf("sz : %u\n", sz);
    printf("al : %d\n", al);
    printf("preval : %d\n", preval);
    printf("inqck : %d\n", inqck);
}
sf_block* coalecing(sf_block* freeblk)
{
    int isPrevBlkAlloc = GET_OP_PREVALLOC(&HD(freeblk));
    int isNextBlkAlloc = GET_NEXT_ALLOC(&HD(freeblk));

    sf_block* prevBlk = GET_PREV_BLK(freeblk);
    sf_block* nextBlk = GET_NEXT_BLK(freeblk);

    if(isPrevBlkAlloc == 1 && isNextBlkAlloc == 1)
    {
        if(set_footer(freeblk) == -1) { return NULL; }
        return freeblk;
    }
    else if(isPrevBlkAlloc == 1 && isNextBlkAlloc == 0)
    {
        unlink_freeblk(nextBlk);
        sf_size_t size = GET_BLK_SIZE(&HD(freeblk)) + GET_BLK_SIZE(&HD(nextBlk));
        SET_BLK_SIZE(&HD(freeblk), size);

        if(set_footer(freeblk) == -1) { return NULL; }
        return freeblk;
    }
    else if(isPrevBlkAlloc == 0 && isNextBlkAlloc == 1)
    {
        unlink_freeblk(prevBlk);
        sf_size_t size = GET_BLK_SIZE(&HD(prevBlk)) + GET_BLK_SIZE(&HD(freeblk));
        SET_BLK_SIZE(&HD(prevBlk), size);

        if(set_footer(prevBlk) == -1) { return NULL; }
        return prevBlk;
    }
    else
    {
        unlink_freeblk(prevBlk);
        unlink_freeblk(nextBlk);
        sf_size_t size = GET_BLK_SIZE(&HD(prevBlk)) + GET_BLK_SIZE(&HD(freeblk)) + GET_BLK_SIZE(&HD(nextBlk));
        SET_BLK_SIZE(&HD(prevBlk), size);

        if(set_footer(prevBlk) == -1) { return NULL; }
        return prevBlk;
    }
    return NULL;
}

// return 0 if its footer and header are the same. otherwise return -1
int is_prev_alloc(sf_block* freeblk)
{
    sf_footer* ft = &PREV_FT(freeblk);
    // get the corresponding header of the following footer.
    sf_header* hd = ft - GET_BLK_SIZE(ft) - 1;

    if(hd == ft && GET_OP_ALLOC(ft) == 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int push_to_freelists(sf_block* freeblk)
{
    if(freeblk == NULL) {return -1;}

    int size = GET_BLK_SIZE(&HD(freeblk));

    int curSize = 32;

    for(int i = 0; i < NUM_FREE_LISTS; i++)
    {
        if(size <= curSize)
        {
            push_freeblk(freeblk, i);
            return 0;
        }
        curSize = curSize * 2;
    }
    push_freeblk(freeblk, NUM_FREE_LISTS - 1);
    return 0;
}

int push_freeblk(sf_block* freeblk, int i)
{
    sf_block* firstBlk = sf_free_list_heads[i].body.links.next;

    (*freeblk).body.links.next = firstBlk; // newblock -> first block
    (*firstBlk).body.links.prev = freeblk; // new block <- first block

    sf_free_list_heads[i].body.links.next = freeblk; // sentinel -> new block
    (*freeblk).body.links.prev = &sf_free_list_heads[i]; //sentinel <- new block
    return 0;
}

sf_block* search_quicklists(sf_size_t size, sf_size_t padding)
{
    if(size + padding > 16*9) {return NULL;}
    int i = ((size + padding) - 32)/16;

    if(sf_quick_lists[i].length > 0)
    {
        return alloc_quickblk(size, padding, i);
    }

    // for(int i = 0; i < NUM_QUICK_LISTS; i++)
    // {
    //     if(sf_quick_lists[i].length > 0)
    //     {
    //         return alloc_quickblk(size, padding, i);

    //     }
    // }
    return NULL;
}

sf_block* alloc_quickblk(sf_size_t size, sf_size_t padding, int i)
{
    sf_block* firstBlk = sf_quick_lists[i].first;

    if(sf_quick_lists[i].length > 1)
    {
        sf_block* nextBlk = (*firstBlk).body.links.next;
        sf_quick_lists[i].first = nextBlk;
    }
    else
    {
        sf_quick_lists[i].first = NULL;
    }

    sf_quick_lists[i].length -= 1;

    // alloc_freeblk_header(firstBlk, size, padding);

    set_header(&HD(firstBlk), size, size+padding, 1, -1, 0);

    // set_nextBlk_prevAlloc(firstBlk);
    SET_NEXT_PREALLOC(&HD(firstBlk), 1);

    return firstBlk;
}

sf_block* search_freelists(sf_size_t size, sf_size_t padding)
{

    sf_size_t totalSize = size + padding;
    int memGrowResult = 1;

    while(memGrowResult == 1)
    {
        for(int i = 0; i < NUM_FREE_LISTS; i++)
        {
            sf_block* sentinel = &sf_free_list_heads[i];
            sf_block* curBlk = (*sentinel).body.links.next;
            while(curBlk != sentinel)
            {
                if(totalSize <= GET_BLK_SIZE(&HD(curBlk)))
                {
                    unlink_freeblk(curBlk);

                    if(alloc_freeblk(curBlk, size, padding) == -1) {return NULL;}

                    return curBlk;
                }
                curBlk = (*curBlk).body.links.next;
            }
        }

        if (try_mem_grow() == -1)
        {
            return NULL;
        }
    }
    return NULL;
}

void unlink_freeblk(sf_block* freeblk)
{
    sf_block* prevBlk = (*freeblk).body.links.prev;
    sf_block* nextBlk = (*freeblk).body.links.next;

    (*prevBlk).body.links.next = nextBlk;
    (*nextBlk).body.links.prev = prevBlk;
}

int alloc_freeblk(sf_block* freeblk, sf_size_t size, sf_size_t padding)
{
    sf_size_t totalSize = size + padding;
    sf_size_t freeblkSize = GET_BLK_SIZE(&HD(freeblk));
    sf_size_t remainSize = freeblkSize - totalSize;

    if(remainSize >= 32)
    {

        // alloc_freeblk_header(freeblk, size, padding);
        set_header(&HD(freeblk), size, size+padding, 1, -1, 0);

        // set_nextBlk_prevAlloc(firstBlk);

        int step = (size+padding)/8;

        sf_block* splitedBlk = (sf_block*)((sf_header*)freeblk + step);

        // set_freeblk_header(splitedBlk, remainSize);
        set_header(&HD(splitedBlk), 0, remainSize, 0, 1, 0);

        if(set_footer(splitedBlk) == -1) {return -1;}

        set_freeblk(splitedBlk);

    }
    else
    {
        // alloc_freeblk_header(freeblk, size, remainSize + padding);

        set_header(&HD(freeblk), size, size+padding, 1, -1, 0);

        // set_nextBlk_prevAlloc(firstBlk);
        SET_NEXT_PREALLOC(&HD(freeblk), 1);
    }

    return 0;
}


void print_info()
{
    sf_block* tests = sf_mem_start();
    sf_block* teste = sf_mem_end();
    printf("mem start: %p\n", tests);
    printf("mem end: %p\n", teste);
}

sf_size_t get_padding(sf_size_t size)
{

    sf_size_t padding = 0;

    if(size < 32)
    {
        padding = 32 - size;
    }
    else
    {
        padding = (16 - (size % 16));
        if(padding == 16)
        {
            padding = 0;
        }
    }
    return padding;
}

void *sf_malloc(sf_size_t size) {

    if(tryInit() == -1) { return NULL; }

    if(size <= 0) {return NULL;}
    size = size + 8; // add header bytes

    sf_size_t padding = get_padding(size); // padding (align = 16bytes, min = 32bytes)

    size = size - 8;
    padding = padding + 8;

    sf_block* quickBlk = search_quicklists(size, padding); // fit blk in quick lists. otherwise NULL

    if(quickBlk != NULL)
    {
        sf_peak_utilization();
        return PLDadd(quickBlk); // return the payload address.
    }
    else
    {
        sf_block* resultBlk = search_freelists(size, padding);
        if(resultBlk == NULL)
        {
            sf_errno = ENOMEM;
            return NULL;
        }
        else
        {
            sf_peak_utilization();
            return PLDadd(resultBlk);
        }
    }
}

int push_to_quickLists(sf_block* nfreeBlk)
{
    sf_size_t blkSize = GET_BLK_SIZE(&HD(nfreeBlk));
    sf_size_t qckSize = -1;
    for(int i = 0; i < NUM_QUICK_LISTS; i++)
    {
        qckSize = (i * 16) + 32;
        if(blkSize == qckSize)
        {
            if(sf_quick_lists[i].length == QUICK_LIST_MAX)
            {
                flushing_quicklist(i);
            }
            push_quickBlk(nfreeBlk, i);

            return 0;
        }
    }
    return -1;
}

void push_quickBlk(sf_block* nfreeBlk, int i)
{
    set_header(&HD(nfreeBlk), 0, -1, -1, -1, 1); // inqick = 1, rest of them are the same
    if(sf_quick_lists[i].length == 0)
    {
        sf_quick_lists[i].first = nfreeBlk;
        (*nfreeBlk).body.links.next = NULL;
    }
    else
    {
        sf_block* first = sf_quick_lists[i].first;
        sf_quick_lists[i].first = nfreeBlk;
        (*nfreeBlk).body.links.next = first;
    }
    sf_quick_lists[i].length += 1;
}

int flushing_quicklist(int i)
{
    sf_block* blk = sf_quick_lists[i].first;
    sf_block* next = NULL;
    for(int q = 0; q < sf_quick_lists[i].length; q++)
    {
        next = (*blk).body.links.next;
        set_header(&HD(blk), 0, -1, 0, -1, 0);

        SET_NEXT_PREALLOC(&HD(blk), 0);

        if(set_freeblk(blk) == -1) {return -1;} // push to appropriate freelists
        blk = next;
    }
    sf_quick_lists[i].first = NULL;
    sf_quick_lists[i].length = 0;
    return 0;
}

int sf_free_valid(void *pp)
{
    if(pp == NULL)           {return -1;}
    if(alignCheck(pp) == -1) {return -1;}

    sf_header* header = (sf_header*)pp - 1;
    sf_block* firstBlk = sf_mem_start();
    sf_block* endOfLastBlk = sf_mem_end() - 8;

    if(GET_BLK_SIZE(header) < 32)         {return -1;}
    if(GET_BLK_SIZE(header) % 16 != 0)    {return -1;}
    if(header < (sf_header*)firstBlk)     {return -1;}
    if(header > (sf_header*)endOfLastBlk) {return -1;}
    if(GET_OP_ALLOC(header) == 0)         {return -1;}
    if(GET_OP_PREVALLOC(header) == 0 && GET_PREV_ALLOC(header) != 0) {return -1;}

    return 0;
}

void sf_free(void *pp) {

    if(sf_free_valid(pp) == -1) { abort(); }


    sf_block* nfreeBlk = (sf_block*)((sf_header*)pp - 2);

    if(push_to_quickLists(nfreeBlk) == 0) {
        sf_peak_utilization();
        return;
    }

    set_header(&HD(nfreeBlk), 0, -1, 0, -1, 0);

    SET_NEXT_PREALLOC(&HD(nfreeBlk), 0);

    set_freeblk(nfreeBlk);

    sf_peak_utilization();

    return;
}

void *sf_realloc(void *pp, sf_size_t rsize) {

    if(sf_free_valid(pp) == -1)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    sf_block* allocBlk = (sf_block*)((sf_header*)pp - 2);

    rsize = rsize + 8; // add header bytes
    sf_size_t padding = get_padding(rsize); // padding (align = 16bytes, min = 32bytes)
    rsize = rsize - 8;
    padding = padding + 8;

    sf_size_t rBlkSize = rsize + padding;
    sf_size_t pBlkSize = GET_BLK_SIZE(&HD(allocBlk));

    if(rBlkSize == pBlkSize)
    {
        return pp;
    }
    else if(rBlkSize > pBlkSize)
    {
        void *np = sf_malloc(rsize);
        if(np == NULL) {return NULL;}

        memcpy(np, pp, GET_BLK_SIZE(&HD(allocBlk)));

        sf_free(pp);

        return np;
    }
    else
    {
        sf_size_t diff = pBlkSize - rBlkSize;
        if(diff < 32)
        {
            SET_PL_SIZE(&HD(allocBlk), (uint64_t)rsize);
            return pp;
        }
        else
        {

            sf_block* newFreeBlk = (sf_block*)((sf_header*)allocBlk + (rBlkSize/8));

            set_header(&HD(allocBlk), (uint64_t)rsize, rBlkSize, 1, -1, 0);

            set_header(&HD(newFreeBlk), 0, diff, 0, 1, 0);
            set_footer(newFreeBlk);
            set_freeblk(newFreeBlk);

            return pp;
        }
    }
    return NULL;
}

double sf_internal_fragmentation() {
    uint64_t totalSize = 0;
    uint64_t totalPayload = 0;

    sf_block* memStart = sf_mem_start();
    sf_header* epilogue = sf_mem_end() - 8;

    sf_header* firstBlk = &HD(memStart) + (GET_BLK_SIZE(&HD(memStart))/8);

    sf_header* curHeader = firstBlk;

    while(curHeader < epilogue)
    {
        int isAlloc = GET_OP_ALLOC(curHeader);
        int isInqck = GET_OP_INQKLST(curHeader);

        if(isAlloc == 1 && isInqck == 0)
        {
            totalSize += GET_BLK_SIZE(curHeader);
            totalPayload += GET_PL_SIZE(curHeader);
        }
        curHeader += GET_BLK_SIZE(curHeader)/8;
    }
    if(totalPayload == 0) {return 0.0;}

    return (double)totalPayload / (double)totalSize;
}

double sf_peak_utilization() {
    double thisUtilzation = sf_internal_fragmentation();
    if(maxUtilization < thisUtilzation)
    {
        maxUtilization = thisUtilzation;
    }
    return maxUtilization;
}
