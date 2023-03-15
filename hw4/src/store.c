#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sdata.h>


VAR_BLK s_sentinelblk = {.name = NULL, .val = NULL, .prev = NULL, .next = NULL};

void store_init()
{
    var_blk_storage = malloc(100 * sizeof(VAR_BLK));
    var_blk_storage_max = 100;
}

VAR_BLK* create_newsblk()
{
    if(var_blk_storage_count >= var_blk_storage_max - 1)
    {
        var_blk_storage = realloc(var_blk_storage, 2 * var_blk_storage_max * sizeof(VAR_BLK));
        var_blk_storage_max = 2 * var_blk_storage_max;
    }
    VAR_BLK newblk = {.name = NULL, .val = NULL, .prev = NULL, .next = NULL};
    var_blk_storage[var_blk_storage_count] = newblk;

    var_blk_storage_count++;

    return &var_blk_storage[var_blk_storage_count-1];
}

/*
 * This is the "data store" module for Mush.
 * It maintains a mapping from variable names to values.
 * The values of variables are stored as strings.
 * However, the module provides functions for setting and retrieving
 * the value of a variable as an integer.  Setting a variable to
 * an integer value causes the value of the variable to be set to
 * a string representation of that integer.  Retrieving the value of
 * a variable as an integer is possible if the current value of the
 * variable is the string representation of an integer.
 */

/**
 * @brief  Get the current value of a variable as a string.
 * @details  This function retrieves the current value of a variable
 * as a string.  If the variable has no value, then NULL is returned.
 * Any string returned remains "owned" by the data store module;
 * the caller should not attempt to free the string or to use it
 * after any subsequent call that would modify the value of the variable
 * whose value was retrieved.  If the caller needs to use the string for
 * an indefinite period, a copy should be made immediately.
 *
 * @param  var  The variable whose value is to be retrieved.
 * @return  A string that is the current value of the variable, if any,
 * otherwise NULL.
 */
char *store_get_string(char *var) {
    if(var == NULL)
    {
        printf("ERR: var is null (store.c:get_string)");
        return NULL;
    }

    VAR_BLK *targetblk = search_sblk(var);

    if(targetblk == NULL)
    {
        return NULL;
    }
    else
    {
        if(targetblk->val == NULL)
        {
            printf("ERR: val is null (store.c:get_string)");
            return NULL;
        }

        return targetblk->val;
    }
}

/**
 * @brief  Get the current value of a variable as an integer.
 * @details  This retrieves the current value of a variable and
 * attempts to interpret it as an integer.  If this is possible,
 * then the integer value is stored at the pointer provided by
 * the caller.
 *
 * @param  var  The variable whose value is to be retrieved.
 * @param  valp  Pointer at which the returned value is to be stored.
 * @return  If the specified variable has no value or the value
 * cannot be interpreted as an integer, then -1 is returned,
 * otherwise 0 is returned.
 */
int store_get_int(char *var, long *valp) {
    if(var == NULL)
    {
        printf("ERR: var is null (store.c:get_int)");
        return -1;
    }

    VAR_BLK *targetblk = search_sblk(var);

    if(targetblk == NULL)
    {
        return -1;
    }
    else
    {
        if(targetblk->val == NULL)
        {
            printf("ERR: val is null (store.c:get_int)");
            return -1;
        }
        char* endp;

        int val = strtol(targetblk->val, &endp, 10);

        if(*endp != '\0') return -1;

        *valp = val;

        return 0;
    }
}

/**
 * @brief  Set the value of a variable as a string.
 * @details  This function sets the current value of a specified
 * variable to be a specified string.  If the variable already
 * has a value, then that value is replaced.  If the specified
 * value is NULL, then any existing value of the variable is removed
 * and the variable becomes un-set.  Ownership of the variable and
 * the value strings is not transferred to the data store module as
 * a result of this call; the data store module makes such copies of
 * these strings as it may require.
 *
 * @param  var  The variable whose value is to be set.
 * @param  val  The value to set, or NULL if the variable is to become
 * un-set.
 */
int store_set_string(char *var, char *val) {

    if(var == NULL) {return -1;}

    VAR_BLK *targetblk = search_sblk(var);

    if(targetblk != NULL)
    {
        if(val == NULL)
        {
            unlink_sblk(targetblk);
            free_sblk(targetblk);
            return 0;
        }
        else
        {
            char *cpyvar = copy_str(val);
            override_val(targetblk, cpyvar);
            return 0;
        }
    }
    else
    {
        char *copyVar = copy_str(var);
        char *copyVal = copy_str(val);

        if(insert_newsblk(copyVar, copyVal) == -1) {return -1;}

        return 0;
    }
}


