#define main zemain
#include "lispy.c"
#undef main
/***************************************************************/
#include "ut/ut.h"
void expect_parse(char *input, long expected) {
    char *res = parse(input);
    ASSERT(!!res);
    EXPECT_EQ(expected, atoi(res));
    if (res) free(res);
}
TESTMETHOD(test2) {
    expect_parse("+ 1 -2 6", 5);
}
TESTMETHOD(ztest) {
    expect_parse("max 1 5 3", 5);
    expect_parse("min 1 5 3", 1);
    expect_parse("^ 4 2", 16);
    expect_parse("% 10 6", 4);
    expect_parse("- (* 10 10) (+ 1 1 1)", 97);
    expect_parse("* 10 (+ 1 51)", 520);
    expect_parse("* 1 2 6", 12);
    expect_parse("+ 1 2 6", 9);
}
