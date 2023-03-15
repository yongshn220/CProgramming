

// store.c

typedef struct var_blk {
    char *name;
    char *val;
    struct var_blk *prev;
    struct var_blk *next;
} VAR_BLK;

VAR_BLK *var_blk_storage;
int var_blk_storage_max;
int var_blk_storage_count;


void store_init();
VAR_BLK* create_newsblk();
VAR_BLK* search_sblk(char *var);
void override_val(VAR_BLK *blk, char *val);
char* copy_str(char* c);
void free_sblk(VAR_BLK *blk);

void unlink_sblk(VAR_BLK *blk);

int insert_newsblk(char *var, char *val);

char* i_to_s(int val);
int s_to_i(char *val);

void clear_store();