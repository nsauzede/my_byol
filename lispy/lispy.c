#include "../mpc/mpc.c"
/***************************************************************/
#include <math.h>
//#define DUMP_AST
typedef struct {
    int type;
    union {
        long num;
        int err;
    };
} lval;
enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM, LERR_PARSE_ERROR };
lval lval_num(long num) {
    lval v;
    v.type = LVAL_NUM;
    v.num = num;
    return v;
}
lval lval_err(int err) {
    lval v;
    v.type = LVAL_ERR;
    v.err = err;
    return v;
}
void lval_print(lval v) {
    switch (v.type) {
        case LVAL_NUM:
            printf("%li", v.num);
            break;
        case LVAL_ERR:
            switch (v.err) {
                case LERR_DIV_ZERO:
                    printf("Error: Division By Zero!");
                    break;
                case LERR_BAD_OP:
                    printf("Error: Invalid Operator!");
                    break;
                case LERR_BAD_NUM:
                    printf("Error: Invalid Number!");
                    break;
                case LERR_PARSE_ERROR:
                    printf("Error: Parse Error!");
                    break;
            }
            break;
    }
}
void lval_println(lval v) {
    lval_print(v);
    puts("");
}
void ast_print(int level, mpc_ast_t *a) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
    printf("%s", a->tag);
    if (strlen(a->contents)) {
        printf(":%ld:%ld", a->state.row+1, a->state.col+1);
        printf(" '%s'", a->contents);
    }
    printf("\n");
    for (int i = 0; i < a->children_num; i++) {
        ast_print(level + 1, a->children[i]);
    }
}
lval ast_eval(mpc_ast_t *a) {
    if (strstr(a->tag, "number")) {
        return lval_num(atoi(a->contents));
    }
    char *op = a->children[1]->contents;
    lval ret = ast_eval(a->children[2]);
    if (ret.type == LVAL_ERR) return ret;
    for (int i = 3; i < a->children_num; i++) {
        if (!strstr(a->children[i]->tag, "expr"))
            break;
        lval ret_ = ast_eval(a->children[i]);
        if (ret_.type == LVAL_ERR) return ret_;
        if (!strcmp(op, "*")) ret.num *= ret_.num;
        if (!strcmp(op, "/")) {
            ret = ret_.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(ret.num / ret_.num);
        }
        if (!strcmp(op, "+")) ret.num += ret_.num;
        if (!strcmp(op, "-")) ret.num -= ret_.num;
        if (!strcmp(op, "%")) ret.num %= ret_.num;
        if (!strcmp(op, "^")) ret.num = pow(ret.num, ret_.num);
        if (!strcmp(op, "min")) ret.num = ret.num>ret_.num?ret_.num:ret.num;
        if (!strcmp(op, "max")) ret.num = ret.num<ret_.num?ret_.num:ret.num;
    }
    return ret;
}
int ast_count(int count, mpc_ast_t *a) {
    for (int i = 0; i < a->children_num; i++) {
        count += ast_count(0, a->children[i]);
    }
    return count + 1;
}
void *parse_lispy(char *s) {
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");
    mpca_lang(MPCA_LANG_DEFAULT,
        "number  : /-?[0-9]+/ ;\
        operator: '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;\
        expr    : <number> | '(' <operator> <expr>+ ')' ;\
        lispy   : /^/ <operator> <expr>+ /$/ ;",
        Number, Operator, Expr, Lispy
    );
    void *res = 0;
    mpc_result_t r;
    if (mpc_parse("<stdin>", s, Lispy, &r)) {
#ifdef DUMP_AST
        printf("\n");ast_print(0, r.output);
#endif
        res = r.output;
    } else {
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }
    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    return res;
}
lval eval(char *s) {
    mpc_result_t r;
    lval res = lval_err(LERR_PARSE_ERROR);
    r.output = parse_lispy(s);
    if (r.output) {
        res = ast_eval(r.output);
        mpc_ast_delete(r.output);
    }
    return res;
}
char *parse(char *s) {
    char *res = 0;
    lval ret = eval(s);
    lval_println(ret);
    if (ret.type == LVAL_NUM) {
        int len = 16;
        res = malloc(len);
        snprintf(res, len, "%ld", ret.num);
    } else {
        int len = 16;
        res = malloc(len);
        snprintf(res, len, "err:%d", ret.err);
    }
    return res;
}
int main() {
    char buf[512];
    while (1) {
        printf("lispy> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        char *endl = strchr(buf, '\n');
        if (endl) *endl = 0;
        char *res = parse(buf);
        if (res) free(res);
    }
    printf("Bye!\n");
    return 0;
}
