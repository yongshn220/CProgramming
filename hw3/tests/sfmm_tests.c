#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == ((bp->header ^ MAGIC) & 0xfffffff0))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
 * Assert the total number of quick list blocks of a specified size.
 * If size == 0, then assert the total number of all quick list blocks.
 */
void assert_quick_list_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_QUICK_LISTS; i++) {
	sf_block *bp = sf_quick_lists[i].first;
	while(bp != NULL) {
	    if(size == 0 || size == ((bp->header ^ MAGIC) & 0xfffffff0)) {
		cnt++;
		if(size != 0) {
		    // Check that the block is in the correct list for its size.
		    int index = (size - 32) >> 4;
		    cr_assert_eq(index, i, "Block %p (size %ld) is in wrong quick list for its size "
				 "(expected %d, was %d)",
				 &bp->header, (bp->header ^ MAGIC) & 0xfffffff0, index, i);
		}
	    }
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of quick list blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of quick list blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz = sizeof(int);
	int *x = sf_malloc(sz);

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");
	sf_block *bp = (sf_block *)((char *)x - 16);
	cr_assert((bp->header >> 32) & 0xffffffff,
		  "Malloc'ed block payload size (%ld) not what was expected (%ld)!",
		  (bp->header >> 32) & 0xffffffff, sz);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(944, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;

	void *x = sf_malloc(4032);
	cr_assert_not_null(x, "x is NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(98304);

	cr_assert_null(x, "x is not NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(24528, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_basecode_suite, free_quick, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 32, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(48, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(864, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 200, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(208, 1);
	assert_free_block_count(704, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_w = 8, sz_x = 200, sz_y = 300, sz_z = 4;
	/* void *w = */ sf_malloc(sz_w);
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);
	sf_free(x);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(384, 1);
	assert_free_block_count(528, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
        size_t sz_u = 200, sz_v = 150, sz_w = 50, sz_x = 150, sz_y = 200, sz_z = 250;
	void *u = sf_malloc(sz_u);
	/* void *v = */ sf_malloc(sz_v);
	void *w = sf_malloc(sz_w);
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_quick_list_block_count(0, 1);
	assert_free_block_count(0, 3);
	assert_free_block_count(208, 2);
	assert_free_block_count(928, 1);

	// First block in list should be the most recently freed block not in quick list.
	int i = 3;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(&bp->header, (char *)y - 8,
		     "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, &bp->header, (char *)y - 8);
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int), sz_y = 10, sz_x1 = sizeof(int) * 20;
	void *x = sf_malloc(sz_x);
	/* void *y = */ sf_malloc(sz_y);
	x = sf_realloc(x, sz_x1);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 16);
	cr_assert((bp->header ^ MAGIC) & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & 0xfffffff0) == 96,
		  "Realloc'ed block size (%ld) not what was expected (%ld)!",
		  (bp->header ^ MAGIC) & 0xfffffff0, 96);
	cr_assert((((bp->header ^ MAGIC) >> 32) & 0xffffffff) == sz_x1,
		  "Realloc'ed block payload size (%ld) not what was expected (%ld)!",
		  (((bp->header ^ MAGIC) >> 32) & 0xffffffff), sz_x1);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(32, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(816, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int) * 20, sz_y = sizeof(int) * 16;
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char *)x - 16);
	cr_assert((bp->header ^ MAGIC) & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & 0xfffffff0) == 96,
		  "Realloc'ed block size (%ld) not what was expected (%ld)!",
		  (bp->header ^ MAGIC) & 0xfffffff0, 96);
	cr_assert((((bp->header ^ MAGIC) >> 32) & 0xffffffff) == sz_y,
		  "Realloc'ed block payload size (%ld) not what was expected (%ld)!",
		  (((bp->header ^ MAGIC) >> 32) & 0xffffffff), sz_y);

	// There should be only one free block.
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(880, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(double) * 8, sz_y = sizeof(int);
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char *)x - 16);
	cr_assert((bp->header ^ MAGIC) & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert(((bp->header ^ MAGIC) & 0xfffffff0) == 32,
		  "Realloc'ed block size (%ld) not what was expected (%ld)!",
		  (bp->header ^ MAGIC) & 0xfffffff0, 32);
	cr_assert((((bp->header ^ MAGIC) >> 32) & 0xffffffff) == sz_y,
		  "Realloc'ed block payload size (%ld) not what was expected (%ld)!",
		  (((bp->header ^ MAGIC) >> 32) & 0xffffffff), sz_y);

	// After realloc'ing x, we can return a block of size 48
	// to the freelist.  This block will go into the main freelist and be coalesced.
	// Note that we don't put split blocks into the quick lists because their sizes are not sizes
	// that were requested by the client, so they are not very likely to satisfy a new request.
	assert_quick_list_block_count(0, 0);	
	assert_free_block_count(0, 1);
	assert_free_block_count(944, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

#define T_MG(v)  (v ^ MAGIC)
#define T_uMG(v) (v ^ MAGIC)
#define T_HD(p)      ((*p).header)
#define T_PLDadd(p)  (&T_HD(p) + 1)
#define T_FTadd(p)   (&((*p).header) + (T_GET_BLK_SIZE(&HD(p))/8) - 1)
#define T_PREV_FT(p) ((*p).prev_footer)
#define T_GET_PL_SIZE(h)      ((T_uMG(*h) & 0xffffffff00000000) >> 32)
#define T_GET_BLK_SIZE(h)     (T_uMG(*h) & 0x00000000fffffff0)
#define T_GET_OP(h)           (T_uMG(*h) & 0xf)
#define T_GET_OP_ALLOC(h)     ((T_uMG(*h) & 0x4) >> 2)
#define T_GET_OP_PREVALLOC(h) ((T_uMG(*h) & 0x2) >> 1)
#define T_GET_OP_INQKLST(h)   (T_uMG(*h) & 0x1)

void assert_quick_same_size(void* pp) {

    sf_block* blk = (sf_block*)((sf_header*)pp - 2);

    uint32_t blkSize = T_GET_BLK_SIZE(&T_HD(blk));
    uint64_t plSize = T_GET_PL_SIZE(&T_HD(blk));

    cr_assert_eq(blkSize, 80, "Wrong block size (exp=%d, found=%d)", 80, blkSize);
    cr_assert_eq(plSize, 64, "Wrong payload size (exp=%d, found=%d)", 64, blkSize);

}

Test(sfmm_student_suite, complicated_mixture_test, .timeout = TEST_TIMEOUT) {
        size_t s1 = 1, s2 = 203, s3 = 502, s4 = 560, s5 = 402, s6 = 103, s7= 30;
        sf_malloc(s1);
        sf_malloc(s2);
        void* a1 = sf_malloc(s3);
        void* a2 = sf_malloc(s4);
        sf_free(a1);
        void* a3 = sf_malloc(s5);
        sf_realloc(a2, s5);
        void* a4 = sf_malloc(s6);
        void* m5 = sf_malloc(s7);
        void* m6 = sf_malloc(s6);
        void* m7 = sf_malloc(s7);
        sf_free(a3);
        sf_free(a4);
        sf_free(m5);
        sf_free(m6);
        sf_free(m7);

        assert_free_block_count(0, 2);
        assert_quick_list_block_count(0,4);
}

Test(sfmm_student_suite, realloc_same_size_test, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(double) * 8;
        size_t sz_y = sizeof(double) * 8;

	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	assert_quick_same_size(y);
}

Test(sfmm_student_suite, quickLists_flushing_test, .timeout = TEST_TIMEOUT) {
 	size_t sz_w = 30;

    	void* a1 = sf_malloc(sz_w);
   	void* a2 = sf_malloc(sz_w);
    	void* a3 = sf_malloc(sz_w);
   	void* a4 = sf_malloc(sz_w);
    	void* a5 = sf_malloc(sz_w);
    	void* a6 = sf_malloc(sz_w);
    	void* a7 = sf_malloc(sz_w);
    	void* a8 = sf_malloc(sz_w);
    	void* a9 = sf_malloc(sz_w);
    	void* a10 = sf_malloc(sz_w);
    	void* a11 = sf_malloc(sz_w);

    	sf_free(a1);
    	sf_free(a2);
    	sf_free(a3);
    	sf_free(a4);
    	sf_free(a5);
    	sf_free(a6);
    	sf_free(a7);
    	sf_free(a8);
    	sf_free(a9);
    	sf_free(a10);
    	sf_free(a11);

    	assert_quick_list_block_count(0, 1);
}

Test(sfmm_student_suite, internal_fragment_test, .timeout = TEST_TIMEOUT) {
 	size_t sz_u = 1, sz_v = 2, sz_w = 30, sz_x = 153, sz_y = 201, sz_z = 253;
    	sf_malloc(sz_u);
    	sf_malloc(sz_v);
    	void* a1 = sf_malloc(sz_w);
    	void* a2 = sf_malloc(sz_w);
    	void* a3 = sf_malloc(sz_z);
    	void* a4 = sf_malloc(sz_w);
    	sf_malloc(sz_y);
    	sf_malloc(sz_w);
    	void* a7 = sf_malloc(sz_z);
    	void* a8 = sf_malloc(sz_y);
    	void* a9 = sf_malloc(sz_w);
    	sf_malloc(sz_x);
    	void* b = sf_malloc(sz_y);
    	sf_malloc(sz_z);

    	sf_free(a1);
    	sf_free(a2);
    	sf_free(a3);
    	sf_free(a4);
    	sf_free(a7);
    	sf_free(a8);
    	sf_free(a9);
    	sf_free(b);

    	double frag = sf_internal_fragmentation();

    	cr_assert(frag < 0.817 && frag > 0.816, "sf_internal_fragmentation is wrong!");
}

Test(sfmm_student_suite, peak_utilization_test, .timeout = TEST_TIMEOUT) {
 	size_t sz_u = 1, sz_v = 2, sz_w = 30, sz_x = 153, sz_y = 201, sz_z = 253;
    	sf_malloc(sz_u);

    	double max1 = sf_peak_utilization();
    	sf_malloc(sz_v);
    	void* a1 = sf_malloc(sz_w);
    	void* a2 = sf_malloc(sz_w);
    	void* a3 = sf_malloc(sz_z);
    	void* a4 = sf_malloc(sz_w);
    	sf_malloc(sz_y);
    	sf_malloc(sz_w);

    	double max2 = sf_peak_utilization();
    	void* a7 = sf_malloc(sz_z);
    	void* a8 = sf_malloc(sz_y);
    	void* a9 = sf_malloc(sz_w);
    	sf_malloc(sz_x);
    	void* b = sf_malloc(sz_y);
    	sf_malloc(sz_z);

    	sf_free(a1);
    	sf_free(a2);
    	sf_free(a3);
    	sf_free(a4);
    	sf_free(a7);
    	sf_free(a8);
    	sf_free(a9);
    	sf_free(b);

    	double max3 = sf_peak_utilization();

    	cr_assert(max1 < 0.04 && max1> 0.03, "1st sf_peak_utilization is wrong!");
    	cr_assert(max2 < 0.78 && max2 > 0.77, "2nd sf_peak_utilization is wrong!");
    	cr_assert(max3 < 0.859 && max3 > 0.858, "3rd sf_peak_utilization is wrong!");
}