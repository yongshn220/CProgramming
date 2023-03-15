#include "argo_utils.h"

// Read a simple ARGO_NUMBER_TYPE value
Test(argo_read_value_suite, simple_number_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    int num = 213;
    char buffer[MAX_BUFFER_LEN] = {0};

    sprintf(buffer, "%d", num);
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    ARGO_VALUE *read_val = argo_read_value(f);

    assert_argo_value_not_null(read_val);
    assert_argo_value_type(read_val->type, ARGO_NUMBER_TYPE);
    assert_argo_expected_value_int(read_val->content.number.int_value, num,
            read_val->content.number.valid_int);
    exit(MAGIC_EXIT_CODE);
}

// Read a simple ARGO_STRING_TYPE value
Test(argo_read_value_suite, simple_string_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char *ch = "\"hello\"";
    FILE *f = fmemopen(ch, strlen(ch), "r");
    ARGO_VALUE *read_val = argo_read_value(f);
    fclose(f);

    long s_len = 5;
    ARGO_CHAR *s_exp_content = char_to_ARGO_CHAR("hello", 5);

    assert_argo_value_not_null(read_val);
    assert_argo_value_type(read_val->type, ARGO_STRING_TYPE);
    assert_argo_expected_argo_char(read_val->content.string.content, s_exp_content,
            read_val->content.string.length, s_len);

    exit(MAGIC_EXIT_CODE);
}

// Read a simple ARGO_BASIC_TYPE value
Test(argo_read_value_suite, simple_basic_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};

    sprintf(buffer, "true");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    ARGO_VALUE *read_val = argo_read_value(f);
    fclose(f);

    ARGO_VALUE_TYPE exp_type = ARGO_BASIC_TYPE;
    ARGO_BASIC val = ARGO_TRUE;
    assert_argo_value_not_null(read_val);
    assert_argo_value_type(read_val->type, ARGO_BASIC_TYPE);
    assert_argo_expected_value_basic(read_val->content.basic, val);

    exit(MAGIC_EXIT_CODE);
}

// Read a simple ARGO_OBJECT_TYPE value
Test(argo_read_value_suite, simple_object_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};

    sprintf(buffer, "{ \"cse\": 320 }");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    ARGO_VALUE *read_val = argo_read_value(f);
    fclose(f);

    ARGO_CHAR *exp_name = char_to_ARGO_CHAR("cse", 3);

    assert_argo_value_not_null(read_val);
    assert_argo_value_type(read_val->type, ARGO_OBJECT_TYPE);
    ARGO_VALUE *ml = read_val->content.object.member_list;
    ARGO_VALUE *val = ml->next;
    assert_argo_expected_argo_char(val->name.content, exp_name,
            val->name.length, 3);
    assert_argo_expected_value_int(val->content.number.int_value, 320, 1);
    exit(MAGIC_EXIT_CODE);
}

// Read a simple ARGO_ARRAY_TYPE value
Test(argo_read_value_suite, simple_array_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};

    sprintf(buffer, "[ \"cse\", 320 ]");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    ARGO_VALUE *read_val = argo_read_value(f);
    fclose(f);

    ARGO_CHAR *exp_str = char_to_ARGO_CHAR("cse", 3);

    assert_argo_value_not_null(read_val);
    assert_argo_value_type(read_val->type, ARGO_ARRAY_TYPE);
    ARGO_VALUE *el = read_val->content.array.element_list;
    ARGO_VALUE *val = el->next;
    assert_argo_expected_argo_char(val->content.string.content, exp_str,
            val->content.string.length, 3);
    val = val->next;
    assert_argo_expected_value_int(val->content.number.int_value, 320, 1);
    exit(MAGIC_EXIT_CODE);
}

