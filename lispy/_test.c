#define main themain
#include "lispy.c"
#undef main
/***************************************************************/
#include "ut/ut.h"
#define assert_repr(input,expected) assert_repr_(0,input,expected)
#define assert_repr_noeval(input,expected) assert_repr_(1,input,expected)
void assert_repr_(int read_not_eval, char *input, const char *expected) {
    lval *res = read_not_eval ? lread(input) : eval(input);
    ASSERT(!!res);
    char *repr = lval_repr(res);
    EXPECT_EQ(expected, repr);
    free(repr);
    lval_del(res);
}
#define assert_error(input,expected) assert_error_(0,input,expected)
#define assert_error_noeval(input,expected) assert_error_(1,input,expected)
void assert_error_(int read_not_eval, char *input, int expected) {
    lval *res = read_not_eval ? lread(input) : eval(input);
    ASSERT(!!res);
    EXPECT_EQ(res->type, LVAL_ERR);
    EXPECT_EQ(lerrors[expected], res->err);
    lval_del(res);
}
TESTMETHOD(test_fun) {
    assert_repr("eval (head {5 10 11 15})", "5");
    assert_repr("(eval (head {+ - + - * /})) 10 20", "30");
    assert_error_noeval("hello", LERR_PARSE_ERROR);
    //assert_repr("+", "<function>");
    //assert_repr("eval (head {+ - + - * /})", "<function>");
}
TESTMETHOD(test_sexpr) {
    assert_repr("", "()");
}
TESTMETHOD(test_qexpr) {
    assert_repr("list 1 2 3 4", "{1 2 3 4}");
    assert_repr("{head (list 1 2 3 4)}", "{head (list 1 2 3 4)}");
    assert_repr("eval {head (list 1 2 3 4)}", "{1}");
    assert_repr("tail {tail tail tail}", "{tail tail}");
    assert_repr("eval (tail {tail tail {5 6 7}})", "{6 7}");
    assert_repr("eval (head {(+ 1 2) (+ 10 20)})", "3");
    assert_repr("join {tail 111 head} {222} {head 333 tail}", "{tail 111 head 222 head 333 tail}");
    assert_repr("len {}", "0");
    assert_repr("len {0 1 2 3 4 5}", "6");
    assert_repr("cons 0 {}", "{0}");
    assert_repr("cons 1 {2 {3}}", "{1 2 {3}}");
    assert_repr("init {3 2 1 0}", "{3 2 1}");
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
    assert_repr("+ 1.2 1.3", "2.5");
    assert_repr("* 2.0 3.14", "6.28");
}
TESTMETHOD(test_noeval) {
    assert_repr_noeval("", "()");
    assert_repr_noeval("+", "(+)");
    assert_error_noeval("2,", LERR_PARSE_ERROR);
    assert_repr_noeval("+ 2 2", "(+ 2 2)");
    assert_repr_noeval("+ 2 2.1", "(+ 2 2.1)");
    assert_repr_noeval("5", "(5)");
}
TESTMETHOD(test) {
    assert_repr("+ 1 -2 6", "5");
    assert_repr("max 1 5 3", "5");
    assert_repr("min 1 5 3", "1");
    assert_repr("^ 4 2", "16");
    assert_repr("% 10 6", "4");
    assert_repr("- (* 10 10) (+ 1 1 1)", "97");
    assert_repr("* 10 (+ 1 51)", "520");
    assert_repr("* 1 2 6", "12");
    assert_repr("+ 1 2 6", "9");
}
TESTMETHOD(test_sappendf) {
    char *p = 0;
    int plen = 0;
    sappendf(&p, &plen, "%s%c%s", "hello", ' ', "world");
    printf("p=[%s]\n", p);
    EXPECT_EQ(p, "hello world");
    free(p);
}
