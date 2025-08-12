#define main zemain
#include "lispy.c"
#undef main
/***************************************************************/
#include "ut/ut.h"
void assert_parse(char *input, long expected) {
    char *res = parse(input);
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
    assert_parse("+ 1 -2 6", 5);
}
TESTMETHOD(test) {
    assert_parse("max 1 5 3", 5);
    assert_parse("min 1 5 3", 1);
    assert_parse("^ 4 2", 16);
    assert_parse("% 10 6", 4);
    assert_parse("- (* 10 10) (+ 1 1 1)", 97);
    assert_parse("* 10 (+ 1 51)", 520);
    assert_parse("* 1 2 6", 12);
    assert_parse("+ 1 2 6", 9);
}