// Read an empty ARGO_OBJECT_TYPE value
Test(argo_read_value_suite, object_empty_member, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};

    sprintf(buffer, "{ }");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    ARGO_VALUE *read_val = argo_read_value(f);
    fclose(f);

    assert_argo_value_not_null(read_val);
    assert_argo_value_type(read_val->type, ARGO_OBJECT_TYPE);
    ARGO_VALUE *ml = read_val->content.object.member_list;
    assert_argo_list_empty(ml, "Object");
    exit(MAGIC_EXIT_CODE);
}

// Read an empty ARGO_ARRAY_TYPE value
Test(argo_read_value_suite, array_empty_list, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};

    sprintf(buffer, "[ ]");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    ARGO_VALUE *read_val = argo_read_value(f);
    fclose(f);

    assert_argo_value_not_null(read_val);
    assert_argo_value_type(read_val->type, ARGO_ARRAY_TYPE);
    ARGO_VALUE *el = read_val->content.array.element_list;
    assert_argo_list_empty(el, "Array");

    exit(MAGIC_EXIT_CODE);
}

// Read an ARGO_VALUE that has nested structures inside an argo array
Test(argo_read_value_suite, nested_argo_value, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};

    sprintf(buffer, "[ [\"sbu\", \"cs\", 2022], 1962, { \"nested\": \"test\", \"name\": \"content\" } ]");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    ARGO_VALUE *read_val = argo_read_value(f);
    fclose(f);

    assert_argo_value_not_null(read_val);
    assert_argo_value_type(read_val->type, ARGO_ARRAY_TYPE);
    ARGO_VALUE *el = read_val->content.array.element_list;
    ARGO_VALUE *arr = el->next;
    ARGO_VALUE *arr_el = arr->content.array.element_list;
    arr_el = arr_el->next;
    assert_argo_expected_argo_char(arr_el->content.string.content, char_to_ARGO_CHAR("sbu", 3),
            arr_el->content.string.length, 3);
    arr_el = arr_el->next;
    assert_argo_expected_argo_char(arr_el->content.string.content, char_to_ARGO_CHAR("cs", 2),
            arr_el->content.string.length, 2);
    arr_el = arr_el->next;
    assert_argo_expected_value_int(arr_el->content.number.int_value, 2022, arr_el->content.number.valid_int);
    arr = arr->next;
    assert_argo_expected_value_int(arr->content.number.int_value, 1962, arr->content.number.valid_int);
    arr = arr->next;
    assert_argo_value_type(arr->type, ARGO_OBJECT_TYPE);
    ARGO_VALUE *ml = arr->content.object.member_list;
    ml = ml->next;
    assert_argo_expected_argo_char(ml->name.content, char_to_ARGO_CHAR("nested", 6),
            ml->name.length, 6);
	assert_argo_expected_argo_char(ml->content.string.content, char_to_ARGO_CHAR("test", 4),
			ml->content.string.length, 4);
    ml = ml->next;
    assert_argo_expected_argo_char(ml->name.content, char_to_ARGO_CHAR("name", 4),
            ml->name.length, 4);
	assert_argo_expected_argo_char(ml->content.string.content, char_to_ARGO_CHAR("content", 7),
			ml->content.string.length, 7);

    exit(MAGIC_EXIT_CODE);
}

// Check for number of ARGO_VALUE's used inside argo_value_storage
Test(argo_read_value_suite, array_storage_usage, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    char buffer[MAX_BUFFER_LEN] = {0};

    for (int i = 0; i < NUM_ARGO_VALUES; i++)
        argo_value_storage[i].type = ARGO_NO_TYPE;
    sprintf(buffer, "[ [\"sbu\", \"cs\", 2022], 1962, { \"nested\": \"test\", \"name\": \"content\" } ]");
    FILE *f = fmemopen(&buffer, strlen(buffer), "r");
    ARGO_VALUE *read_val = argo_read_value(f);
    fclose(f);

    int num_argo_exp = 9;
    int count = 0;
    for (int i = 0; i < NUM_ARGO_VALUES; i++) {
        if (argo_value_storage[i].type != ARGO_NO_TYPE)
            count++;
    }
    cr_assert_eq(count, num_argo_exp,
            "Invalid number of ARGO_VALUE's being used in argo_value_storage. Got: %d | Expected: %d",
            count, num_argo_exp);

    exit(MAGIC_EXIT_CODE);
}

