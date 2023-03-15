#include "argo_utils.h"

// Test against a simple json file, output the canonicalized version
Test(blackbox_suite, simple_json_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *cmd = "ulimit -t 5; ulimit -f 2000; bin/argo -c -p 8 < tests/test_inputs/simple_inp.json > tests/test_outputs/simple_out.json";
    char *cmp = "cmp tests/test_outputs/exp_simple_out.json tests/test_outputs/simple_out.json";

    int return_code = WEXITSTATUS(system(cmd));
	assert_normal_exit(return_code);
    return_code = WEXITSTATUS(system(cmp));
	assert_outfile_matches(return_code);
    remove("tests/test_outputs/simple_out.json");
	exit(MAGIC_EXIT_CODE);
}

// Test against an average length json file, output the canonicalized version
Test(blackbox_suite, average_json_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *cmd = "ulimit -t 5; ulimit -f 2000; bin/argo -c -p 4 < tests/test_inputs/average_inp.json > tests/test_outputs/average_out.json";
    char *cmp = "cmp tests/test_outputs/exp_average_out.json tests/test_outputs/average_out.json";

    int return_code = WEXITSTATUS(system(cmd));
	assert_normal_exit(return_code);
    return_code = WEXITSTATUS(system(cmp));
	assert_outfile_matches(return_code);
    remove("tests/test_outputs/average_out.json");
	exit(MAGIC_EXIT_CODE);
}

// Test against a huge json file, output the pretty version
Test(blackbox_suite, long_json_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *cmd = "ulimit -t 5; ulimit -f 2000; bin/argo -c -p 10 < tests/test_inputs/long_inp.json > tests/test_outputs/long_out.json";
    char *cmp = "cmp tests/test_outputs/exp_long_out.json tests/test_outputs/long_out.json";

    int return_code = WEXITSTATUS(system(cmd));
	assert_normal_exit(return_code);
    return_code = WEXITSTATUS(system(cmp));
	assert_outfile_matches(return_code);
    remove("tests/test_outputs/long_out.json");
	exit(MAGIC_EXIT_CODE);
}

// Read a json file with varying indentations and output the pretty version
Test(blackbox_suite, different_spaces_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *cmd = "ulimit -t 5; ulimit -f 2000; bin/argo -c -p 5 < tests/test_inputs/diff_space_inp.json > tests/test_outputs/diff_space_out.json";
    char *cmp = "cmp tests/test_outputs/exp_diff_space_out.json tests/test_outputs/diff_space_out.json";

    int return_code = WEXITSTATUS(system(cmd));
    assert_normal_exit(return_code);
    return_code = WEXITSTATUS(system(cmp));
    assert_outfile_matches(return_code);
    remove("tests/test_outputs/diff_space_out.json");
	exit(MAGIC_EXIT_CODE);
}

// Read and write a json file contains non-ascii characters
Test(blackbox_suite, non_ascii_test, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *cmd = "ulimit -t 5; ulimit -f 2000; bin/argo -c -p 8 < tests/test_inputs/non_ascii_inp.json > tests/test_outputs/non_ascii_out.json";
    char *cmp = "cmp tests/test_outputs/exp_non_ascii_out.json tests/test_outputs/non_ascii_out.json";

    int return_code = WEXITSTATUS(system(cmd));
    assert_normal_exit(return_code);
    return_code = WEXITSTATUS(system(cmp));
    assert_outfile_matches(return_code);
    remove("tests/test_outputs/non_ascii_out.json");
	exit(MAGIC_EXIT_CODE);
}
