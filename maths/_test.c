#include "../mpc/mpc.c"
/***************************************************************/
#include "ut/ut.h"
mpc_val_t *fold_maths(int n, mpc_val_t **xs) {
    int **vs = (int**)xs;
    if (strcmp(xs[1], "*") == 0) { *vs[0] *= *vs[2]; }
    if (strcmp(xs[1], "/") == 0) { *vs[0] /= *vs[2]; }
    if (strcmp(xs[1], "+") == 0) { *vs[0] += *vs[2]; }
    if (strcmp(xs[1], "-") == 0) { *vs[0] -= *vs[2]; }
    free(xs[1]); free(xs[2]);
    return xs[0];
}
static void* read_line(void* line) {
  printf("Reading Line: %s", (char*)line);
  return line;
}
TESTMETHOD(ztest3) {
 const char *input =
   "abcHVwufvyuevuy3y436782\n"
   "\n"
   "\n"
   "rehre\n"
   "rew\n"
   "-ql.;qa\n"
   "eg";
 mpc_parser_t* Line = mpc_many(
   mpcf_strfold,
   mpc_apply(mpc_re("[^\\n]*(\\n|$)"), read_line));

 mpc_result_t r;
 mpc_parse("input", input, Line, &r);
 printf("\nParsed String: %s", (char*)r.output);
 free(r.output);

 mpc_delete(Line);
}
TESTMETHOD(test2) {
    mpc_result_t r;
    mpc_parser_t* Foobar;
    Foobar = mpc_new("foobar");
    mpca_lang(MPCA_LANG_DEFAULT, "foobar : \"foo\" | \"bar\";", Foobar);
    if (mpc_parse("<stdin>", "foo", Foobar, &r)) {
        mpc_ast_print(r.output);
        mpc_ast_delete(r.output);
    } else {
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }
    mpc_cleanup(1, Foobar);
}
TESTMETHOD(test1) {
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Factor = mpc_new("factor");
    mpc_parser_t *Term = mpc_new("term");
    mpc_parser_t *Maths = mpc_new("maths");
    mpc_define(Expr, mpc_or(2,
        mpc_and(3, fold_maths,
            Factor, mpc_oneof("+-"), Factor,
            free, free),
        Factor
    ));
    mpc_define(Factor, mpc_or(2,
        mpc_and(3, fold_maths,
            Term, mpc_oneof("*/"), Term,
            free, free),
        Term
    ));
    mpc_define(Term, mpc_or(2, mpc_int(), mpc_parens(Expr, free)));
    mpc_define(Maths, mpc_whole(Expr, free));
#if 1
    mpc_result_t r;
    if (mpc_parse("<stdin>", "3*2", Maths, &r)) {
        int res = *(int *)r.output;
        printf("res=%d\n", res);
        ASSERT_EQ(6, res);
        free(r.output);
    } else {
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }
#endif
    //mpc_delete(Maths);
    mpc_cleanup(4, Expr, Factor, Term, Maths);
    ASSERT(1);
}
