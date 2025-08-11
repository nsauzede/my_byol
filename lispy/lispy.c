#include "../mpc/mpc.c"
/***************************************************************/
#include <math.h>
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
long ast_eval(mpc_ast_t *a) {
    if (strstr(a->tag, "number")) {
        return atoi(a->contents);
    }
    char *op = a->children[1]->contents;
    long ret = ast_eval(a->children[2]);
    for (int i = 3; i < a->children_num; i++) {
        if (!strstr(a->children[i]->tag, "expr"))
            break;
        long ret_ = ast_eval(a->children[i]);
        if (!strcmp(op, "*")) ret *= ret_;
        if (!strcmp(op, "/")) ret /= ret_;
        if (!strcmp(op, "+")) ret += ret_;
        if (!strcmp(op, "-")) ret -= ret_;
        if (!strcmp(op, "%")) ret %= ret_;
        if (!strcmp(op, "^")) ret = pow(ret, ret_);
        if (!strcmp(op, "min")) ret = ret>ret_?ret_:ret;
        if (!strcmp(op, "max")) ret = ret<ret_?ret_:ret;
    }
    return ret;
}
int ast_count(int count, mpc_ast_t *a) {
    for (int i = 0; i < a->children_num; i++) {
        count += ast_count(0, a->children[i]);
    }
    return count + 1;
}
char *parse(char *s) {
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
    char *res = 0;
    mpc_result_t r;
    if (mpc_parse("<stdin>", s, Lispy, &r)) {
        printf("\n");ast_print(0, r.output);
        //printf("=> counted %d nodes\n", ast_count(0, r.output));
        int len = 16;
        res = malloc(len);
        long ret = ast_eval(r.output);
        printf("%ld\n", ret);
//        printf("leaves=%d\n", ast_leaves(r.output));
        snprintf(res, len, "%ld", ret);
        //mpc_ast_print(r.output);
        mpc_ast_delete(r.output);
    } else {
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }
    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    return res;
}
int main() {
    char buf[512];
    while (1) {
        printf("lispy> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        char *endl = strchr(buf, '\n');
        if (endl) *endl = 0;
        //printf("You entered: [%s]\n", buf);
        char *res = parse(buf);
        if (res) free(res);
    }
    printf("Bye!\n");
    return 0;
}
