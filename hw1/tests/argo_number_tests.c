#include "argo_utils.h"
#include <math.h>

// Read a valid integer
Test(argo_read_number_suite, valid_int, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    long number = 567890;
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer, "%ld", number);
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_int(v.int_value, number, v.valid_int);
	exit(MAGIC_EXIT_CODE);
}

// Read a negative int
Test(argo_read_number_suite, negative_int, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    long number = -3478913;
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer, "%ld", number);
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_int(v.int_value, number, v.valid_int);
	exit(MAGIC_EXIT_CODE);
}

// Read a number with plus sign
Test(argo_read_number_suite, plus_sign, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer, "+53420904");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

	assert_argo_not_equal(ret, 0, "argo_read_number");
	exit(MAGIC_EXIT_CODE);
}

// Read a valid floating point value 
Test(argo_read_number_suite, valid_float, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    double number = 9.345217;
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer, "%f", number);
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_float(v.float_value, number, v.valid_float);
	exit(MAGIC_EXIT_CODE);
}

// Floating point for exact 0
Test(argo_read_number_suite, float_exact_zero, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    double number = 0.000;
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer, "%f", number);
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_float(v.float_value, number, v.valid_float);
	exit(MAGIC_EXIT_CODE);
}

// Floating point for exact 1
Test(argo_read_number_suite, float_exact_one, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    double number = 1.000;
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer, "%f", number);
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_float(v.float_value, number, v.valid_float);
	exit(MAGIC_EXIT_CODE);
}

// Allow for small exponent symbol
Test(argo_read_number_suite, small_exponent_symbol, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};
    double number = 5436;

    sprintf(buffer, "543.6e1");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_float(v.float_value, number, v.valid_float);
	exit(MAGIC_EXIT_CODE);
}

// Allow for capital exponent symbol
Test(argo_read_number_suite, capital_exponent_symbol, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};
    double number = 34000000;

    sprintf(buffer, "34E6");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_float(v.float_value, number, v.valid_float);
	exit(MAGIC_EXIT_CODE);
}

// Negative exponent symbol for a negative number
Test(argo_read_number_suite, negative_exponent_negative_number, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};
    double number = -890266;

    sprintf(buffer, "-89026600E-2");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_float(v.float_value, number, v.valid_float);
	exit(MAGIC_EXIT_CODE);
}

// Read a valid string value
Test(argo_read_number_suite, valid_string, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer, "32567.31e3");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    char *s_got_content = ARGO_CHAR_to_char(v.string_value.content, strlen(buffer)-1);
    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_string(s_got_content, buffer, v.valid_float);
	exit(MAGIC_EXIT_CODE);
}

// Read an invalid input
Test(argo_read_number_suite, invalid_input_1, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer, "badinput");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

	assert_argo_not_equal(ret, 0, "argo_read_number");
	exit(MAGIC_EXIT_CODE);
}

// Read valid number 1e3e4 (not a valid JSON input though) 
Test(argo_read_number_suite, invalid_input_2, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};
    double number = 1000;

    sprintf(buffer, "1e3e4");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_read_number");
	assert_argo_expected_value_float(v.float_value, number, v.valid_float);
	exit(MAGIC_EXIT_CODE);
}

// Read an invalid input
Test(argo_read_number_suite, invalid_input_3, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer, "10..10");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    int ret = argo_read_number(&v, f);
    fclose(f);

	assert_argo_not_equal(ret, 0, "argo_read_number");
	exit(MAGIC_EXIT_CODE);
}

// Read a valid integer
Test(argo_write_number_suite, valid_int, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    long number = -3478913;
    char buffer1[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    sprintf(buffer1, "%ld", number);
    v.valid_int = 1;
    v.int_value = number;

    char buffer2[MAX_BUFFER_LEN] = {0};
    FILE *f = fmemopen(&buffer2, sizeof(buffer2), "w");
    int ret = argo_write_number(&v, f);
    fclose(f);

    int exp_ret = 0;
	assert_argo_expected_status(ret, exp_ret, "argo_write_number");
	assert_argo_expected_value_string(buffer1, buffer2, 1);
	exit(MAGIC_EXIT_CODE);
}

// Write a valid floating point number
Test(argo_write_number_suite, valid_float, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    double number1 = 3.14567e1;
    char buffer1[MAX_BUFFER_LEN] = {0};
    ARGO_NUMBER v = {0};

    v.valid_float = 1;
    v.float_value = number1;

    char buffer2[MAX_BUFFER_LEN] = {0};
    FILE *f = fmemopen(&buffer2, sizeof(buffer2), "w");
    int ret = argo_write_number(&v, f);
    fclose(f);

    double number2;
    int exp_ret = 0;

    sscanf(buffer2, "%lf", &number2);
	assert_argo_expected_status(ret, exp_ret, "argo_write_number");
	//check if the absolute value of the difference is greater than 0.5
    cr_assert_eq((fabs(number1 - number2) > 0.5), 0, "argo_write_number returned invalid value . Got: %s | Expected: %s", buffer2, buffer1);
	exit(MAGIC_EXIT_CODE);
}

// Exceeds max digits, test whether there is a crash or not
Test(argo_write_number_suite, exceed_max_digits, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    long long number = 349999978913;
    ARGO_NUMBER v = {0};

    v.valid_int = 1;
    v.int_value = number;

    char buffer[MAX_BUFFER_LEN] = {0};
    FILE *f = fmemopen(&buffer, sizeof(buffer), "w");
    int ret = argo_write_number(&v, f);
    fclose(f);

	exit(MAGIC_EXIT_CODE);
}
