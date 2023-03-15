#include <stdlib.h>
#include <stdio.h>

#include "argo.h"
#include "global.h"
#include "debug.h"




//CUSTUM FUNCTION (read)

int is_error_msg_printed = -1;
int brace_count = 0;
int brack_count = 0;

int create_argo_value();
int hexchar_to_int(int c);
int char_converter(int c, FILE *f);

int get_argo_object(ARGO_OBJECT *o, FILE *f);
int get_argo_array(ARGO_ARRAY *a, FILE *f);
int argo_read_basic(ARGO_BASIC *b, FILE *f);

struct argo_value *get_dummy_value(FILE *f);

int set_value_name(ARGO_VALUE *v, FILE *f);
int set_value_content(ARGO_VALUE *v, FILE *f);
int set_argo_string(ARGO_STRING *s, FILE *f);

int is_content_empty(FILE *f);

char get_next_input(FILE *f);
double argo_pow(int a, int b);

void print_error_message(int type);


//CUSTUM FUNCTION (write)

int parentType = 0;
char flagType;
int indent;

void print_indent();
void startSetting();

void try_move_to_next_value(ARGO_VALUE *v, FILE *f);
void try_write_value_name(ARGO_VALUE *v, FILE *f);

void write_value_content_basic(ARGO_VALUE *v, FILE *f);
void write_value_content_string(ARGO_VALUE *v, FILE *f);
void write_value_content_number(ARGO_VALUE *v, FILE *f);

void call_value_content_object(ARGO_VALUE *v, FILE *f);
void call_value_content_array(ARGO_VALUE *v, FILE *f);
void call_value_next(ARGO_VALUE *v, FILE *f);

int is_top_most_value(ARGO_VALUE *v);
int is_value_name_null(ARGO_VALUE *v);
int is_value_next_sentinel(ARGO_VALUE *v);

void print_char(int c);
void print_sign(char c);

int num_canonicalize(double num);

/**
 * @brief  Read JSON input from a specified input stream, parse it,
 * and return a data structure representing the corresponding value.
 * @details  This function reads a sequence of 8-bit bytes from
 * a specified input stream and attempts to parse it as a JSON value,
 * according to the JSON syntax standard.  If the input can be
 * successfully parsed, then a pointer to a data structure representing
 * the corresponding value is returned.  See the assignment handout for
 * information on the JSON syntax standard and how parsing can be
 * accomplished.  As discussed in the assignment handout, the returned
 * pointer must be to one of the elements of the argo_value_storage
 * array that is defined in the const.h header file.
 * In case of an error (these include failure of the input to conform
 * to the JSON standard, premature EOF on the input stream, as well as
 * other I/O errors), a one-line error message is output to standard error
 * and a NULL pointer value is returned.
 *
 * @param f  Input stream from which JSON is to be read.
 * @return  A valid pointer if the operation is completely successful,
 * NULL if there is any error.
 */


int create_argo_value()
{
    if(NUM_ARGO_VALUES <= argo_next_value)
    {
        print_error_message(8);
        return -1;
    }
    else
    {
        ARGO_VALUE arVal = { .type = 0, .next = NULL, .prev = NULL};
        argo_value_storage[argo_next_value] = arVal;

        argo_next_value++;
        return 0;
    }
}

ARGO_VALUE *argo_read_value(FILE *f) {

    if(is_content_empty(f) == 0)
    {
        return NULL;
    }

    if(brace_count != 0 || brack_count != 0)
    {
        int c = get_next_input(f);

        if(c == ARGO_RBRACE || c == ARGO_RBRACK)
        {
            print_error_message(100);
            return NULL;
        }
        ungetc(c, f);
    }

    if(create_argo_value() != 0) {return NULL;}

    ARGO_VALUE *arVal = &argo_value_storage[argo_next_value - 1];

    if(parentType == 4) // 4 : object
    {
        if(set_value_name(arVal, f) != 0) {return NULL;}
        int c = get_next_input(f);
        if(c != ARGO_COLON) {return NULL;}
    }

    if(set_value_content(arVal, f) != 0) {return NULL;}
    return arVal;
}