// Write a simple ARGO_NUMBER_TYPE value
Test(argo_write_value_suite, simple_number_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE)
{
	char buffer[MAX_BUFFER_LEN] = {0};
    FILE *f = fmemopen(&buffer, sizeof(buffer), "w");

    long exp_num = 3948;
    int ret;
    ARGO_VALUE *val = malloc(sizeof(ARGO_VALUE));
    val->type = ARGO_NUMBER_TYPE;
    val->content.number.valid_int = 1;
    val->content.number.int_value = exp_num;
    ret = argo_write_value(val, f);
    fclose(f);

	assert_normal_exit(ret);
    assert_argo_expected_value_string(buffer, "3948", 1);
    free(val);
    exit(MAGIC_EXIT_CODE);
}

// Write a simple ARGO_STRING_TYPE value
Test(argo_write_value_suite, simple_string_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE)
{
	char buffer[MAX_BUFFER_LEN] = {0};
    FILE *f = fmemopen(&buffer, sizeof(buffer), "w");

    char *exp_str = "\"gitlab\"";
    int ret;
    ARGO_VALUE *val = malloc(sizeof(ARGO_VALUE));
    val->type = ARGO_STRING_TYPE;
	val->content.string.capacity = 0;
	val->content.string.length = 0;
	write_string_content(&val->content.string, "gitlab");
	ret = argo_write_value(val, f);
	fclose(f);

	assert_normal_exit(ret);
	assert_argo_expected_string(buffer, exp_str, strlen(buffer), 8);
	free(val->content.string.content);
    free(val);
    exit(MAGIC_EXIT_CODE);
}

// Write a simple ARGO_BASIC_TYPE value
Test(argo_write_value_suite, simple_basic_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE)
{
	char buffer[MAX_BUFFER_LEN] = {0};
    FILE *f = fmemopen(&buffer, sizeof(buffer), "w");

    int ret;
    ARGO_VALUE *val = malloc(sizeof(ARGO_VALUE));
    val->type = ARGO_BASIC_TYPE;
    val->content.basic = ARGO_NULL;
    ret = argo_write_value(val, f);
	fclose(f);

    assert_normal_exit(ret);
	assert_argo_expected_string(buffer, "null", strlen(buffer), 4);
    free(val);
    exit(MAGIC_EXIT_CODE);
}

// Write a simple ARGO_OBJECT_TYPE value
Test(argo_write_value_suite, simple_object_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE)
{
	char buffer[MAX_BUFFER_LEN] = {0};
	FILE *f = fmemopen(&buffer, sizeof(buffer), "w");

	int ret;
	ARGO_VALUE *val = malloc(sizeof(ARGO_VALUE));
	val->type = ARGO_OBJECT_TYPE;
	ARGO_VALUE *ml = malloc(sizeof(ARGO_VALUE));
	val->content.object.member_list = ml;
	ARGO_VALUE *outval = malloc(sizeof(ARGO_VALUE));
	ml->type = ARGO_NO_TYPE;
	ml->next = outval;
	ml->prev = outval;
	outval->type = ARGO_NUMBER_TYPE;
	outval->next = ml;
	outval->prev = ml;
	outval->name.capacity = 0;
	outval->name.length = 0;
	write_string_content(&outval->name, "simple");
	outval->content.number.valid_int = 1;
	outval->content.number.int_value = 16;
	ret = argo_write_value(val, f);
	fclose(f);

	assert_normal_exit(ret);
	assert_argo_expected_string(buffer, "{\"simple\":16}", strlen(buffer), 13);
	free(outval->content.string.content);
	free(outval);
	free(ml);
	free(val);
	exit(MAGIC_EXIT_CODE);
}

