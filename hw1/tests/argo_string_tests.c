#include <string.h>
#include "argo_utils.h"

// Read a valid string
Test(argo_read_string_suite, valid_string, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *ch = "\"Hello_world\"";
    ARGO_STRING s;
    FILE *f = fmemopen(ch, strlen(ch), "r");
    int ret = argo_read_string(&s, f);
    fclose(f);

    int exp_ret = 0;
    long s_len = 11;
    char *s_got_content = ARGO_CHAR_to_char(s.content, s_len);
    char *s_exp_content = "Hello_world";

	assert_argo_expected_status(ret, exp_ret, "argo_read_string");
	assert_argo_expected_string(s_got_content, s_exp_content, s.length, s_len);
	exit(MAGIC_EXIT_CODE);
}

// Read a simple escape sequence
Test(argo_read_string_suite, simple_valid_string, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *ch = "\"Hello\\nworld\"";
    ARGO_STRING s;
    FILE *f = fmemopen(ch, strlen(ch), "r");
    int ret = argo_read_string(&s, f);
    fclose(f);

    int exp_ret = 0;
    long s_len = 11;
    char *s_got_content = ARGO_CHAR_to_char(s.content, s_len);
    char *s_exp_content = "Hello\nworld";

	assert_argo_expected_status(ret, exp_ret, "argo_read_string");
	assert_argo_expected_string(s_got_content, s_exp_content, s.length, s_len);
	exit(MAGIC_EXIT_CODE);
}

// Check control characters
Test(argo_read_string_suite, control_character_check, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) { 
    char *ch = "\"Hello\tworld\"";
    ARGO_STRING s;
    FILE *f = fmemopen(ch, strlen(ch), "r");
    int ret = argo_read_string(&s, f);
    fclose(f);

	assert_argo_not_equal(ret, 0, "argo_read_string");
	exit(MAGIC_EXIT_CODE);
}

// Read a valid string of a mixed escape sequence
Test(argo_read_string_suite, valid_escape_sequence, .timeout = 5, .exit_code = MAGIC_EXIT_CODE){
    char *ch = "\"H\\\"e\\\\l\\/l\\bo\\f_\\nwo\\rl\\td\"";
    ARGO_STRING s;
    FILE *f = fmemopen(ch, strlen(ch), "r");
    int ret = argo_read_string(&s, f);
    fclose(f);

    int exp_ret = 0;
    long s_len = 18;
    char *s_got_content = ARGO_CHAR_to_char(s.content, s_len);
    char *s_exp_content = "H\"e\\l/l\bo\f_\nwo\rl\td";

	assert_argo_expected_status(ret, exp_ret, "argo_read_string");
	assert_argo_expected_string(s_got_content, s_exp_content, s.length, s_len);
	exit(MAGIC_EXIT_CODE);
}

// Read a valid string of an escape sequence of hex characters
Test(argo_read_string_suite, valid_escape_hex_sequence, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *ch = "\"\\u0048\\u0065\\u006C\\u006C\\u006F\\u005f\\u0057\\u006f\\u0072\\u006C\\u0064\"";
    ARGO_STRING s;
    FILE *f = fmemopen(ch, strlen(ch), "r");
    int ret = argo_read_string(&s, f);
    fclose(f);

    int exp_ret = 0;
    long s_len = 11;
    char *s_got_content = ARGO_CHAR_to_char(s.content, s_len);
    char *s_exp_content = "Hello_World";

	assert_argo_expected_status(ret, exp_ret, "argo_read_string");
	assert_argo_expected_string(s_got_content, s_exp_content, s.length, s_len);
	exit(MAGIC_EXIT_CODE);
}

// Read a string of an invalid escape sequence
Test(argo_read_string_suite, wrong_escape_sequence, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *ch = "\"Hello wor\\pld";
    ARGO_STRING s;
    FILE *f = fmemopen(ch, strlen(ch), "r");
    int ret = argo_read_string(&s, f);
    fclose(f);

	assert_argo_not_equal(ret, 0, "argo_read_string");
	exit(MAGIC_EXIT_CODE);
}

// Read a string of an invalid hex escape sequence
Test(argo_read_string_suite, wrong_escape_hex_sequence, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *ch = "\"Hello wor\\u123Kld";
    ARGO_STRING s;
    FILE *f = fmemopen(ch, strlen(ch), "r");
    int ret = argo_read_string(&s, f);
    fclose(f);

	assert_argo_not_equal(ret, 0, "argo_read_string");
	exit(MAGIC_EXIT_CODE);
}

// Write a simple string
Test(argo_write_string_suite, simple_string, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    ARGO_STRING s;
    s.capacity = s.length = 0;
    s.content = NULL;
    char *ch1 = "hello_world";
    write_string_content(&s, ch1);

    char ch2[MAX_BUFFER_LEN] = {0};
    FILE *f = fmemopen(&ch2, sizeof(ch2), "w");
    int ret = argo_write_string(&s, f);
    fclose(f);

    char *ch_exp = "\"hello_world\"";
    int exp_ret = 0;
    assert_argo_expected_status(ret, exp_ret, "argo_write_string");
    assert_argo_expected_string(ch2, ch_exp, 0, 0);
    exit(MAGIC_EXIT_CODE);

}

// Write a string with a simple sequence of escape characters
Test(argo_write_string_suite, simple_escape_sequence, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    ARGO_STRING s;
    s.capacity = s.length = 0;
    s.content = NULL;
    char *ch1 = "hello\tworld";
    write_string_content(&s, ch1);

    char ch2[MAX_BUFFER_LEN] = {0};
    FILE *f = fmemopen(&ch2, sizeof(ch2), "w");
    int ret = argo_write_string(&s, f);
    fclose(f);

    char *ch_exp = "\"hello\\tworld\"";
    int exp_ret = 0;
    assert_argo_expected_status(ret, exp_ret, "argo_write_string");
    assert_argo_expected_string(ch2, ch_exp, 0, 0);
    exit(MAGIC_EXIT_CODE);
}

// Write a string of mixed escape characters
Test(argo_write_string_suite, mixed_escape_sequence, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    ARGO_STRING s;
    s.capacity = s.length = 0;
    s.content = NULL;
    char *ch1 = "hel\tl\no_wo\rl\"d";
    write_string_content(&s, ch1);

    char ch2[MAX_BUFFER_LEN] = {0};
    FILE *f = fmemopen(&ch2, sizeof(ch2), "w");
    int ret = argo_write_string(&s, f);
    fclose(f);

    char *ch_exp = "\"hel\\tl\\no_wo\\rl\\\"d\"";
    int exp_ret = 0;
    assert_argo_expected_status(ret, exp_ret, "argo_write_string");
    assert_argo_expected_string(ch2, ch_exp, 0, 0);
    exit(MAGIC_EXIT_CODE);
}