/**
 * @brief  Read JSON input from a specified input stream, attempt to
 * parse it as a JSON string literal, and return a data structure
 * representing the corresponding string.
 * @details  This function reads a sequence of 8-bit bytes from
 * a specified input stream and attempts to parse it as a JSON string
 * literal, according to the JSON syntax standard.  If the input can be
 * successfully parsed, then a pointer to a data structure representing
 * the corresponding value is returned.
 * In case of an error (these include failure of the input to conform
 * to the JSON standard, premature EOF on the input stream, as well as
 * other I/O errors), a one-line error message is output to standard error
 * and a NULL pointer value is returned.
 *
 * @param f  Input stream from which JSON is to be read.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_read_string(ARGO_STRING *s, FILE *f) {
    int c = fgetc(f); //char after "
    while(c != ARGO_QUOTE)
    {
        int cInt = char_converter(c, f);

        if(argo_append_char(s, cInt) != 0)
        {
            return -1;
        }
        c = fgetc(f);
    }
    return 0;
}

int hexchar_to_int(int c)
{
    if(argo_is_digit(c))
    {
        return c - '0';
    }
    else if(c >= 'a' || c <= 'z')
    {
        return c - 'a' + 10;
    }
    else if(c >= 'A' || c <= 'Z')
    {
        return c - 'A' + 10;
    }
    else
    {
        return -1;
    }
}

int char_converter(int ch, FILE *f)
{
    int hex;
    if(ch == ARGO_SPACE)
    {
        return ch;
    }
    else if(ch == '\\')
    {
        int c = fgetc(f);
        if(c == ARGO_U)
        {
            hex = 0;
            for(int i = 0; i < 4; i++)
            {
                c = fgetc(f);
                if(argo_is_hex(c))
                {
                    int hchar = hexchar_to_int(c);
                    hex = hex * 0x10 + hchar;
                }
                else
                {
                    return -1;
                }
            }
        }
        else if(c == ARGO_FSLASH)
        {
            hex = ARGO_FSLASH;
        }
        else if(c == ARGO_BSLASH)
        {
            hex = ARGO_BSLASH;
        }
        else if(c == ARGO_QUOTE)
        {
            hex = ARGO_QUOTE;
        }
        else if(c == ARGO_B)
        {
            hex = ARGO_BS;
        }
        else if(c == ARGO_F)
        {
            hex = ARGO_FF;
        }
        else if(c == ARGO_N)
        {
            hex = ARGO_LF;
        }
        else if(c == ARGO_R)
        {
            hex = ARGO_CR;
        }
        else if(c == ARGO_T)
        {
            hex = ARGO_HT;
        }
        return hex;
    }
    else
    {
        hex = ch;
        return hex;
    }
}

/**
 * @brief  Read JSON input from a specified input stream, attempt to
 * parse it as a JSON number, and return a data structure representing
 * the corresponding number.
 * @details  This function reads a sequence of 8-bit bytes from
 * a specified input stream and attempts to parse it as a JSON numeric
 * literal, according to the JSON syntax standard.  If the input can be
 * successfully parsed, then a pointer to a data structure representing
 * the corresponding value is returned.  The returned value must contain
 * (1) a string consisting of the actual sequence of characters read from
 * the input stream; (2) a floating point representation of the corresponding
 * value; and (3) an integer representation of the corresponding value,
 * in case the input literal did not contain any fraction or exponent parts.
 * In case of an error (these include failure of the input to conform
 * to the JSON standard, premature EOF on the input stream, as well as
 * other I/O errors), a one-line error message is output to standard error
 * and a NULL pointer value is returned.
 *
 * @param f  Input stream from which JSON is to be read.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_read_number(ARGO_NUMBER *n, FILE *f) {
    int c = fgetc(f);

    double result = 0;
    float deci_point = -1; // nth decimal point. -1: no decimal point
    int exp = -1;  // n power of. -1 = no exponential.
    int sign = 1; // sign of number
    int exp_sign = 1; // exp sign.

    int read_cnt = 0; // count of while loop


    while(c != ARGO_COMMA && c != ARGO_RBRACK && c != ARGO_RBRACE)
    {
        read_cnt++;
        if(c == ARGO_MINUS)
        {
            if(exp != -1) // sign of exponent
            {
                exp_sign = -1;
            }
            else if(read_cnt != 1)
            {
                return -1; // minus is not exponent and is not comes at first(ex '1-45.0')
            }
            else // sign of whole number
            {
                sign = -1;
            }
        }
        else if(c == ARGO_PERIOD)
        {
            if(deci_point > 0 || read_cnt == 1)
            {
                return -1; // period came out two times. or came out at first position.
            }
            else
            {
                deci_point = 1;
            }
        }
        else if(argo_is_exponent(c))
        {
            if(exp != -1 || read_cnt == 1)
            {
                return -1; // exponent came out two times. or came out at first position.
            }
            else
            {
                exp = 0;
            }
        }
        else if(argo_is_digit(c))
        {
            c = c - '0'; // char to number;

            if(exp != -1)
            {
                exp = exp*10 + c;
            }
            else if(deci_point != -1) // number after decimal point
            {
                deci_point = deci_point * 0.1;
                result = result + (deci_point * c);
            }
            else // number before decimal point.
            {
                result = (result * 10) + c;
            }
        }
        else if(argo_is_whitespace(c))
        {
            // do nothing.
        }
        else
        {
            print_error_message(7);
            return -1;
        }
        c = fgetc(f);
    }

    ungetc(c, f); // go back one step. so later it can whether keep going or not by (']', '}', ',')

    if(deci_point == -1 && exp == -1)
    {
        if(read_cnt > 10)
        {
            return -1;
        }
        result = result * sign;
        (*n).int_value = result;
        (*n).valid_int = 1;
        return 0;
    }
    else
    {
        double exponent_value;
        if(exp != -1)
        {
            exponent_value = argo_pow(10, (exp * exp_sign));
        }
        else
        {
            exponent_value = 1;
        }
        result =  result * sign * exponent_value;
        (*n).float_value = result;
        (*n).valid_float = 1;
        return 0;
    }
}


double argo_pow(int a, int b)
{
    int sign = 1;
    if(b < 0)
    {
        b *= -1;
        sign = -1;
    }

    double res = 1;

    for(int i = 0; i < b; i++)
    {
        res *= a;
    }

    if(sign == 1)
    {
        return res;
    }
    return 1 / res;
}

int set_value_name(ARGO_VALUE *v, FILE *f)
{
    char c = get_next_input(f);

    if(c == ARGO_QUOTE)
    {
        ARGO_STRING arStr = {};
        if(argo_read_string(&arStr, f) != 0)
        {
            print_error_message(0);
            return -1;
        }
        (*v).name = arStr;
        return 0;
    }
    printf("ERROR: you might miss 'Quote' \n");
    return -1;
}

int set_value_content(ARGO_VALUE *v, FILE *f)
{
    int c = get_next_input(f); // valid if (c == Quote, LBrace, LBrack, minus, digit, t, f, n) otherwise, invalid.

    if(parentType == 0)
    {
        if(c != ARGO_LBRACE && c != ARGO_LBRACK)
        {
            print_error_message(7);
            return -1;
        }
    }

    // content: number
    if(argo_is_digit(c) || c == ARGO_MINUS)
    {
        ungetc(c, f); // one step back so that read_value_function can read from the first.

        ARGO_NUMBER arNum = {};
        if(argo_read_number(&arNum, f) != 0)
        {
            print_error_message(2);
            return -1;
        }

        (*v).content.number = arNum;

        (*v).type = ARGO_NUMBER_TYPE; // set type
    }

    //content: string
    else if(c == ARGO_QUOTE)
    {
        ARGO_STRING arStr = {};
        if(argo_read_string(&arStr, f) != 0)
        {
            print_error_message(3);
            return -1;
        }

        (*v).content.string = arStr;

        (*v).type = ARGO_STRING_TYPE; // set type
    }

    //content: object
    else if(c == ARGO_LBRACE)
    {

        ARGO_OBJECT arObj = {};

        if(get_argo_object(&arObj, f) != 0)
        {
            print_error_message(4);
            return -1;
        }

        (*v).content.object = arObj;
        (*v).type = ARGO_OBJECT_TYPE; // set type
    }

    //content: array
    else if(c == ARGO_LBRACK)
    {
        ARGO_ARRAY arArr = {};

        if(get_argo_array(&arArr, f) != 0)
        {
            print_error_message(5);
            return -1;
        }

        (*v).content.array = arArr;
        (*v).type = ARGO_ARRAY_TYPE; // set type
    }

    else if(c == ARGO_T || c == ARGO_F || c == ARGO_N)
    {

        ungetc(c, f); // go back 1 step, so it can read from 't'rue or 'f'alse or 'n'ull

        ARGO_BASIC arBsc = 0;

        if(argo_read_basic(&arBsc, f) != 0)
        {
            print_error_message(1);
            return -1;
        }

        (*v).content.basic = arBsc;
        (*v).type = ARGO_BASIC_TYPE;
    }
    else
    {
        print_error_message(6);
        return -1;
    }

    if(brace_count == 0 && brack_count == 0)
    {
        c = get_next_input(f);
        if(c != EOF)
        {
            print_error_message(100);
            return -1;
        }
    }

    return 0;
}

int argo_read_basic(ARGO_BASIC *b, FILE *f)
{
    int c = fgetc(f);
    if(c == 't')
    {
        c = fgetc(f);
        if(c == 'r')
        {
            c = fgetc(f);
            if(c == 'u')
            {
                c = fgetc(f);
                if(c == 'e')
                {
                    (*b) = ARGO_TRUE;
                    return 0;
                }
            }
        }
    }
    else if(c == 'f')
    {
        c = fgetc(f);
        if(c == 'a')
        {
            c = fgetc(f);
            if(c == 'l')
            {
                c = fgetc(f);
                if(c == 's')
                {
                    c = fgetc(f);
                    if(c == 'e')
                    {
                        (*b) = ARGO_FALSE;
                        return 0;
                    }
                }
            }
        }
    }
    else if(c == 'n')
    {
        c = fgetc(f);
        if(c == 'u')
        {
            c = fgetc(f);
            if(c == 'l')
            {
                c = fgetc(f);
                if(c == 'l')
                {
                    (*b) = ARGO_NULL;
                    return 0;
                }
            }
        }
    }
    return -1;
}

int get_argo_object(ARGO_OBJECT *o, FILE *f)
{
    int prev_parent_type = parentType;
    brace_count++;

    parentType = 4;
    ARGO_VALUE *arVal;
    arVal = get_dummy_value(f);
    if(arVal == NULL) {return -1;}
    (*o).member_list = arVal;

    parentType = prev_parent_type;

    brace_count--;
    return 0;
}

int get_argo_array(ARGO_ARRAY *a, FILE *f)
{
    int prev_parent_type = parentType;
    brack_count++;

    parentType = 5;
    ARGO_VALUE *arVal;
    arVal = get_dummy_value(f);
    if(arVal == NULL) {return -1;}
    (*a).element_list = arVal;
    parentType = prev_parent_type;

    brack_count--;
    return 0;
}

struct argo_value *get_dummy_value(FILE *f)
{
    create_argo_value();
    ARGO_VALUE *dum_arVal = &argo_value_storage[argo_next_value - 1];

    ARGO_VALUE *cur_arVal = dum_arVal;



    if(is_content_empty(f) == 0)
    {
        (*dum_arVal).next = dum_arVal;  // if value == [] or {}
        (*dum_arVal).prev = dum_arVal;  // set next & prev can pointing self.
        return dum_arVal;               // if next_arVal is NULL because of empty value.
    }

    int c;

    do
    {
        ARGO_VALUE *next_arVal = argo_read_value(f);

        if(next_arVal == NULL)
        {
            return NULL;  // if next_arVal is NULL because of Error,
        }
        (*cur_arVal).next = next_arVal;
        (*next_arVal).prev = cur_arVal;
        cur_arVal = next_arVal;

        c = get_next_input(f);

    }while(c == ARGO_COMMA);  //continue if there is more element.

    if(c != ARGO_RBRACE && c != ARGO_RBRACK)
    {
        print_error_message(6);
        return NULL; // invalid input or comma missed.
    }

    (*cur_arVal).next = dum_arVal; // set link cur_val  to dummy
    (*dum_arVal).prev = cur_arVal;
    return dum_arVal;
}


int set_argo_string(ARGO_STRING *s, FILE *f)
{
    if(argo_read_string(s, f) == 0)
    {
        return 0;
    }
    return -1;
}

char get_next_input(FILE *f)
{
    int c = fgetc(f);
    while(argo_is_whitespace(c))
    {
        c = fgetc(f);
    }
    return c;
}

int is_content_empty(FILE *f)
{
    char c = get_next_input(f);

    if(c == ARGO_RBRACE || c == ARGO_RBRACK)
    {
        return 0;
    }
    ungetc(c, f);
    return -1;
}

void print_error_message(int type)
{
    if(is_error_msg_printed == -1)
    {
        if(type == 0)
        {
            printf("ERROR: please check the 'name' field again.\n");
        }
        else if(type == 1)
        {
            printf("ERROR: please check basic type again.\n");
        }
        else if(type == 2)
        {
            printf("ERROR: please check numeric type.\n");
        }
        else if(type == 3)
        {
            printf("ERROR: please check string type.\n");
        }
        else if(type == 4)
        {
            printf("ERROR: please check {}.\n");
        }
        else if(type == 5)
        {
            printf("ERROR: please check [].\n");
        }
        else if(type == 6)
        {
            printf("ERROR: there is a invalid input or miss comma.\n");
        }
        else if(type == 7)
        {
            printf("ERROR: you might miss [] or {}\n");
        }
        else if(type == 8)
        {
            printf("ERROR: you might exceed argo_value_storage.\n");
        }
        else
        {
            printf("ERROR: you have invalid input.\n");
        }
        is_error_msg_printed = 0;
    }
}



/**
 * @brief  Write canonical JSON representing a specified value to
 * a specified output stream.
 * @details  Write canonical JSON representing a specified value
 * to specified output stream.  See the assignment document for a
 * detailed discussion of the data structure and what is meant by
 * canonical JSON.
 *
 * @param v  Data structure representing a value.
 * @param f  Output stream to which JSON is to be written.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */

