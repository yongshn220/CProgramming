


// program.c

typedef struct stmt_blk {
    struct stmt_blk *prev;
    struct stmt_blk *next;
    STMT *stmt;
    int isCounter;
} STMT_BLK;



STMT_BLK *stmt_blk_storage;
int stmt_blk_storage_max;
int stmt_blk_storage_count;



void prog_init();

STMT_BLK* create_newblk(STMT *stmt);

STMT_BLK * get_last_blk();

STMT_BLK* get_blk_by_lineno(int lineno);

int unlink_blk(STMT_BLK *blk);

void link_blk(STMT_BLK *prev, STMT_BLK *cur, STMT_BLK *next);


void clear_program();