#include "argo_utils.h"

// Check value of global_options
// If there are errors, global_options = 0

Test(validargs_suite, no_args_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
	int exp_ret = -1;
	assert_validargs_expected_status(ret, exp_ret);
	exit(MAGIC_EXIT_CODE);
}

Test(validargs_suite, no_dash_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "h", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
	int exp_ret = -1;
	assert_validargs_expected_status(ret, exp_ret);
	exit(MAGIC_EXIT_CODE);
}

Test(validargs_suite, ignore_args_after_help_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "-h", "-x", "-y", "-z", "-w", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
	int exp_ret = 0;
	int opt = global_options;
	int exp_opt = HELP_OPTION;
	assert_validargs_expected_status(ret, exp_ret);
	assert_validargs_expected_options(opt, exp_opt);
	exit(MAGIC_EXIT_CODE);
}

Test(validargs_suite, optional_before_positional_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "-p", "12", "-c", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
	assert_validargs_expected_status(ret, exp_ret);
	exit(MAGIC_EXIT_CODE);
}

Test(validargs_suite, help_first_positional_arg_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "-v", "-h", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
	assert_validargs_expected_status(ret, exp_ret);
	exit(MAGIC_EXIT_CODE);
}

Test(validargs_suite, negative_indent_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "-c", "-p", "-4", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = -1;
	assert_validargs_expected_status(ret, exp_ret);
	exit(MAGIC_EXIT_CODE);
}
