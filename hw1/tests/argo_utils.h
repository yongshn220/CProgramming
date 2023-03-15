#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>
#include "argo.h"
#include "global.h"

#define MAGIC_EXIT_CODE 101
#define MAX_BUFFER_LEN 100

static char *progname = "bin/argo";

static char* ARGO_CHAR_to_char(ARGO_CHAR *c, int length) {
    char *str = (char *)malloc(sizeof(char) * (length+1));
    ARGO_CHAR *content = c;
    int i = 0;
    while (i <= length) {
        str[i] = *content;
        if (*content == '\0')
            break;
        i++;
        content++;
    }
    return str;
}

static ARGO_CHAR* char_to_ARGO_CHAR(char *c, int length) {
    ARGO_CHAR *str = (ARGO_CHAR *)malloc(sizeof(ARGO_CHAR) * (length+1));
    char *content = c;
    int i = 0;
    while (i <= length) {
        str[i] = *content;
        if (*content == '\0')
            break;
        i++;
        content++;
    }
    return str;
}

static void write_string_content(ARGO_STRING *s, char *ch) {
    while(*ch != '\0'){
        argo_append_char(s, *ch);
        ch++;
    }
}

static void write_file_content(char *ch, FILE *f) {
    fputc('"',f);
    while(*ch != '\0'){
        fputc(*ch,f);
        ch++;
    }
    fputc('"',f);
}

static void assert_validargs_expected_status(int status, int expected)
{
	cr_assert_eq(status, expected,
			"Invalid return for validargs. Got: %d | Expected: %d",
			status, expected);
}

static void assert_validargs_expected_options(int option, int expected)
{
    cr_assert_eq(option, expected, "Invalid options settings. Got: 0x%x | Expected: 0x%x",
			option, expected);
}

static void assert_argo_not_equal(int ret, int exp, char *name)
{
	cr_assert_neq(ret, exp, "Invalid return for %s. Return value should not be: %d | Got: %d",
			name, exp, ret);
}

static void assert_argo_expected_status(int ret, int exp, char *test_name)
{
	cr_assert_eq(ret, exp, "Invalid return for %s. Got: %d | Expected: %d",
			test_name, ret, exp);
}

static void assert_argo_expected_value_basic(ARGO_BASIC ret, ARGO_BASIC exp)
{
    cr_assert_eq(ret, exp, "Invalid value in ARGO_BASIC. Got: %d | Expected: %d",
			ret, exp);
}

static void assert_argo_expected_value_int(int ret, int exp, int valid)
{
    cr_assert_eq(ret, exp, "Invalid value in int_value. Got: %d | Expected: %d",
			ret, exp);
    cr_assert_eq(valid, 1, "Invalid value in valid_int. Got: %d | Expected: %d",
			valid, 1);
}

static void assert_argo_expected_value_float(double ret, double exp, int valid)
{
    cr_assert_eq(ret, exp, "Invalid value in float_value. Got: %f | Expected: %f",
			ret, exp);
    cr_assert_eq(valid, 1, "Invalid value in valid_float. Got: %d | Expected: %d",
			valid, 1);
}

static void assert_argo_expected_value_string(char *s, char *exp, int valid)
{
	cr_assert_eq(strcmp(s, exp), 0, "Invalid value in string_value. Expected: %s",
			exp);
    cr_assert_eq(valid, 1, "Invalid value in valid_string. Got: %d | Expected: %d",
			valid, 1);
}

static void assert_argo_expected_string(char *s, char *exp, long len, long exp_len)
{
    cr_assert_eq(strcmp(s, exp), 0, "Invalid string content.");
    cr_assert_eq(len, exp_len, "Invalid string length. Got: %ld | Expected: %ld",
                 len, exp_len);
}

static void assert_argo_expected_argo_char(ARGO_CHAR *s, ARGO_CHAR *exp, long len, long exp_len)
{
    cr_assert_eq(len, exp_len, "Invalid string length. Got: %ld | Expected: %ld",
                 len, exp_len);
    for (int i = 0; i < len; i++) {
        cr_assert_eq(s[i], exp[i], "Invalid ARGO_CHAR stored in string. Got: 0x%x | Expected: 0x%x",
                s[i], exp[i]);
    }
}

static void assert_argo_value_type(ARGO_VALUE_TYPE type, ARGO_VALUE_TYPE exp)
{
    cr_assert_eq(type, exp, "Invalid ARGO_VALUE_TYPE. Got: %d | Expected: %d",
            type, exp);
}

static void assert_argo_value_not_null(ARGO_VALUE *val)
{
    cr_assert_not_null(val, "Return value for argo_read_value() should not be null. Test failed.");
}

static void assert_argo_list_empty(ARGO_VALUE *head, char *name)
{
    cr_assert_eq(head, head->next, "%s is empty, next pointer should point to the sentinel node.", name);
    cr_assert_eq(head, head->prev, "%s is empty, prev pointer should point to the sentinel node.", name);
}

static void assert_normal_exit(int status)
{
    cr_assert_eq(status, EXIT_SUCCESS,
                 "Program exited with 0x%x instead of EXIT_SUCCESS",
				 status);
}

static void assert_outfile_matches(int status)
{
    cr_assert_eq(status, EXIT_SUCCESS,
                 "Program output did not match reference output.");
}