int argo_write_value(ARGO_VALUE *v, FILE *f) {

    // VALUE TYPE =>> "No Type"
    if((*v).type  == 0)
    {
        if((*v).next == v && (*v).prev == v)
        {
            return 0;
        }

        call_value_next(v, f);

        return 0;
    }

    // VALUE TYPE =>> "Basic"
    else if((*v).type == 1)
    {

        try_write_value_name(v, f);

        write_value_content_basic(v, f); // wrtie content(basic)

        try_move_to_next_value(v, f); // move to next value if exists.

        return 0;
    }

    // VALUE TYPE =>> "Number"
    else if((*v).type == 2)
    {
        try_write_value_name(v, f);

        write_value_content_number(v, f); // write content(number)

        try_move_to_next_value(v, f); // move to next value if exists.

        return 0;
    }

    // VALUE TYPE =>> "String"
    else if((*v).type == 3)
    {
        try_write_value_name(v, f);

        write_value_content_string(v, f);

        try_move_to_next_value(v, f); // move to next value if exists.

        return 0;
    }

    // VALUE TYPE =>> "Object"
    else if((*v).type == 4)
    {
        if(is_top_most_value(v) == 0)
        {
            parentType = 4;
            print_sign(ARGO_LBRACE); // "{"

            call_value_content_object(v, f);

            print_sign(ARGO_RBRACE); // "}"

            if(flagType == 'p')
            {
                printf("\n");
            }
            return 0;
        }

        try_write_value_name(v, f);

        print_sign(ARGO_LBRACE); // "{"

        call_value_content_object(v, f);

        print_sign(ARGO_RBRACE); // "}"

        try_move_to_next_value(v, f); // move to next value if exists.

        return 0;
    }
    // type == ARRAY
    else if((*v).type == 5)
    {
        if(is_top_most_value(v) == 0)
        {
            parentType = 5;
            print_sign(ARGO_LBRACK); // "["

            call_value_content_array(v, f);

            print_sign(ARGO_RBRACK); // "]"

            if(flagType == 'p')
            {
                printf("\n");
            }

            return 0;
        }

        try_write_value_name(v, f);

        print_sign(ARGO_LBRACK); // "["

        call_value_content_array(v, f);

        print_sign(ARGO_RBRACK); // "]"

        try_move_to_next_value(v, f); // move to next value if exists.

        return 0;
    }
    return 0;
}


