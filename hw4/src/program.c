#include <stdlib.h>
#include <stdio.h>

#include "mush.h"
#include "debug.h"


#include "data.h"


STMT_BLK sentinelblk = {.prev = NULL, .next = NULL, .stmt = NULL, .isCounter = 0};
STMT_BLK counterblk = {.prev = NULL, .next = NULL, .stmt = NULL, .isCounter = 1};


void prog_init()
{
    stmt_blk_storage = malloc(100 * sizeof(STMT_BLK));
    stmt_blk_storage_max = 100;
    sentinelblk.next = &counterblk;
    counterblk.prev = &sentinelblk;
}

STMT_BLK* create_newblk(STMT *stmt)
{
    if(stmt_blk_storage_count >= stmt_blk_storage_max - 1)
    {
        stmt_blk_storage = realloc(stmt_blk_storage, 2 * stmt_blk_storage_max * sizeof(STMT_BLK));
        stmt_blk_storage_max = 2 * stmt_blk_storage_max;
    }
    STMT_BLK newblk = {.prev = NULL, .next = NULL, .stmt = stmt, .isCounter = 0};
    stmt_blk_storage[stmt_blk_storage_count] = newblk;

    stmt_blk_storage_count++;

    return &stmt_blk_storage[stmt_blk_storage_count-1];
}

/*
 * This is the "program store" module for Mush.
 * It maintains a set of numbered statements, along with a "program counter"
 * that indicates the current point of execution, which is either before all
 * statements, after all statements, or in between two statements.
 * There should be no fixed limit on the number of statements that the program
 * store can hold.
 */

/**
 * @brief  Output a listing of the current contents of the program store.
 * @details  This function outputs a listing of the current contents of the
 * program store.  Statements are listed in increasing order of their line
 * number.  The current position of the program counter is indicated by
 * a line containing only the string "-->" at the current program counter
 * position.
 *
 * @param out  The stream to which to output the listing.
 * @return  0 if successful, -1 if any error occurred.
 */
int prog_list(FILE *out) {
    STMT_BLK *curblk = sentinelblk.next;

    while(curblk != NULL)
    {
        if(curblk->isCounter == 1)
        {
            printf("-->\n");
            curblk = curblk->next;
            continue;
        }

        if(curblk->stmt == NULL)
        {
            printf("ERR: (prog_list) - no stmt");
            return -1;
        }

        show_stmt(out, curblk->stmt);

        curblk =  curblk->next;
    }
    return 0;
}

/**
 * @brief  Insert a new statement into the program store.
 * @details  This function inserts a new statement into the program store.
 * The statement must have a line number.  If the line number is the same as
 * that of an existing statement, that statement is replaced.
 * The program store assumes the responsibility for ultimately freeing any
 * statement that is inserted using this function.
 * Insertion of new statements preserves the value of the program counter:
 * if the position of the program counter was just before a particular statement
 * before insertion of a new statement, it will still be before that statement
 * after insertion, and if the position of the program counter was after all
 * statements before insertion of a new statement, then it will still be after
 * all statements after insertion.
 *
 * @param stmt  The statement to be inserted.
 * @return  0 if successful, -1 if any error occurred.
 */
int prog_insert(STMT *stmt) {

    if(stmt == NULL)
    {
        printf("ERR: no stmt");
        return -1;
    }

    STMT_BLK *lastblk = get_last_blk();

    if(lastblk == NULL)
    {
        printf("ERR: Last block not exits.");
        return -1;
    }

    STMT_BLK *curblk = sentinelblk.next;

    STMT_BLK *newblk = create_newblk(stmt);
    while(1)
    {
        if(curblk == NULL) { printf("ERR: no cur blk"); return -1;}

        if(curblk->isCounter)
        {
            if(curblk->next == NULL) // if curblk is last block
            {
                link_blk(curblk->prev, newblk, curblk);
                break;
            }
            else
            {
                curblk = curblk->next;
                continue;
            }
        }

        int curlineno = curblk->stmt->lineno;
        if(curblk->next == NULL)
        {
            if(curlineno < stmt->lineno)
            {
                link_blk(curblk, newblk, NULL);
                break;
            }
            else
            {
                link_blk(curblk->prev, newblk, curblk);
                break;
            }
        }

        else
        {
            if( curlineno == stmt->lineno)
            {
                free_stmt(curblk->stmt);
                curblk->stmt = newblk->stmt;
                break;
            }
            else if(curlineno < stmt->lineno)
            {
                curblk = curblk->next;
            }
            else
            {
                link_blk(curblk->prev, newblk, curblk);
                break;
            }
        }
    }
    return 0;
}


STMT_BLK* get_blk_by_lineno(int lineno)
{
    if(lineno < 1) { return NULL;}

    STMT_BLK *curblk = sentinelblk.next;

    while(curblk != NULL)
    {
        if(curblk->isCounter == 1)
        {
            curblk = curblk->next;
            continue;
        }

        if(curblk->stmt == NULL)
        {
            printf("ERR: block has no stmt.");
            return NULL;
        }

        if(curblk->stmt->lineno == lineno)
        {
            return curblk;
        }
        curblk = curblk->next;
    }

    return NULL;
}

STMT_BLK * get_last_blk()
{
    STMT_BLK *curblk = sentinelblk.next;

    if(curblk == NULL)
    {
        return NULL;
    }

    while(curblk->next != NULL)
    {
        curblk = curblk -> next;
    }

    return curblk;
}

