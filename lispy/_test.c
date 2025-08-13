#define main themain
#include "lispy.c"
#undef main
/***************************************************************/
#include "ut/ut.h"
void expect_sexpr(char *input, const char *expected) {
    lval *res = eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_SEXPR);
    char *repr = lval_repr(res);
    EXPECT_EQ(expected, repr);
    free(repr);
    lval_del(res);
}
void expect_sexpr_noeval(char *input, const char *expected) {
    lval *res = lread(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_SEXPR);
    char *repr = lval_repr(res);
    EXPECT_EQ(expected, repr);
    free(repr);
    lval_del(res);
}
void expect_flt(char *input, double expected) {
    lval *res = eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_FLT);
    EXPECT_EQ(expected, res->flt);
    lval_del(res);
}
void expect_num(char *input, long expected) {
    lval *res = eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_NUM);
    EXPECT_EQ(expected, res->num);
    lval_del(res);
}
void expect_error(char *input, int error) {
    lval *res = eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_ERR);
    EXPECT_EQ(lerrors[error], res->err);
    lval_del(res);
}
void expect_error_noeval(char *input, int error) {
    lval *res = lread(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_ERR);
    EXPECT_EQ(lerrors[error], res->err);
    lval_del(res);
}
TESTMETHOD(test_sappendf) {
    char *p = 0;
    int plen = 0;
    sappendf(&p, &plen, "%s%c%s", "hello", ' ', "world");
    printf("p=[%s]\n", p);
    EXPECT_EQ(p, "hello world");
    free(p);
}
TESTMETHOD(test_number_err) {
    expect_error("+ 9999999999999999999 1", LERR_BAD_NUM);
    expect_error("+ 12 2a3", LERR_PARSE_ERROR);
}
TESTMETHOD(test_div_zero) {
    expect_error("/ 10 0", LERR_DIV_ZERO);
}
TESTMETHOD(test_flt_err) {
    expect_error("+ 2 3.1", LERR_INVALID_OPERAND);
    expect_error("+ 1.1 2.", LERR_PARSE_ERROR);
}
TESTMETHOD(test_flt) {
    expect_flt("+ 1.2 1.3", 2.5);
    expect_flt("* 2.0 3.14", 6.28);
}
TESTMETHOD(test_noeval) {
    expect_sexpr_noeval("", "()");
    expect_sexpr_noeval("+", "(+)");
    expect_error_noeval("2,", LERR_PARSE_ERROR);
    expect_sexpr_noeval("+ 2 2", "(+ 2 2)");
    expect_sexpr_noeval("+ 2 2.1", "(+ 2 2.100000)");
    expect_sexpr_noeval("5", "(5)");
}
TESTMETHOD(test_sexpr) {
    expect_sexpr("", "()");
}
TESTMETHOD(test) {
    expect_num("+ 1 -2 6", 5);
    expect_num("max 1 5 3", 5);
    expect_num("min 1 5 3", 1);
    expect_num("^ 4 2", 16);
    expect_num("% 10 6", 4);
    expect_num("- (* 10 10) (+ 1 1 1)", 97);
    expect_num("* 10 (+ 1 51)", 520);
    expect_num("* 1 2 6", 12);
    expect_num("+ 1 2 6", 9);
}
