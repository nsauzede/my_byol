#define main themain
#include "lispy.c"
#undef main
/***************************************************************/
#include "ut/ut.h"
void assert_repr(char *input, const char *expected) {
    lval *res = eval(input);
    ASSERT(!!res);
    char *repr = lval_repr(res);
    EXPECT_EQ(expected, repr);
    free(repr);
    lval_del(res);
}
void assert_qexpr(char *input, const char *expected) {
    lval *res = eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_QEXPR);
    char *repr = lval_repr(res);
    EXPECT_EQ(expected, repr);
    free(repr);
    lval_del(res);
}
void assert_sexpr(char *input, const char *expected) {
    lval *res = eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_SEXPR);
    char *repr = lval_repr(res);
    EXPECT_EQ(expected, repr);
    free(repr);
    lval_del(res);
}
void assert_sexpr_noeval(char *input, const char *expected) {
    lval *res = lread(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_SEXPR);
    char *repr = lval_repr(res);
    EXPECT_EQ(expected, repr);
    free(repr);
    lval_del(res);
}
void assert_flt(char *input, double expected) {
    lval *res = eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_FLT);
    EXPECT_EQ(expected, res->flt);
    lval_del(res);
}
void assert_num(char *input, long expected) {
    lval *res = eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_NUM);
    EXPECT_EQ(expected, res->num);
    lval_del(res);
}
void assert_error(char *input, int error) {
    lval *res = eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_ERR);
    EXPECT_EQ(lerrors[error], res->err);
    lval_del(res);
}
void assert_error_noeval(char *input, int error) {
    lval *res = lread(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_ERR);
    EXPECT_EQ(lerrors[error], res->err);
    lval_del(res);
}
TESTMETHOD(test_sexpr) {
    assert_sexpr("", "()");
}
TESTMETHOD(test_qexpr) {
    assert_repr("list 1 2 3 4", "{1 2 3 4}");
    assert_repr("{head (list 1 2 3 4)}", "{head (list 1 2 3 4)}");
    assert_repr("eval {head (list 1 2 3 4)}", "{1}");
    assert_repr("tail {tail tail tail}", "{tail tail}");
    assert_repr("eval (tail {tail tail {5 6 7}})", "{6 7}");
    assert_repr("eval (head {(+ 1 2) (+ 10 20)})", "3");
    assert_repr("join {tail 111 head} {222} {head 333 tail}", "{tail 111 head 222 head 333 tail}");
}
TESTMETHOD(test_number_err) {
    assert_error("+ 9999999999999999999 1", LERR_BAD_NUM);
    assert_error("+ 12 2a3", LERR_PARSE_ERROR);
}
TESTMETHOD(test_div_zero) {
    assert_error("/ 10 0", LERR_DIV_ZERO);
}
TESTMETHOD(test_flt_err) {
    assert_error("+ 2 3.1", LERR_INVALID_OPERAND);
    assert_error("+ 1.1 2.", LERR_PARSE_ERROR);
}
TESTMETHOD(test_flt) {
    assert_flt("+ 1.2 1.3", 2.5);
    assert_flt("* 2.0 3.14", 6.28);
}
TESTMETHOD(test_noeval) {
    assert_sexpr_noeval("", "()");
    assert_sexpr_noeval("+", "(+)");
    assert_error_noeval("2,", LERR_PARSE_ERROR);
    assert_sexpr_noeval("+ 2 2", "(+ 2 2)");
    assert_sexpr_noeval("+ 2 2.1", "(+ 2 2.100000)");
    assert_sexpr_noeval("5", "(5)");
}
TESTMETHOD(test) {
    assert_num("+ 1 -2 6", 5);
    assert_num("max 1 5 3", 5);
    assert_num("min 1 5 3", 1);
    assert_num("^ 4 2", 16);
    assert_num("% 10 6", 4);
    assert_num("- (* 10 10) (+ 1 1 1)", 97);
    assert_num("* 10 (+ 1 51)", 520);
    assert_num("* 1 2 6", 12);
    assert_num("+ 1 2 6", 9);
}
TESTMETHOD(test_sappendf) {
    char *p = 0;
    int plen = 0;
    sappendf(&p, &plen, "%s%c%s", "hello", ' ', "world");
    printf("p=[%s]\n", p);
    EXPECT_EQ(p, "hello world");
    free(p);
}
