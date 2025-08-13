//#define DUMP_AST
#include "../mpc/mpc.c"
/***************************************************************/
const char *lispy_grammar =
        "number  : /-?[0-9]+/ ;\
        float   : /-?[0-9]+[.][0-9]+/ ;\
        symbol  : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" | \
                  \"list\" | \"eval\" | \"head\" | \"tail\" | \"join\" | \
                  \"len\" | \"cons\" | \"init\" ;\
        sexpr   : '(' <expr>* ')' ;\
        qexpr   : '{' <expr>* '}' ;\
        expr    : <float> | <number> | <symbol> | <sexpr> | <qexpr> ;\
        lispy   : /^/ <expr>* /$/ ;";
#include <math.h>
typedef struct lval {
    int type;
    long num;
    double flt;
    char *err;
    char *sym;
    // {S,Q}-expr
    int count;
    struct lval **cell;
} lval;
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FLT };
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
lval *lval_qexpr(void) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_QEXPR,.count=0,.cell=NULL};
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
        case LVAL_QEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }
    free(v);
}
int sappendf(char **p, int *plen, const char *fmt, ...) {
    if (!p || !plen)return -1;
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (n <= 0) { perror("vsnprintf"); return -1; }
    char *s = calloc(n + 1, 1);
    if (!s) { perror("calloc"); return -1; }
    va_start(args, fmt);
    vsnprintf(s, n + 1, fmt, args);
    va_end(args);
    if (!*p) {
        *p = calloc(1,1);
        if (!*p) { perror("calloc"); free(s); return -1; }
    }
    char *tmp = realloc(*p, *plen + n + 1);
    if (!tmp) { perror("calloc"); free(s); return -1; }
    *p = tmp;
    memcpy(*p + *plen, s, n + 1);
    *plen += n;
    free(s);
    return n;
}
void lval_repr_(char **s, int *slen, lval *v);
void lval_expr_repr(char **s, int *slen, lval *v, char open, char close) {
    sappendf(s, slen, "%c", open);
    for (int i = 0; i < v->count; i++) {
        lval_repr_(s, slen, v->cell[i]);
        if (i != v->count - 1) {
            sappendf(s, slen, " ");
        }
    }
    sappendf(s, slen, "%c", close);
}
void lval_repr_(char **s, int *slen, lval *v) {
    if (!v) return;
    switch (v->type) {
        case LVAL_NUM:sappendf(s, slen, "%li", v->num);break;
        case LVAL_FLT:sappendf(s, slen, "%f", v->flt);break;
        case LVAL_SYM:sappendf(s, slen, "%s", v->sym);break;
        case LVAL_ERR:sappendf(s, slen, "%s", v->err);break;
        case LVAL_SEXPR:lval_expr_repr(s, slen, v, '(', ')');break;
        case LVAL_QEXPR:lval_expr_repr(s, slen, v, '{', '}');break;
    }
}
char *lval_repr(lval *v) {
    char *repr = 0;
    int len = 0;
    lval_repr_(&repr, &len, v);
    return repr;
}
void lval_print(lval *v);
void lval_expr_print(lval *v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);
        if (i != v->count - 1) {
            putchar(' ');
        }
    }
    putchar(close);
}
void lval_print(lval *v) {
    if (!v) return;
    switch (v->type) {
        case LVAL_NUM:printf("%li", v->num);break;
        case LVAL_FLT:printf("%f", v->flt);break;
        case LVAL_SYM:printf("%s", v->sym);break;
        case LVAL_ERR:printf("Error: %s", v->err);break;
        case LVAL_SEXPR:lval_expr_print(v, '(', ')');break;
        case LVAL_QEXPR:lval_expr_print(v, '{', '}');break;
    }
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
    if (strstr(a->tag, "qexpr")) {
        ret = lval_qexpr();
    }
    for (int i = 0; i < a->children_num; i++) {
        if (!strcmp(a->children[i]->contents, "(")
            ||!strcmp(a->children[i]->contents, ")")
            ||!strcmp(a->children[i]->contents, "{")
            ||!strcmp(a->children[i]->contents, "}")
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
    mpc_parser_t *Qexpr = mpc_new("qexpr");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");
    mpca_lang(MPCA_LANG_DEFAULT, lispy_grammar,
        Number, Float, Symbol, Sexpr, Qexpr, Expr, Lispy
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
    mpc_cleanup(7, Number, Float, Symbol, Sexpr, Qexpr, Expr, Lispy);
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
lval *lval_join(lval *x, lval *y) {
    while (y->count > 0) {
        x = lval_add(x, lval_pop(y, 0));
    }
    lval_del(y);
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
        else if (!strcmp(op, "/")) {
            lval *fail = builtin_div_or_fail(x, y);
            if (fail) {
                lval_del(x); lval_del(y);
                x = fail;
                break;
            }
        }
        else if (!strcmp(op, "+")) builtin_add(x, y);
        else if (!strcmp(op, "-")) builtin_sub(x, y);
        else if (!strcmp(op, "%")) {
            lval *fail = builtin_mod_or_fail(x, y);
            if (fail) {
                lval_del(x); lval_del(y);
                x = fail;
                break;
            }
        }
        else if (!strcmp(op, "^")) builtin_pow(x, y);
        else if (!strcmp(op, "min")) builtin_min(x, y);
        else if (!strcmp(op, "max")) builtin_max(x, y);
        lval_del(y);
    }
    lval_del(v);
    return x;
}
#define LASSERT(v, cond, err) \
    do {if (!(cond)){lval_del(v); return lval_err(err);}} while(0)
lval *builtin_list(lval *v) {
    v->type = LVAL_QEXPR;
    return v;
}
lval *builtin_eval(lval *v) {
    LASSERT(v, v->count == 1, "Function 'eval' requires exactly one argument!");
    LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Function 'eval' requires Q-expr arg!");
    lval *x = lval_take(v, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(x);
}
lval *builtin_head(lval *v) {
    LASSERT(v, v->count == 1, "Function 'head' requires exactly one argument!");
    LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Function 'head' requires Q-expr arg!");
    LASSERT(v, v->cell[0]->count > 0, "Function 'head' requires non-empty Q-expr arg!");
    lval *x = lval_take(v, 0);
    while (x->count > 1) { lval_del(lval_pop(x, 1)); }
    return x;
}
lval *builtin_tail(lval *v) {
    LASSERT(v, v->count == 1, "Function 'tail' requires exactly one argument!");
    LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Function 'tail' requires Q-expr arg!");
    LASSERT(v, v->cell[0]->count > 0, "Function 'tail' requires non-empty Q-expr arg!");
    lval *x = lval_take(v, 0);
    lval_del(lval_pop(x, 0));
    return x;
}
lval *builtin_join(lval *v) {
    for (int i = 0; i < v->count; i++) {
        LASSERT(v, v->cell[i]->type == LVAL_QEXPR, "Function 'join' requires only Q-expr args!");
    }
    lval *x = lval_pop(v, 0);
    while (v->count > 0) {
        x = lval_join(x, lval_pop(v, 0));
    }
    lval_del(v);
    return x;
}
lval *builtin_len(lval *v) {
    LASSERT(v, v->count == 1, "Function 'len' requires exactly one argument!");
    LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Function 'len' requires Q-expr arg!");
    lval *x = lval_num(v->cell[0]->count);
    lval_del(v);
    return x;
}
lval *builtin_cons(lval *v) {
    LASSERT(v, v->count == 2, "Function 'cons' requires exactly two arguments!");
    LASSERT(v, v->cell[0]->type == LVAL_NUM, "Function 'cons' requires arg1 to be a number!");
    LASSERT(v, v->cell[1]->type == LVAL_QEXPR, "Function 'cons' requires arg2 to be a Q-expr!");
    lval *x = lval_pop(v, 0);
    lval *y = lval_pop(v, 0);
    lval *z = lval_qexpr();
    z = lval_add(z, x);
    while (y->count > 0) {
        z = lval_add(z, lval_pop(y, 0));
    }
    lval_del(y);
    lval_del(v);
    return z;
}
lval *builtin_init(lval *v) {
    LASSERT(v, v->count == 1, "Function 'init' requires exactly one argument!");
    LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Function 'init' requires Q-expr arg!");
    LASSERT(v, v->cell[0]->count > 0, "Function 'init' requires non-empty Q-expr arg!");
    lval *x = lval_take(v, 0);
    lval_del(lval_pop(x, x->count - 1));
    return x;
}
lval *builtin(lval *v, char *func) {
    if (!strcmp("list", func)) { return builtin_list(v); }
    else if (!strcmp("eval", func)) { return builtin_eval(v); }
    else if (!strcmp("head", func)) { return builtin_head(v); }
    else if (!strcmp("tail", func)) { return builtin_tail(v); }
    else if (!strcmp("join", func)) { return builtin_join(v); }
    else if (!strcmp("len", func)) { return builtin_len(v); }
    else if (!strcmp("cons", func)) { return builtin_cons(v); }
    else if (!strcmp("init", func)) { return builtin_init(v); }
    else if (strstr("+-/*%^minmax", func)) { return builtin_op(v, func); }
    lval_del(v);
    return lval_err("Unknown Builtin Function!");
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
    lval *result = builtin(v, f->sym);
    lval_del(f);
    return result;
}
lval *lread(char *s) {
    mpc_result_t r;
    lval *res = NULL;
    r.output = parse_lispy(s);
    if (r.output) {
        res = lval_read(r.output);
        mpc_ast_delete(r.output);
    } else {
        res = lval_err(lerrors[LERR_PARSE_ERROR]);
    }
    return res;
}
lval *eval(char *s) {
    lval *res = lread(s);
    if (res->type == LVAL_ERR) return res;
    return lval_eval_sexpr(res);
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