/**
 * @brief  Write canonical JSON representing a specified string
 * to a specified output stream.
 * @details  Write canonical JSON representing a specified string
 * to specified output stream.  See the assignment document for a
 * detailed discussion of the data structure and what is meant by
 * canonical JSON.  The argument string may contain any sequence of
 * Unicode code points and the output is a JSON string literal,
 * represented using only 8-bit bytes.  Therefore, any Unicode code
 * with a value greater than or equal to U+00FF cannot appear directly
 * in the output and must be represented by an escape sequence.
 * There are other requirements on the use of escape sequences;
 * see the assignment handout for details.
 *
 * @param v  Data structure representing a string (a sequence of
 * Unicode code points).
 * @param f  Output stream to which JSON is to be written.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_write_string(ARGO_STRING *s, FILE *f) {
    // TO BE IMPLEMENTED.
    if(parentType == 5)
    {
        print_indent();
    }
    print_sign(ARGO_QUOTE);
    for(int i = 0; i < (*s).length; i++)
    {
        print_char(*((*s).content+i));
    }
    print_sign(ARGO_QUOTE);
    return 0;
}

/**
 * @brief  Write canonical JSON representing a specified number
 * to a specified output stream.
 * @details  Write canonical JSON representing a specified number
 * to specified output stream.  See the assignment document for a
 * detailed discussion of the data structure and what is meant by
 * canonical JSON.  The argument number may contain representations
 * of the number as any or all of: string conforming to the
 * specification for a JSON number (but not necessarily canonical),
 * integer value, or floating point value.  This function should
 * be able to work properly regardless of which subset of these
 * representations is present.
 *
 * @param v  Data structure representing a number.
 * @param f  Output stream to which JSON is to be written.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_write_number(ARGO_NUMBER *n, FILE *f) {
    // TO BE IMPLEMENTED.

    if((*n).valid_int != 0)
    {
        printf("%ld",(*n).int_value);
        return 0;
    }
    if((*n).valid_float != 0)
    {
        if(num_canonicalize((*n).float_value) == -1) {return -1;}
        return 0;
    }
    return -1;
}


int num_canonicalize(double num)
{
    if(num == 0.0)
    {
        printf("0.0");
        return 0;
    }

    int expCnt = 0;
    int sign = 1;
    if(num < 0)
    {
        sign = -1; //keep the sign
        num = num * -1; //  make it |num|
    }

    if(num >= 1.0)
    {
        while(num >= 1.0)
        {
            num = num / 10;  // make xx.xxx -> 0.xxxxx
            expCnt++;        // count exp
        }
        printf("%.9ge%d",(num*sign),expCnt);
        return 0;
    }
    if(num < 1.0)
    {
        while(num < 1.0)
        {
            num = num * 10; // until  0.000xxx -> x.xxx
            expCnt++;
        }
        num = num / 10; //   make x.xxx -> 0.xxxx
        expCnt--;

        if(expCnt == 0) // if exponent is zero  ex) xxe0
        {
            printf("%.9g", num*sign); // print w/o 'e' sign.
        }
        else
        {
            printf("%.9ge-%d", num*sign, expCnt);
        }
        return 0;

    }
    return -1;
}



void try_move_to_next_value(ARGO_VALUE *v, FILE *f)
{
    if(is_value_next_sentinel(v) == -1)
    {
        print_sign(ARGO_COMMA);

        call_value_next(v, f);
    }
}


void try_write_value_name(ARGO_VALUE *v, FILE *f)
{
    if(parentType == 4) // if this is a member of object.
    {
        print_indent();
        ARGO_STRING *arStr;
        arStr = &(*v).name;
        //printf("len:%zu", (*arStr).length);
        argo_write_string(arStr, f);
        print_sign(ARGO_COLON);
    }
}

void write_value_content_basic(ARGO_VALUE *v, FILE *f)
{
    ARGO_BASIC *arBsc;
    arBsc = &(*v).content.basic;

    if(*arBsc == ARGO_NULL)
    {
        if(parentType == 5)
        {
            print_indent();
        }
        printf(ARGO_NULL_TOKEN);
    }
    else if(*arBsc == ARGO_TRUE)
    {
        if(parentType == 5)
        {
            print_indent();
        }
        printf(ARGO_TRUE_TOKEN);
    }
    else if(*arBsc == ARGO_FALSE)
    {
        if(parentType == 5)
        {
            print_indent();
        }
        printf(ARGO_FALSE_TOKEN);
    }
}

void write_value_content_string(ARGO_VALUE *v, FILE *f)
{
    ARGO_STRING *arStr;
    arStr = &(*v).content.string;
    argo_write_string(arStr, f);
}

void write_value_content_number(ARGO_VALUE *v, FILE *f)
{
    ARGO_NUMBER *arNum;
    arNum = &(*v).content.number;

    if(parentType == 5)
    {
        print_indent();
    }
    argo_write_number(arNum, f);
}

void call_value_content_object(ARGO_VALUE *v, FILE *f)
{
    int tempParentType = parentType;
    ARGO_VALUE *arVal;
    arVal = &(*((*v).content.object.member_list));
    parentType = 4; // set parentType as current Type;
    argo_write_value(arVal, f);
    parentType = tempParentType; // set parentType back.
}

void call_value_content_array(ARGO_VALUE *v, FILE *f)
{
    int tempParentType = parentType;
    ARGO_VALUE *arVal;
    arVal = &(*((*v).content.array.element_list));
    parentType = 5; // set parentType as current Type;
    argo_write_value(arVal, f);
    parentType = tempParentType; // set parentType back.
}

void call_value_next(ARGO_VALUE *v, FILE *f)
{
    argo_write_value((*v).next, f);
}

int is_top_most_value(ARGO_VALUE *v)
{
    if((*v).next == NULL && (*v).prev == NULL)
    {
        startSetting();
        return 0;
    }
    return -1;
}

int is_value_name_null(ARGO_VALUE *v)
{
    // if name is null
    if((*v).name.content == NULL)
    {
        return 0;
    }
    return -1;
}

int is_value_next_sentinel(ARGO_VALUE *v)
{

    if((*((*v).next)).type == 0)
    {
        // if this is pretty print mode
        if(flagType == 'p')
        {
            printf("\n");
        }
        return 0;
    }
    return -1;
}

void print_sign(char c)
{
    if(flagType == 'p')
    {
        if(c == ARGO_COLON)
        {
            printf("%c", c);
            printf(" ");
        }
        else if(c == ARGO_COMMA)
        {
            printf("%c", c);
            printf("\n");
        }
        else if(c == ARGO_LBRACE || c == ARGO_LBRACK)
        {
            if(parentType == 5)
            {
                print_indent();
            }
            indent_level += 1;
            printf("%c", c);
            printf("\n");
        }
        else if(c == ARGO_RBRACE || c == ARGO_RBRACK)
        {
            indent_level -= 1;
            print_indent();
            printf("%c", c);
        }
        else
        {
            printf("%c", c);
        }
    }
    else
    {
        printf("%c", c);
    }
}

void print_char(int c)
{
    if(c == ARGO_BS)
    {
        printf("\\b");
    }
    else if(c == ARGO_FF)
    {
        printf("\\f");
    }
    else if(c == ARGO_LF)
    {
        printf("\\n");
    }
    else if(c == ARGO_CR)
    {
        printf("\\r");
    }
    else if(c == ARGO_HT)
    {
        printf("\\t");
    }
    else if(c == ARGO_BSLASH)
    {
        printf("\\\\");
    }
    else if(c == ARGO_QUOTE)
    {
        printf("\\\"");
    }
    else if(argo_is_control(c) || c > 0xFF)
    {
        int hex = 0x1000;
        printf("\\u");
        while(c < hex && hex >= 0x10)
        {
            printf("0");
            hex /= 0x10;
        }
        printf("%x", c);
    }
    else
    {
        printf("%c", c);
    }
}

void print_indent()
{
    for(int i = 0; i < indent_level; i++)
    {
        for(int j = 0; j < indent; j++)
        {
            printf(" ");
        }
    }
}

void startSetting()
{
    parentType = 0;
    if(global_options == VALIDATE_OPTION)
    {
        flagType = 'v';
    }
    else if(global_options == CANONICALIZE_OPTION)
    {
        flagType = 'c';
    }
    else
    {
        flagType = 'p';
        indent_level = 0;
        indent = global_options - (CANONICALIZE_OPTION + PRETTY_PRINT_OPTION);
    }
}