/**
 * @brief  Delete statements from the program store.
 * @details  This function deletes from the program store statements whose
 * line numbers fall in a specified range.  Any deleted statements are freed.
 * Deletion of statements preserves the value of the program counter:
 * if before deletion the program counter pointed to a position just before
 * a statement that was not among those to be deleted, then after deletion the
 * program counter will still point the position just before that same statement.
 * If before deletion the program counter pointed to a position just before
 * a statement that was among those to be deleted, then after deletion the
 * program counter will point to the first statement beyond those deleted,
 * if such a statement exists, otherwise the program counter will point to
 * the end of the program.
 *
 * @param min  Lower end of the range of line numbers to be deleted.
 * @param max  Upper end of the range of line numbers to be deleted.
 */
int prog_delete(int min, int max) {

    STMT_BLK *curblk = sentinelblk.next;

    if(curblk == NULL)
    {
        return -1;
    }

    while(curblk != NULL)
    {
        if(curblk->isCounter == 1) // skip counter block
        {
            curblk = curblk->next;
            continue;
        }

        int lineno = curblk->stmt->lineno;

        if(min <= lineno && lineno <= max) // if cur stmt line number is included in the region,
        {
            STMT_BLK *temp = curblk->next;
            if(unlink_blk(curblk) == -1)
            {
                printf("ERR: unlink blk");
                return -1;
            }
            free_stmt(curblk->stmt);
            curblk->stmt = NULL;
            curblk = temp;
        }
        else
        {
            curblk = curblk->next;
        }
    }
    return 0;
}



/**
 * @brief  Reset the program counter to the beginning of the program.
 * @details  This function resets the program counter to point just
 * before the first statement in the program.
 */
void prog_reset(void) {
    if(sentinelblk.next->isCounter == 1)
    {
        return;
    }

    unlink_blk(&counterblk);

    link_blk(&sentinelblk, &counterblk, sentinelblk.next);
}

/**
 * @brief  Fetch the next program statement.
 * @details  This function fetches and returns the first program
 * statement after the current program counter position.  The program
 * counter position is not modified.  The returned pointer should not
 * be used after any subsequent call to prog_delete that deletes the
 * statement from the program store.
 *
 * @return  The first program statement after the current program
 * counter position, if any, otherwise NULL.
 */
STMT *prog_fetch(void) {

    if(counterblk.next == NULL)
    {
        return NULL;
    }
    else
    {
        if(counterblk.next->stmt == NULL)
        {
            printf("ERR: unexpected block exists (program.c:prog_fetch");
            exit(0);
        }
        else
        {
            return counterblk.next->stmt;
        }
    }
}


/**
 * @brief  Advance the program counter to the next existing statement.
 * @details  This function advances the program counter by one statement
 * from its original position and returns the statement just after the
 * new position.  The returned pointer should not be used after any
 * subsequent call to prog_delete that deletes the statement from the
 * program store.
 *
 * @return The first program statement after the new program counter
 * position, if any, otherwise NULL.
 */
STMT *prog_next() {
    if(counterblk.next == NULL)
    {
        return NULL;
    }

    STMT_BLK *next1 = counterblk.next;
    STMT_BLK *next2 = counterblk.next->next;

    unlink_blk(&counterblk);

    if(next2 == NULL)
    {
        link_blk(next1, &counterblk, NULL);
        return NULL;
    }
    else
    {
        link_blk(next1, &counterblk, next2);
        return next2->stmt;
    }
}

/**
 * @brief  Perform a "go to" operation on the program store.
 * @details  This function performs a "go to" operation on the program
 * store, by resetting the program counter to point to the position just
 * before the statement with the specified line number.
 * The statement pointed at by the new program counter is returned.
 * If there is no statement with the specified line number, then no
 * change is made to the program counter and NULL is returned.
 * Any returned statement should only be regarded as valid as long
 * as no calls to prog_delete are made that delete that statement from
 * the program store.
 *
 * @return  The statement having the specified line number, if such a
 * statement exists, otherwise NULL.
 */
STMT *prog_goto(int lineno) {

    STMT_BLK *targetblk = get_blk_by_lineno(lineno);

    if(targetblk == NULL)
    {
        return NULL;
    }

    if(targetblk->stmt == NULL)
    {
        printf("ERR: stmt not exits.");
        return NULL;
    }

    unlink_blk(&counterblk);

    link_blk(targetblk->prev, &counterblk, targetblk); // prevblk --> counterblk --> targetblk

    return targetblk->stmt;
}

int unlink_blk(STMT_BLK *blk)
{
    if(blk == NULL)
    {
        return -1;
    }

    STMT_BLK *prevblk = blk->prev;
    STMT_BLK *nextblk = blk->next;

    if(prevblk == NULL) {return -1;}

    if(nextblk == NULL)
    {
        prevblk -> next = NULL;
    }
    else
    {
        prevblk -> next = nextblk;
        nextblk -> prev = prevblk;
    }

    blk->prev = NULL;
    blk->next = NULL;
    return 0;
}


void link_blk(STMT_BLK *prev, STMT_BLK *cur, STMT_BLK *next)
{
    if(prev == NULL || cur == NULL)
    {
        printf("ERR: link blk");
        return;
    }

    if(next == NULL)
    {
        prev->next = cur;
        cur->prev = prev;
    }
    else
    {
        prev->next = cur;
        cur->prev = prev;

        cur->next = next;
        next-> prev = cur;
    }
}

void clear_program()
{
    STMT_BLK *curblk = sentinelblk.next;

    while(curblk != NULL)
    {
        if(curblk->stmt != NULL)
        {
            free_stmt(curblk->stmt);
            curblk->stmt = NULL;
        }
        curblk = curblk->next;
    }
    free(stmt_blk_storage);
}