// Write a simple ARGO_VALUE_TYPE value
Test(argo_write_value_suite, simple_array_type, .timeout = 5, .exit_code = MAGIC_EXIT_CODE)
{
	char buffer[MAX_BUFFER_LEN] = {0};
	FILE *f = fmemopen(&buffer, sizeof(buffer), "w");

	int ret;
	ARGO_VALUE *val = malloc(sizeof(ARGO_VALUE));
	val->type = ARGO_ARRAY_TYPE;
	ARGO_VALUE *el = malloc(sizeof(ARGO_VALUE));
	val->content.array.element_list = el;
	ARGO_VALUE *outval = malloc(sizeof(ARGO_VALUE));
	el->type = ARGO_NO_TYPE;
	el->next = outval;
	el->prev = outval;
	outval->type = ARGO_STRING_TYPE;
	outval->next = el;
	outval->prev = el;
	outval->content.string.capacity = 0;
	outval->content.string.length = 0;
	write_string_content(&outval->content.string, "skywalker");
	ret = argo_write_value(val, f);
	fclose(f);

	assert_normal_exit(ret);
	assert_argo_expected_string(buffer, "[\"skywalker\"]", strlen(buffer), 13);
	free(outval->content.string.content);
	free(outval);
	free(el);
	free(val);
	exit(MAGIC_EXIT_CODE);
}

// Write the canonicalized version of a nested ARGO_ARRAY_TYPE value
Test(argo_write_value_suite, nested_canonical, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
	char buffer[MAX_BUFFER_LEN] = {0};
	FILE *f = fmemopen(&buffer, sizeof(buffer), "w");

	int ret;
	global_options = CANONICALIZE_OPTION;
	ARGO_VALUE *val = malloc(sizeof(ARGO_VALUE));
	val->type = ARGO_ARRAY_TYPE;
	ARGO_VALUE *el = malloc(sizeof(ARGO_VALUE));
	val->content.array.element_list = el;
	ARGO_VALUE *array = malloc(sizeof(ARGO_VALUE));
	el->type = ARGO_NO_TYPE;
	el->next = array;
	el->prev = array;
	array->type = ARGO_ARRAY_TYPE;
	array->next = el;
	array->prev = el;
	ARGO_VALUE *array_sentinel = malloc(sizeof(ARGO_VALUE));
	array_sentinel->type = ARGO_NO_TYPE;
	array->content.array.element_list = array_sentinel;

	ARGO_VALUE *array_el = malloc(sizeof(ARGO_VALUE));
	ARGO_VALUE *prev = array_el;
	array_sentinel->next = array_el;
	array_el->prev = array_sentinel;
	array_el->type = ARGO_STRING_TYPE;
	array_el->content.string.capacity = 0;
	array_el->content.string.length = 0;
	write_string_content(&array_el->content.string, "sbu");

	array_el->next = malloc(sizeof(ARGO_VALUE));
	array_el = array_el->next;
	array_el->next = array_sentinel;
	array_sentinel->prev = array_el;
	array_el->type = ARGO_NUMBER_TYPE;
	array_el->content.number.valid_int = 1;
	array_el->content.number.int_value = 2022;
	ret = argo_write_value(val, f);
	fclose(f);

	assert_normal_exit(ret);
	assert_argo_expected_string(buffer, "[[\"sbu\",2022]]", strlen(buffer), 14);
	free(array_el);
	free(array_sentinel);
	free(array);
	free(el);
	free(val);
	exit(MAGIC_EXIT_CODE);
}

