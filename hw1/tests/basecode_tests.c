#include "argo_utils.h"

Test(basecode_suite, validargs_help_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "-h", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = HELP_OPTION;
	assert_validargs_expected_status(ret, exp_ret);
	assert_validargs_expected_options(opt, exp_opt);
	exit(MAGIC_EXIT_CODE);
}

Test(basecode_suite, validargs_validate_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "-v", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = VALIDATE_OPTION;
	assert_validargs_expected_status(ret, exp_ret);
	assert_validargs_expected_options(opt, exp_opt);
	exit(MAGIC_EXIT_CODE);
}

Test(basecode_suite, validargs_canonicalize_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "-c", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = CANONICALIZE_OPTION;
	assert_validargs_expected_status(ret, exp_ret);
	assert_validargs_expected_options(opt, exp_opt);
	exit(MAGIC_EXIT_CODE);
}

Test(basecode_suite, validargs_pretty_print_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "-c", "-p", "13", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = CANONICALIZE_OPTION | PRETTY_PRINT_OPTION | 13;
	assert_validargs_expected_status(ret, exp_ret);
	assert_validargs_expected_options(opt, exp_opt);
	exit(MAGIC_EXIT_CODE);
}

Test(basecode_suite, validargs_error_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *argv[] = {progname, "-v", "-p", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int exp_ret = -1;
    int ret = validargs(argc, argv);
	assert_validargs_expected_status(ret, exp_ret);
	exit(MAGIC_EXIT_CODE);
}

Test(basecode_suite, help_system_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *cmd = "ulimit -t 5; ulimit -f 2000; bin/argo -h > /dev/null 2>&1";

    // system is a syscall defined in stdlib.h
    // it takes a shell command as a string and runs it
    // we use WEXITSTATUS to get the return code from the run
    // use 'man 3 system' to find out more
    int return_code = WEXITSTATUS(system(cmd));
	assert_normal_exit(return_code);
	exit(MAGIC_EXIT_CODE);
}

Test(basecode_suite, argo_basic_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *cmd = "ulimit -t 5; ulimit -f 2000; bin/argo -c < rsrc/strings.json > test_output/strings_-c.json";
    char *cmp = "cmp test_output/strings_-c.json tests/rsrc/strings_-c.json";

    int return_code = WEXITSTATUS(system(cmd));
	assert_normal_exit(return_code);
    return_code = WEXITSTATUS(system(cmp));
	assert_outfile_matches(return_code);
	exit(MAGIC_EXIT_CODE);
}