/**
 * @brief  Set the value of a variable as an integer.
 * @details  This function sets the current value of a specified
 * variable to be a specified integer.  If the variable already
 * has a value, then that value is replaced.  Ownership of the variable
 * string is not transferred to the data store module as a result of
 * this call; the data store module makes such copies of this string
 * as it may require.
 *
 * @param  var  The variable whose value is to be set.
 * @param  val  The value to set.
 */
int store_set_int(char *var, long val) {
    if(var == NULL)
    {
        printf("ERR: var is null (store.c:store_set_int)");
        return -1;
    }

    VAR_BLK *targetblk = search_sblk(var);


    char *cvtval = i_to_s(val);

    if(targetblk != NULL)
    {
        override_val(targetblk, cvtval);
        return 0;
    }
    else
    {
        char *cpyvar = copy_str(var);

        if(insert_newsblk(cpyvar, cvtval) == -1) {return -1;}

        return 0;
    }
}

/**
 * @brief  Print the current contents of the data store.
 * @details  This function prints the current contents of the data store
 * to the specified output stream.  The format is not specified; this
 * function is intended to be used for debugging purposes.
 *
 * @param f  The stream to which the store contents are to be printed.
 */
void store_show(FILE *f) {

    VAR_BLK *curblk = s_sentinelblk.next;

    printf("{");
    while(curblk != NULL)
    {
        if(curblk->name == NULL)
        {
            printf("ERR: name is null (store.c:store_show");
            return;
        }

        if(curblk->val != NULL)
        {
            printf("%s=%s", curblk->name, curblk->val);
        }
        curblk = curblk->next;
        if(curblk != NULL)
        {
            printf(", ");
        }
    }
    printf("}\n");
}


char* i_to_s(int val)
{
    char *cvtstr = malloc(3*sizeof(char));
    sprintf(cvtstr, "%d", val);
    return cvtstr;
}


int insert_newsblk(char *var, char *val)
{
    if(var == NULL || val == NULL)
    {
        printf("ERR: var or val is null (store.c:insert_newblk)");
        return -1;
    }

    VAR_BLK *newblk = create_newsblk();
    newblk->name = var;
    newblk->val = val;

    VAR_BLK *firstblk = s_sentinelblk.next;

    s_sentinelblk.next = newblk;
    newblk->prev = &s_sentinelblk;

    if(firstblk != NULL)
    {
        newblk->next = firstblk;
        firstblk->prev = newblk;
    }
    return 0;
}


void free_sblk(VAR_BLK *blk)
{
    if(blk == NULL)
    {
        return;
    }

    if(blk->name != NULL)
    {
        free(blk->name);
        blk->name = NULL;
    }
    if(blk->val != NULL)
    {
        free(blk->val);
        blk->val = NULL;
    }
}

void override_val(VAR_BLK *blk, char *val)
{
    if(blk->val != NULL)
    {
        free(blk->val);
    }
    blk->val = val;
}


char* copy_str(char* c)
{
    if(c == NULL)
    {
        printf("ERR: char is NULL (store.c:copy_str");
        return NULL;
    }
    char* newchar = malloc(sizeof(char) * (strlen(c) + 1));

    strcpy(newchar, c);

    return newchar;
}

VAR_BLK* search_sblk(char *var)
{
    VAR_BLK *curblk = s_sentinelblk.next;

    while(curblk != NULL)
    {
        if(curblk->name == NULL || var == NULL)
        {
            printf("ERR: NULL pointer (store.c: search_blk)");
            return NULL;
        }

        if(strcmp(curblk->name, var) == 0)
        {
            return curblk;
        }
        curblk = curblk->next;
    }
    return NULL;
}

void unlink_sblk(VAR_BLK *blk)
{
    VAR_BLK *prev = blk->prev;
    VAR_BLK *next = blk->next;

    if(prev == NULL) {return;}

    if(next == NULL)
    {
        prev->next = NULL;  // prev -x-> blk;
    }
    else
    {
        prev->next = next;
        next->prev = prev;
    }

    blk->prev = NULL;
    blk->next = NULL;
}


void clear_store()
{
    VAR_BLK *curblk = s_sentinelblk.next;

    while(curblk != NULL)
    {
        if(curblk->name != NULL)
        {
            free_sblk(curblk);
        }
        curblk = curblk->next;
    }

    free(var_blk_storage);
}