//#define DUMP_AST
#include "../mpc/mpc.c"
/***************************************************************/
#include <math.h>
typedef struct lval {
    int type;
    long num;
    char *err;
    char *sym;
    // sexpr
    int count;
    struct lval **cell;
} lval;
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };
enum { LERR_DIV_ZERO, LERR_BAD_SYM, LERR_BAD_NUM, LERR_PARSE_ERROR };
lval *lval_num(long num) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_NUM,.num=num};
    return ret;
}
lval *lval_sym(char *sym) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_SYM,.sym=malloc(strlen(sym) + 1)};
    strcpy(ret->sym, sym);
    return ret;
}
lval *lval_sexpr(void) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_SEXPR,.count=0,.cell=NULL};
    return ret;
}
lval *lval_err(const char *err) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_ERR,.err=malloc(strlen(err) + 1)};
    strcpy(ret->err, err);
    return ret;
}
void lval_del(lval *v) {
    switch (v->type) {
        case LVAL_NUM:break;
        case LVAL_SYM:
            free(v->sym);
            break;
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }
    free(v);
}
static const char *lerrors[] = {
    [LERR_DIV_ZERO] = "Division By Zero!",
    [LERR_BAD_NUM] = "Invalid Number!",
};
void lval_sexpr_print(lval *v, char open, char close);
void lval_print(lval *v) {
    if (!v) return;
    switch (v->type) {
        case LVAL_NUM:printf("%li", v->num);break;
        case LVAL_SYM:printf("%s", v->sym);break;
        case LVAL_ERR:printf("Error: %s", v->err);break;
        case LVAL_SEXPR:lval_sexpr_print(v, '(', ')');break;
    }
}
void lval_sexpr_print(lval *v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);
        if (i != v->count - 1) {
            putchar(' ');
        }
    }
    putchar(close);
}
void lval_println(lval *v) {
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
lval *lval_read_num(mpc_ast_t *a) {
    errno = 0;
    long num = strtol(a->contents, NULL, 10);
    return errno==0?lval_num(num):lval_err(lerrors[LERR_BAD_NUM]);
}
lval *lval_add(lval *v, lval *x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}
lval *lval_read(mpc_ast_t *a) {
    if (strstr(a->tag, "number")) {
        return lval_read_num(a);
    }
    if (strstr(a->tag, "symbol")) {
        return lval_sym(a->contents);
    }
    lval *ret = NULL;
    if (!strcmp(a->tag, ">")||strstr(a->tag, "sexpr")) {
        ret = lval_sexpr();
    }
    for (int i = 0; i < a->children_num; i++) {
        if (!strcmp(a->children[i]->contents, "(")
            ||!strcmp(a->children[i]->contents, ")")
            ||!strcmp(a->children[i]->tag, "regex")) {
            continue;
        }
        ret = lval_add(ret, lval_read(a->children[i]));
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
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");
    mpca_lang(MPCA_LANG_DEFAULT,
        "number : /-?[0-9a]+/ ;\
        symbol  : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;\
        sexpr   : '(' <expr>* ')' ;\
        expr    : <number> | <symbol> | <sexpr> ;\
        lispy   : /^/ <expr>* /$/ ;",
        Number, Symbol, Sexpr, Expr, Lispy
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
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
    return res;
}
lval *lval_pop(lval *v, int i) {
    lval *x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count - i - 1));
    v->count--;
    if (v->count == 0) {
        free(v->cell);  // Valgrind doesn't like zero-realloc ?
        v->cell = NULL;
    } else {
        v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    }
    return x;
}
lval *lval_take(lval *v, int i) {
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}
lval *lval_eval_sexpr(lval *v);
lval *lval_eval(lval *v) {
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(v);
    }
    return v;
}
lval *builtin_op(lval *v, char *op) {
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type != LVAL_NUM) {
            lval_del(v);
            return lval_err("Cannot operate on non-number!");
        }
    }
    lval *x = lval_pop(v, 0);
    if (!strcmp(op, "-") && v->count == 0) {
        x->num = -x->num;
    }
    while (v->count > 0) {
        lval *y = lval_pop(v, 0);
        if (!strcmp(op, "*")) x->num *= y->num;
        if (!strcmp(op, "/")) {
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err(lerrors[LERR_DIV_ZERO]);
                break;
            }
            x->num /= y->num;
        }
        if (!strcmp(op, "+")) x->num += y->num;
        if (!strcmp(op, "-")) x->num -= y->num;
        if (!strcmp(op, "%")) x->num %= y->num;
        if (!strcmp(op, "^")) x->num = pow(x->num, y->num);
        if (!strcmp(op, "min")) x->num = x->num>y->num?y->num:x->num;
        if (!strcmp(op, "max")) x->num = x->num<y->num?y->num:x->num;
        lval_del(y);
    }
    lval_del(v);
    return x;
}
lval *lval_eval_sexpr(lval *v) {
    if (v->type == LVAL_ERR) return v;
    if (v->count == 0) return v;
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }
    if (v->count == 1) return lval_take(v, 0);
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f); lval_del(v);
        return lval_err("S-expression Does not start with symbol!");
    }
    lval *result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}
lval *eval(char *s) {
    mpc_result_t r;
    lval *res = NULL;
    r.output = parse_lispy(s);
    if (r.output) {
        res = lval_read(r.output);
        mpc_ast_delete(r.output);
        res = lval_eval_sexpr(res);
    } else {
        res = lval_err("parse error");
    }
    return res;
}
char *eval_print(char *s) {
    char *res = 0;
    lval *ret = eval(s);
    if (!ret) return 0;
    lval_println(ret);
    if (ret->type == LVAL_NUM) {
        int len = 16;
        res = malloc(len);
        snprintf(res, len, "%ld", ret->num);
    } else {
        int len = 32;
        res = malloc(len);
        snprintf(res, len, "err:%s", ret->err);
    }
    lval_del(ret);
    return res;
}
int main() {
    char buf[512];
    while (1) {
        printf("lispy> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        char *endl = strchr(buf, '\n');
        if (endl) *endl = 0;
        char *res = eval_print(buf);
        if (res) free(res);
    }
    printf("Bye!\n");
    return 0;
}