// Write the pretty version of the nested ARGO_ARRAY_TYPE value above
Test(argo_write_value_suite, nested_pretty_print, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
	char buffer[MAX_BUFFER_LEN] = {0};
	FILE *f = fmemopen(&buffer, sizeof(buffer), "w");

	int ret;
	global_options = CANONICALIZE_OPTION | PRETTY_PRINT_OPTION | 0x4;
	ARGO_VALUE *val = malloc(sizeof(ARGO_VALUE));
	val->type = ARGO_ARRAY_TYPE;
	ARGO_VALUE *el = malloc(sizeof(ARGO_VALUE));
	val->content.array.element_list = el; ARGO_VALUE *array = malloc(sizeof(ARGO_VALUE));
	el->type = ARGO_NO_TYPE;
	el->next = array;
	el->prev = array;
	array->type = ARGO_ARRAY_TYPE;
	array->next = el;
	array->prev = el;
	ARGO_VALUE *array_sentinel = malloc(sizeof(ARGO_VALUE));
	array_sentinel->type = ARGO_NO_TYPE;
	array->content.array.element_list = array_sentinel;

	ARGO_VALUE *array_el = malloc(sizeof(ARGO_VALUE));
	ARGO_VALUE *prev = array_el;
	array_sentinel->next = array_el;
	array_el->prev = array_sentinel;
	array_el->type = ARGO_STRING_TYPE;
	array_el->content.string.capacity = 0;
	array_el->content.string.length = 0;
	write_string_content(&array_el->content.string, "sbu");

	array_el->next = malloc(sizeof(ARGO_VALUE));
	array_el = array_el->next;
	array_el->next = array_sentinel;
	array_sentinel->prev = array_el;
	array_el->type = ARGO_NUMBER_TYPE;
	array_el->content.number.valid_int = 1;
	array_el->content.number.int_value = 2022;
	ret = argo_write_value(val, f);
	fclose(f);

	assert_normal_exit(ret);
	assert_argo_expected_string(buffer, "[\n    [\n        \"sbu\",\n        2022\n    ]\n]\n", strlen(buffer), 44);
	free(array_el);
	free(array_sentinel);
	free(array);
	free(el);
	free(val);
	exit(MAGIC_EXIT_CODE);
}

// Write to /dev/full, return error
Test(argo_write_value_suite, output_to_dev_full, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
    FILE *f = fopen("/dev/full", "w");

    long exp_num = 0;
    int ret;
    ARGO_VALUE *val = malloc(sizeof(ARGO_VALUE));
    val->type = ARGO_NUMBER_TYPE;
    val->content.number.valid_int = 1;
    val->content.number.int_value = exp_num;
    argo_write_value(val, f);
	ret = fflush(f);
    fclose(f);

	assert_argo_expected_status(ret, -1, "argo_write_value");
    free(val);
    exit(MAGIC_EXIT_CODE);
}

// Store ARGO_VALUE at random indices in the storage array
Test(argo_write_value_suite, randomized_storage_indices, .timeout = 5, .exit_code = MAGIC_EXIT_CODE) {
	char buffer[MAX_BUFFER_LEN] = {0};
	FILE *f = fmemopen(&buffer, sizeof(buffer), "w");

	int ret;
	ARGO_VALUE *val = &argo_value_storage[4102];
	val->type = ARGO_OBJECT_TYPE;
	ARGO_VALUE *ml = &argo_value_storage[92814];
	val->content.object.member_list = ml;
	ARGO_VALUE *outval = &argo_value_storage[57891];
	ml->type = ARGO_NO_TYPE;
	ml->next = outval;
	ml->prev = outval;
	outval->type = ARGO_NUMBER_TYPE;
	outval->next = ml;
	outval->prev = ml;
	outval->name.capacity = 0;
	outval->name.length = 0;
	write_string_content(&outval->name, "randomized");
	outval->content.number.valid_int = 1;
	outval->content.number.int_value = 7384;
	ret = argo_write_value(val, f);
	fclose(f);

	assert_normal_exit(ret);
	assert_argo_expected_string(buffer, "{\"randomized\":7384}", strlen(buffer), 19);
	free(outval->content.string.content);
	exit(MAGIC_EXIT_CODE);
}
