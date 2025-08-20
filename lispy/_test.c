#define main themain
#include "lispy.c"
#undef main
/***************************************************************/
#include "ut/ut.h"
void assert_repr_lval_(lval *v, const char *expected, const char *input) {
    ASSERT(!!v);
    char *repr = lval_repr(v);
    EXPECT_EQ(repr, expected, input);
    free(repr);
}
void assert_repr_(int read_not_eval, lenv *e, const char *input, const char *expected) {
    lval *res = read_not_eval ? lread(input) : eval(e, input);
    assert_repr_lval_(res, expected, input);
    lval_del(res);
}
#define assert_repr_lval(v,expected) assert_repr_lval_(v,expected,JOIN2(v,expected))
#define assert_repr(e,input,expected) assert_repr_(0,e,input,expected)
#define assert_repr_noeval(e,input,expected) assert_repr_(1,e,input,expected)
void assert_error_lval(lval *v, int expected) {
    ASSERT(!!v);
    EXPECT_EQ(LVAL_ERR, v->type);
    EXPECT_EQ(expected, v->errcode);
}
void assert_error_(int read_not_eval, lenv *e, const char *input, int expected) {
    lval *res = read_not_eval ? lread(input) : eval(e,input);
    assert_error_lval(res, expected);
    lval_del(res);
}
#define assert_error(e,input,expected) assert_error_(0,e,input,expected)
#define assert_error_noeval(e,input,expected) assert_error_(1,e,input,expected)
TESTMETHOD(test_comment) {
    lenv *e = lenv_new();
    assert_repr(e, "\"hello\" ; world", "\"hello\"");
    lenv_del(e);
}
TESTMETHOD(test_str) {
    lenv *e = lenv_new();
    assert_repr(e, "\"hello\"", "\"hello\"");
    assert_repr(e, "\"hello\\n\"", "\"hello\\n\"");
    assert_repr(e, "\"hello\\\"\"", "\"hello\\\"\"");
    assert_repr(e, "head {\"hello\" \"world\"}", "{\"hello\"}");
    assert_repr(e, "eval (head {\"hello\" \"world\"})", "\"hello\"");
    assert_repr(e, "def {a b} \"foo\" \"bar\"", "()");
    assert_repr(e, "b", "\"bar\"");
    lenv_del(e);
}
TESTMETHOD(test_logic) {
    lenv *e = lenv_new();
    assert_repr(e, "def {a b} 1 0", "()");
    assert_repr(e, "if (== a b) {1} {2}", "2");
    assert_repr(e, "|| a b", "true");
    assert_repr(e, "&& a b", "false");
    assert_repr(e, "|| (- 1 1) (+ 1 -1)", "false");
    assert_repr(e, "! 0", "true");
    assert_repr(e, "! 1", "false");
    assert_error(e, "! 1 2", LERR_INVALID_OPERAND);
    assert_error(e, "! 3.14", LERR_INVALID_OPERAND);
    assert_repr(e, "fun {or_ a b} {|| a b}", "()");
    assert_repr(e, "or_ a b", "true");
    assert_repr(e, "fun {and_ a b} {&& a b}", "()");
    assert_repr(e, "and_ a b", "false");
    assert_repr(e, "fun {not_ a} {! a}", "()");
    assert_repr(e, "not_ 0", "true");
    assert_repr(e, "not_ 1", "false");
    assert_repr(e, "! true", "false");
    assert_repr(e, "! false", "true");
    assert_repr(e, "! ()", "true");
    assert_repr(e, "! (! ())", "false");
    lenv_del(e);
}
TESTMETHOD(test_recurse) {
    lenv *e = lenv_new();
    assert_repr(e, "fun {len_ l} {if (== l {}) {0} {+ 1 (len (tail l))}}", "()");
    assert_repr(e, "len_ {1 2 3 4}", "4");
    assert_repr(e, "fun {reverse_ l} {if (== l {}) {{}} {join (reverse_ (tail l)) (head l)}}", "()");
    assert_repr(e, "reverse_ {5 6 7 8}", "{8 7 6 5}");
    assert_repr(e, "fun {nth_ n l} {if (== n 0) {eval (head l)} {nth_ (- n 1) (tail l)}}", "()");
    assert_repr(e, "nth_ 1 {5 6 7 8}", "6");
    assert_repr(e, "fun {in_ n l} {if (== 0 (len_ l)) {false} {if (== n (eval (head l))) {true} {in_ n (tail l)}}}", "()");
    assert_repr(e, "in_ 42 {5 6 7 8}", "false");
    assert_repr(e, "in_ 42 {5 6 42 8}", "true");
    assert_repr(e, "fun {last_ l} {if (> (len_ l) 1) {last_ (tail l)} {if (== 1 (len_ l)) {eval l} {()}}}", "()");
    assert_repr(e, "last_ {5 6 7 8}", "8");
    lenv_del(e);
}
TESTMETHOD(test_ord) {
    lenv *e = lenv_new();
    assert_repr(e, "> 10 5", "true");
    assert_repr(e, "> 5 10", "false");
    assert_repr(e, "<= 88 5", "false");
    assert_repr(e, "== 5 6", "false");
    assert_repr(e, "== 5 {}", "false");
    assert_repr(e, "== 1 1", "true");
    assert_repr(e, "!= {} 56", "true");
    assert_repr(e, "== {1 2 3 {5 6}} {1   2  3   {5 6}}", "true");
    assert_repr(e, "def {x y} 100 200", "()");
    assert_repr(e, "== x y", "false");
    assert_repr(e, "!= x y", "true");
    assert_repr(e, "if (== x y) {+ x y} {- x y}", "-100");
    assert_repr(e, "if (!= x y) {+ x y} {- x y}", "300");
    lenv_del(e);
}
TESTMETHOD(test_currying) {
    lenv *e = lenv_new();
    assert_repr(e, "def {fun_} (\\ {args body} {def (head args) (\\ (tail args) body)})", "()");
    assert_repr(e, "fun_ {unpack_ f xs} {eval (join (list f) xs)}", "()");
    assert_repr(e, "fun_ {pack_ f & xs} {f xs}", "()");
    assert_repr(e, "def {uncurry_} pack_", "()");
    assert_repr(e, "def {curry_} unpack_", "()");
    assert_repr(e, "curry_ + {5 6 7}", "18");
    assert_repr(e, "uncurry_ head 5 6 7", "{5}");
    assert_repr(e, "def {add-uncurried_} +", "()");
    assert_repr(e, "def {add-curried_} (curry_ +)", "()");
    assert_repr(e, "add-curried_ {5 6 7}", "18");
    assert_repr(e, "add-uncurried_ 5 6 7", "18");

    assert_repr(e, "def {head-curried_} head", "()");
    assert_repr(e, "head-curried_ {5 6 7}", "{5}");
//    assert_repr(e, "def {head-uncurried_} (uncurry_ head)", "()");
//    assert_repr(e, "def {head-uncurried_} {uncurry_ head}", "()");
//    assert_repr(e, "head-uncurried_ 5 6 7", "{5}");
    lenv_del(e);
}
TESTMETHOD(test_fun) {
    lenv *e = lenv_new();
    assert_repr(e, "def {fun_} (\\ {args body} {def (head args) (\\ (tail args) body)})", "()");
    assert_repr(e, "fun_ {add-together_ x y} {+ x y}", "()");
    assert_repr(e, "add-together_ 1 2", "3");
    assert_error(e, "add-together_ 1 2 3", LERR_INVALID_OPERAND);

    assert_repr(e, "fun_ {unpack_ f xs} {eval (join (list f) xs)}", "()");
    assert_repr(e, "fun_ {first_ x} {eval (head x)}", "()");
    assert_repr(e, "first_ {10 11 12}", "10");
    assert_repr(e, "fun_ {second_ x} {eval (head (tail x))}", "()");
    assert_repr(e, "second_ {10 11 12}", "11");
    assert_repr(e, "- 5 3", "2");
    assert_repr(e, "- 3 5", "-2");
    assert_repr(e, "fun_ {swap_ f x y} {f y x}", "()");
    assert_repr(e, "swap_ - 5 3", "-2");
    assert_repr(e, "fun_ {double_ x} {* 2 x}", "()");
    assert_repr(e, "double_ 5", "10");
    assert_repr(e, "fun_ {compute_ x y} {double_ (swap_ - x y)}", "()");
    assert_repr(e, "compute_ 10 5", "-10");
    assert_repr(e, "fun_ {third_ x} {eval (head (tail (tail x)))}", "()");
    assert_repr(e, "third_ {10 11 12 13}", "12");
    lenv_del(e);
}
TESTMETHOD(test_lambda) {
    lenv *e = lenv_new();
    assert_repr(e, "\\ {x y} {+ x y}", "(\\ {x y} {+ x y})");
    assert_repr(e, "def {add-mul_} (\\ {x y} {+ x (* x y)})", "()");
    assert_repr(e, "add-mul_ 10 20", "210");
    assert_repr(e, "add-mul_ 10", "(\\ {y} {+ x (* x y)})");
    assert_repr(e, "def {add-mul-ten_} (add-mul_ 10)", "()");
    assert_repr(e, "add-mul-ten_ 50", "510");
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
    lval *v = lval_num(42+0);
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
