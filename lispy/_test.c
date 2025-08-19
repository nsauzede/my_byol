#define main themain
#include "lispy.c"
#undef main
/***************************************************************/
#include "ut/ut.h"
void assert_repr_lval(lval *v, const char *expected) {
    ASSERT(!!v);
    char *repr = lval_repr(v);
    EXPECT_EQ(expected, repr);
    free(repr);
}
void assert_repr_(int read_not_eval, lenv *e, char *input, const char *expected) {
    lval *res = read_not_eval ? lread(input) : eval(e, input);
    assert_repr_lval(res, expected);
    lval_del(res);
}
#define assert_repr(e,input,expected) assert_repr_(0,e,input,expected)
#define assert_repr_noeval(e,input,expected) assert_repr_(1,e,input,expected)
void assert_error_lval(lval *v, int expected) {
    ASSERT(!!v);
    EXPECT_EQ(LVAL_ERR, v->type);
    EXPECT_EQ(expected, v->errcode);
}
void assert_error_(int read_not_eval, lenv *e, char *input, int expected) {
    lval *res = read_not_eval ? lread(input) : eval(e,input);
    assert_error_lval(res, expected);
    lval_del(res);
}
#define assert_error(e,input,expected) assert_error_(0,e,input,expected)
#define assert_error_noeval(e,input,expected) assert_error_(1,e,input,expected)
/*
def {fun} (\ {args body} {def (head args) (\ (tail args) body)})
fun {unpack f xs} {eval (join (list f) xs)}
fun {pack f & xs} {f xs}
def {uncurry} pack
def {curry} unpack
curry + {5 6 7}
uncurry head 5 6 7
*/
TESTMETHOD(test_currying) {
    lenv *e = lenv_new();
    assert_repr(e, "def {fun} (\\ {args body} {def (head args) (\\ (tail args) body)})", "()");
    assert_repr(e, "fun {unpack f xs} {eval (join (list f) xs)}", "()");
    assert_repr(e, "fun {pack f & xs} {f xs}", "()");
    assert_repr(e, "def {uncurry} pack", "()");
    assert_repr(e, "def {curry} unpack", "()");
    assert_repr(e, "curry + {5 6 7}", "18");
    assert_repr(e, "uncurry head 5 6 7", "{5}");
    assert_repr(e, "def {add-uncurried} +", "()");
    assert_repr(e, "def {add-curried} (curry +)", "()");
    assert_repr(e, "add-curried {5 6 7}", "18");
    assert_repr(e, "add-uncurried 5 6 7", "18");

    assert_repr(e, "def {head-curried} head", "()");
    assert_repr(e, "head-curried {5 6 7}", "{5}");
//    assert_repr(e, "def {head-uncurried} (uncurry head)", "()");
//    assert_repr(e, "def {head-uncurried} {uncurry head}", "()");
//    assert_repr(e, "head-uncurried 5 6 7", "{5}");
    lenv_del(e);
}
TESTMETHOD(test_fun) {
    lenv *e = lenv_new();
    assert_repr(e, "def {fun} (\\ {args body} {def (head args) (\\ (tail args) body)})", "()");
    assert_repr(e, "fun {add-together x y} {+ x y}", "()");
    assert_repr(e, "add-together 1 2", "3");
    assert_error(e, "add-together 1 2 3", LERR_INVALID_OPERAND);

    assert_repr(e, "fun {unpack f xs} {eval (join (list f) xs)}", "()");
    assert_repr(e, "fun {first x} {eval (head x)}", "()");
    assert_repr(e, "first {10 11 12}", "10");
    assert_repr(e, "fun {second x} {eval (head (tail x))}", "()");
    assert_repr(e, "second {10 11 12}", "11");
    assert_repr(e, "- 5 3", "2");
    assert_repr(e, "- 3 5", "-2");
    assert_repr(e, "fun {swap f x y} {f y x}", "()");
    assert_repr(e, "swap - 5 3", "-2");
    assert_repr(e, "fun {double x} {* 2 x}", "()");
    assert_repr(e, "double 5", "10");
    assert_repr(e, "fun {compute x y} {double (swap - x y)}", "()");
    assert_repr(e, "compute 10 5", "-10");
    lenv_del(e);
}
TESTMETHOD(test_lambda) {
    lenv *e = lenv_new();
    assert_repr(e, "\\ {x y} {+ x y}", "(\\ {x y} {+ x y})");
    assert_repr(e, "def {add-mul} (\\ {x y} {+ x (* x y)})", "()");
    assert_repr(e, "add-mul 10 20", "210");
    assert_repr(e, "add-mul 10", "(\\ {y} {+ x (* x y)})");
    assert_repr(e, "def {add-mul-ten} (add-mul 10)", "()");
    assert_repr(e, "add-mul-ten 50", "510");
    lenv_del(e);
}
TESTMETHOD(test_def) {
    lenv *e = lenv_new();
    assert_repr(e, "def", "<function 'def'>");
    assert_error(e, "def 1", LERR_INVALID_OPERAND);
    assert_error(e, "def 1 2", LERR_INVALID_OPERAND);
    assert_error(e, "def {}", LERR_INVALID_OPERAND);
    assert_error(e, "def {a}", LERR_INVALID_OPERAND);
    assert_error(e, "def {a} 1 2", LERR_INVALID_OPERAND);
    assert_error(e, "def {a b} 1", LERR_INVALID_OPERAND);
    assert_error(e, "def {def} 1", LERR_INVALID_OPERAND); // builtin syms can't be overriden
    assert_repr(e, "def {x} 100", "()");
    assert_repr(e, "def {y} 200", "()");
    assert_repr(e, "x", "100");
    assert_repr(e, "y", "200");
    assert_repr(e, "+ x y", "300");
    assert_repr(e, "def {a b} 5 6", "()");
    assert_repr(e, "* a b", "30");
    assert_repr(e, "def {arglist} {a b x y}", "()");
    assert_repr(e, "arglist", "{a b x y}");
    assert_repr(e, "def arglist 1 2 3 4", "()");
    assert_repr(e, "list a b x y", "{1 2 3 4}");
    lenv_del(e);
}
TESTMETHOD(test_env) {
    lenv *e = lenv_new();
    lval *k = lval_sym("hello");
    lval *k2 = lval_copy(k);
    EXPECT_EQ(k->sym, k2->sym);
    lval *res = lenv_get(e, k);
    assert_error_lval(res, LERR_UNBOUND_SYM);
    lval_del(res);
    lval *v = lval_num(42);
    lval *v2 = lval_num(666);
    lenv_put(e, k2, v);
    res = lenv_get(e, k);
    assert_repr_lval(res, "42");
    lval_del(res);
    lenv_put(e, k2, v2);
    res = lenv_get(e, k);
    assert_repr_lval(res, "666");
    lval_del(res);
    lval_del(v2);
    lval_del(v);
    lval_del(k2);
    lval_del(k);
    lenv_del(e);
}
static lval *lbuiltin1(lenv *e, lval *v) { return 0; }
TESTMETHOD(test_copy) {
    lval *err = lval_err(0, "err");
    lval *err2 = lval_copy(err);
    EXPECT_EQ(err->err, err2->err);
    lval_del(err2);
    lval_del(err);
    lval *builtin1 = lval_builtin(&lbuiltin1, "builtin1");
    lval *builtin2 = lval_copy(builtin1);
    EXPECT_EQ((void*)builtin1->builtin, builtin2->builtin);
    lval_del(builtin2);
    lval_del(builtin1);
    lval *num = lval_num(42);
    lval *num2 = lval_copy(num);
    EXPECT_EQ(num->num, num2->num);
    lval_del(num2);
    lval_del(num);
}
TESTMETHOD(test_builtin) {
    lenv *e = lenv_new();
    assert_repr(e, "+", "<function '+'>");
    assert_repr(e, "eval (head {5 10 11 15})", "5");
    assert_repr(e, "eval (head {+ - + - * /})", "<function '+'>");
    assert_repr(e, "(eval (head {+ - + - * /})) 10 20", "30");
    assert_error(e, "hello", LERR_UNBOUND_SYM);
    lenv_del(e);
}
TESTMETHOD(test_sexpr) {
    lenv *e = lenv_new();
    assert_repr(e, "", "()");
    lenv_del(e);
}
TESTMETHOD(test_qexpr) {
    lenv *e = lenv_new();
    assert_repr(e, "list 1 2 3 4", "{1 2 3 4}");
    assert_repr(e, "{head (list 1 2 3 4)}", "{head (list 1 2 3 4)}");
    assert_repr(e, "eval {head (list 1 2 3 4)}", "{1}");
    assert_repr(e, "tail {tail tail tail}", "{tail tail}");
    assert_repr(e, "eval (tail {tail tail {5 6 7}})", "{6 7}");
    assert_repr(e, "eval (head {(+ 1 2) (+ 10 20)})", "3");
    assert_repr(e, "join {tail 111 head} {222} {head 333 tail}", "{tail 111 head 222 head 333 tail}");
    assert_repr(e, "len {}", "0");
    assert_repr(e, "len {0 1 2 3 4 5}", "6");
    assert_repr(e, "cons 0 {}", "{0}");
    assert_repr(e, "cons 1 {2 {3}}", "{1 2 {3}}");
    assert_repr(e, "init {3 2 1 0}", "{3 2 1}");
    assert_repr(e, "head {0 1 2 3}", "{0}");
    lenv_del(e);
}
TESTMETHOD(test_number_err) {
    lenv *e = lenv_new();
    assert_error(e, "+ 9999999999999999999 1", LERR_BAD_NUM);
    assert_error(e, "+ 12 2a3", LERR_UNBOUND_SYM);
    lenv_del(e);
}
TESTMETHOD(test_div_zero) {
    lenv *e = lenv_new();
    assert_error(e, "/ 10 0", LERR_DIV_ZERO);
    lenv_del(e);
}
TESTMETHOD(test_flt_err) {
    lenv *e = lenv_new();
    assert_error(e, "+ 2 3.1", LERR_INVALID_OPERAND);
    assert_error(e, "+ 1.1 2.", LERR_PARSE_ERROR);
    lenv_del(e);
}
TESTMETHOD(test_flt) {
    lenv *e = lenv_new();
    assert_repr(e, "+ 1.2 1.3", "2.5");
    assert_repr(e, "* 2.0 3.14", "6.28");
    lenv_del(e);
}
TESTMETHOD(test_noeval) {
    lenv *e = lenv_new_no_builtins();
    assert_repr_noeval(e, "", "()");
    assert_repr_noeval(e, "+", "(+)");
    assert_error_noeval(e, "2,", LERR_PARSE_ERROR);
    assert_repr_noeval(e, "+ 2 2", "(+ 2 2)");
    assert_repr_noeval(e, "+ 2 2.1", "(+ 2 2.1)");
    assert_repr_noeval(e, "5", "(5)");
    lenv_del(e);
}
TESTMETHOD(test) {
    lenv *e = lenv_new();
    assert_repr(e, "+ 1 -2 6", "5");
    assert_repr(e, "max 1 5 3", "5");
    assert_repr(e, "min 1 5 3", "1");
    assert_repr(e, "^ 4 2", "16");
    assert_repr(e, "% 10 6", "4");
    assert_repr(e, "- (* 10 10) (+ 1 1 1)", "97");
    assert_repr(e, "* 10 (+ 1 51)", "520");
    assert_repr(e, "/ 42 2", "21");
    assert_repr(e, "* 1 2 6", "12");
    assert_repr(e, "+ 1 2 6", "9");
    lenv_del(e);
}
TESTMETHOD(test_sappendf) {
    char *p = 0;
    int plen = 0;
    sappendf(&p, &plen, "%s%c%s", "hello", ' ', "world");
    printf("p=[%s]\n", p);
    EXPECT_EQ(p, "hello world");
    free(p);
}
