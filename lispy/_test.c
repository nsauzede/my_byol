#define main zemain
#include "lispy.c"
#undef main
/***************************************************************/
#include "ut/ut.h"
void assert_parse(char *input, long expected) {
    char *res = eval_print(input);
    ASSERT(!!res);
    EXPECT_EQ(expected, atoi(res));
    if (res) free(res);
}
void assert_num(char *input, long expected) {
    lval res = eval(input);
    ASSERT_EQ(res.type, LVAL_NUM);
    ASSERT_EQ(expected, res.num);
}
void expect_error(char *input, int error) {
    lval res = eval(input);
    ASSERT_EQ(res.type, LVAL_ERR);
    ASSERT_EQ(error, res.err);
}
TESTMETHOD(test_number_err) {
    expect_error("+ 9999999999999999999 1", LERR_BAD_NUM);
}
TESTMETHOD(test_div_zero) {
    expect_error("/ 10 0", LERR_DIV_ZERO);
}
TESTMETHOD(test2) {
    assert_num("+ 1 -2 6", 5);
}
TESTMETHOD(test) {
    assert_num("max 1 5 3", 5);
    assert_num("min 1 5 3", 1);
    assert_num("^ 4 2", 16);
    assert_num("% 10 6", 4);
    assert_num("- (* 10 10) (+ 1 1 1)", 97);
    assert_num("* 10 (+ 1 51)", 520);
    assert_num("* 1 2 6", 12);
    assert_num("+ 1 2 6", 9);
}
