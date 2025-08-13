//#define DUMP_AST
#include "../mpc/mpc.c"
/***************************************************************/
#include <math.h>
typedef struct lval {
    int type;
    long num;
    double flt;
    char *err;
    char *sym;
    // sexpr
    int count;
    struct lval **cell;
} lval;
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_FLT };
enum { LERR_DIV_ZERO, LERR_BAD_SYM, LERR_BAD_NUM, LERR_BAD_FLT, LERR_PARSE_ERROR, LERR_INVALID_OPERAND };
const char *lerrors[] = {
    [LERR_DIV_ZERO] = "Division By Zero!",
    [LERR_BAD_NUM] = "Invalid Number!",
    [LERR_BAD_FLT] = "Invalid Float!",
    [LERR_INVALID_OPERAND] = "Invalid Operand!",
    [LERR_PARSE_ERROR] = "Parse Error!",
};
lval *lval_num(long num) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_NUM,.num=num};
    return ret;
}
lval *lval_flt(double flt) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_FLT,.flt=flt};
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
        case LVAL_FLT:break;
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
void lval_sexpr_print(lval *v, char open, char close);
void lval_print(lval *v) {
    if (!v) return;
    switch (v->type) {
        case LVAL_NUM:printf("%li", v->num);break;
        case LVAL_FLT:printf("%f", v->flt);break;
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
lval *lval_read_flt(mpc_ast_t *a) {
    errno = 0;
    double flt = strtod(a->contents, NULL);
    return errno==0?lval_flt(flt):lval_err(lerrors[LERR_BAD_FLT]);
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
    if (strstr(a->tag, "float")) {
        return lval_read_flt(a);
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
    mpc_parser_t *Float = mpc_new("float");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");
    mpca_lang(MPCA_LANG_DEFAULT,
        "number  : /-?[0-9]+/ ;\
        float   : /-?[0-9]+[.][0-9]+/ ;\
        symbol  : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ;\
        sexpr   : '(' <expr>* ')' ;\
        expr    : <float> | <number> | <symbol> | <sexpr> ;\
        lispy   : /^/ <expr>* /$/ ;",
        Number, Float, Symbol, Sexpr, Expr, Lispy
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
    mpc_cleanup(6, Number, Float, Symbol, Sexpr, Expr, Lispy);
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
void builtin_neg(lval *x) {
    if (x->type == LVAL_FLT) x->flt = -x->flt;
    else x->num = -x->num;
}
void builtin_mul(lval *x, lval *y) {
    if (x->type == LVAL_FLT) x->flt *= y->flt;
    else x->num *= y->num;
}
void builtin_add(lval *x, lval *y) {
    if (x->type == LVAL_FLT) x->flt += y->flt;
    else x->num += y->num;
}
void builtin_sub(lval *x, lval *y) {
    if (x->type == LVAL_FLT) x->flt -= y->flt;
    else x->num -= y->num;
}
lval *builtin_mod_or_fail(lval *x, lval *y) {
    lval *fail = 0;
    if (x->type == LVAL_FLT) fail = lval_err(lerrors[LERR_INVALID_OPERAND]);
    else x->num %= y->num;
    return fail;
}
void builtin_pow(lval *x, lval *y) {
    if (x->type == LVAL_FLT) pow(x->flt, y->flt);
    else x->num = pow(x->num, y->num);
}
void builtin_min(lval *x, lval *y) {
    if (x->type == LVAL_FLT) x->flt = x->flt>y->flt?y->flt:x->flt;
    else x->num = x->num>y->num?y->num:x->num;
}
void builtin_max(lval *x, lval *y) {
    if (x->type == LVAL_FLT) x->flt = x->flt<y->flt?y->flt:x->flt;
    else x->num = x->num<y->num?y->num:x->num;
}
lval *builtin_div_or_fail(lval *x, lval *y) {
    lval *fail = 0;
    if (x->type == LVAL_FLT) {
        if (y->flt == 0) {
            fail = lval_err(lerrors[LERR_DIV_ZERO]);
        } else x->flt /= y->flt;
    } else {
        if (y->num == 0) {
            fail = lval_err(lerrors[LERR_DIV_ZERO]);
        } else x->num /= y->num;
    }
    return fail;
}
lval *builtin_op(lval *v, char *op) {
    int flt = 0;
    if (v->cell[0]->type == LVAL_FLT) {
        flt = 1;
    }
    for (int i = 0; i < v->count; i++) {
        if (flt) {
            if (v->cell[i]->type != LVAL_FLT) {
                lval_del(v);
                return lval_err(lerrors[LERR_INVALID_OPERAND]);
            }
        } else {
            if (v->cell[i]->type != LVAL_NUM) {
                lval_del(v);
                return lval_err(lerrors[LERR_INVALID_OPERAND]);
            }
        }
    }
    lval *x = lval_pop(v, 0);
    if (!strcmp(op, "-") && v->count == 0) builtin_neg(x);
    while (v->count > 0) {
        lval *y = lval_pop(v, 0);
        if (!strcmp(op, "*")) builtin_mul(x, y);
        if (!strcmp(op, "/")) {
            lval *fail = builtin_div_or_fail(x, y);
            if (fail) {
                lval_del(x); lval_del(y);
                x = fail;
                break;
            }
        }
        if (!strcmp(op, "+")) builtin_add(x, y);
        if (!strcmp(op, "-")) builtin_sub(x, y);
        if (!strcmp(op, "%")) {
            lval *fail = builtin_mod_or_fail(x, y);
            if (fail) {
                lval_del(x); lval_del(y);
                x = fail;
                break;
            }
        }
        if (!strcmp(op, "^")) builtin_pow(x, y);
        if (!strcmp(op, "min")) builtin_min(x, y);
        if (!strcmp(op, "max")) builtin_max(x, y);
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
        res = lval_err(lerrors[LERR_PARSE_ERROR]);
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
