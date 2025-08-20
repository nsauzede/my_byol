#include "../mpc/mpc.c"
/***************************************************************/
#include <math.h>
#define BRK() do{__asm__ volatile("int $3");}while(0)
#define TODO() do{printf("%s: TODO\n", __func__);BRK();exit(66);}while(0)
#define FIXME() do{printf("%s: FIXME\n", __func__);BRK();exit(66);}while(0)
#define LASSERT(v, cond, fmt, ...) do {if (!(cond)){lval *err = lval_err(LERR_INVALID_OPERAND, fmt, ##__VA_ARGS__);lval_del(v); return err;}} while(0)
#define LASSERT_NUM(builtin,v,n) LASSERT(v,v->count==n,"Function '%s' requires exactly %d argument%s! (%d)",builtin,n,n>1?"s":"",v->count)
#define LASSERT_MIN(builtin,v,n) LASSERT(v,v->count>=n,"Function '%s' requires at least %d argument%s! (%d)",builtin,n,n>1?"s":"",v->count)
#define LASSERT_INUM(builtin,v,i,n) LASSERT(v,v->cell[i]->count==n,"Function '%s' argument %d size must be exactly %d! (%d)",builtin,i,n,v->cell[i]->count)
#define LASSERT_IMIN(builtin,v,i,n) LASSERT(v,v->cell[i]->count>=n,"Function '%s' argument %d size must be at least %d! (%d)",builtin,i,n,v->cell[i]->count)
#define LASSERT_ITYPE(builtin,v,i,t) LASSERT(v,v->cell[i]->type==t,"Function '%s' argument %d type must be %s! (%s)",builtin,i,ltype_name(t),ltype_name(v->cell[i]->type))
const char *lispy_grammar = "\
        number  : /-?[0-9]+/ ;\
        float   : /-?[0-9]+[.][0-9]+/ ;\
        symbol  : /[a-zA-Z0-9_+\\-*\\/^%\\\\=<>!&]+/ ;\
        sexpr   : '(' <expr>* ')' ;\
        qexpr   : '{' <expr>* '}' ;\
        expr    : <float> | <number> | <symbol> | <sexpr> | <qexpr> ;\
        lispy   : /^/ <expr>* /$/ ;";
typedef struct lval lval;
typedef struct lenv lenv;
typedef lval *(*lbuiltin)(lenv *, lval *);
typedef enum {
    LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN,
    LVAL_SEXPR, LVAL_QEXPR, LVAL_FLT } lval_type_t;
typedef enum {
    LERR_UNSPECIFIED, LERR_FIRST = LERR_UNSPECIFIED,
        LERR_PARSE_ERROR, LERR_BAD_NUM, LERR_BAD_FLT,
        LERR_INVALID_OPERAND, LERR_DIV_ZERO, LERR_UNBOUND_SYM,
    LERR_LAST} lerr_t;
struct lval {
    lval_type_t type;
    /* Basic */
    long num;
    double flt;
    char *err;
    lerr_t errcode;
    char *sym;
    /* Function */
    lbuiltin builtin;
    lenv *env;
    lval *formals;
    lval *body;
    /* Expression */
    int count;
    struct lval **cell;
};
struct lenv {
    lenv *par;
    int count;
    char **syms;
    lval **vals;
};
const char *lerrors[] = {
    [LERR_UNSPECIFIED] = "(unspecified error)",
    [LERR_PARSE_ERROR] = "Parse Error!",
    [LERR_BAD_NUM] = "Invalid Number!",
    [LERR_BAD_FLT] = "Invalid Float!",
    [LERR_INVALID_OPERAND] = "Invalid Operand!",
    [LERR_DIV_ZERO] = "Division By Zero!",
    [LERR_UNBOUND_SYM] = "Unbound Symbol!",
};
const char *ltype_name(int t) {
    switch (t) {
        case LVAL_ERR:return "Error";
        case LVAL_NUM:return "Number";
        case LVAL_SYM:return "Symbol";
        case LVAL_FUN:return "Function";
        case LVAL_SEXPR:return "S-Expression";
        case LVAL_QEXPR:return "Q-Expression";
        case LVAL_FLT:return "Float";
        default: return "Unknown?";
    }
}
lenv *lenv_new(void);
lenv *lenv_copy(lenv* e);
void lenv_del(lenv *e);
lval *lval_void(void) { return calloc(1,sizeof(lval)); }
lval *lval_err(int errcode, const char *fmt, ...) {
    if (errcode < LERR_FIRST || errcode >= LERR_LAST) {
        printf("%s: invalid errcode=%d\n", __func__, errcode);
        exit(1);
    }
    char *err = 0;
    if (!fmt) {
        err = strdup(lerrors[errcode]);
    } else {
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(0, 0, fmt, args);
        if (len <= 0) {
            printf("%s: vsnprintf returned %d\n", __func__, len);
            exit(1);
        }
        va_end(args);
        err = malloc(++len);
        va_start(args, fmt);
        vsnprintf(err, len, fmt, args);
        va_end(args);
    }
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_ERR,.err=err,.errcode=errcode};
    return ret;
}
#define lval_err_oper(fmt, ...) lval_err(LERR_INVALID_OPERAND,fmt,##__VA_ARGS__)
#define lval_errcode(errcode) lval_err(errcode,0)
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
lval *lval_builtin(lbuiltin builtin, char *sym) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_FUN,.builtin=builtin,.sym=strdup(sym)};
    return ret;
}
lval *lval_lambda(lval *formals, lval *body) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_FUN,.env=lenv_new(),
        .formals=formals,.body=body};
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
lval *lval_flt(double flt) {
    lval *ret = malloc(sizeof(lval));
    *ret = (lval){LVAL_FLT,.flt=flt};
    return ret;
}
void lval_del(lval *v) {
    if (!v)return;
    switch (v->type) {
        case LVAL_NUM:
        case LVAL_FLT:
            break;
        case LVAL_FUN:
            if (!v->builtin) {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
                break;
            } /* fall-through to LVAL_SYM */
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
lval *lval_copy(lval *v) {
    lval *x = calloc(1, sizeof(lval));
    x->type = v->type;
    switch (v->type) {
        case LVAL_ERR:x->err = strdup(v->err);break;
        case LVAL_NUM:x->num = v->num;break;
        case LVAL_FLT:x->flt = v->flt;break;
        case LVAL_FUN:
            x->builtin = v->builtin;
            if (!v->builtin) {
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
                break;
            } /* fall through LVAL_SYM */
        case LVAL_SYM:
            x->sym = strdup(v->sym);break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }
    return x;
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
        case LVAL_FUN:
            if (v->builtin) {
                sappendf(s, slen, "<function '%s'>", v->sym);
            } else {
                sappendf(s, slen, "(\\ ");
                lval_repr_(s, slen, v->formals);
                sappendf(s, slen, " ");
                lval_repr_(s, slen, v->body);
                sappendf(s, slen, ")");
            }
            break;
        case LVAL_FLT:sappendf(s, slen, "%.15g", v->flt);break;
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
void lval_print(lval *v) {
    char *repr = lval_repr(v);
    if (repr) {
        printf("%s", repr);
        free(repr);
    }
}
void lval_println(lval *v) {
    lval_print(v);
    puts("");
}
lenv *lenv_new_no_builtins(void) {
    lenv *e = calloc(1, sizeof(lenv));
    return e;
}
void lenv_add_builtins(lenv *e);
lenv *lenv_new(void) {
    lenv *e = lenv_new_no_builtins();
    lenv_add_builtins(e);
    return e;
}
lenv *lenv_copy(lenv* e) {
    lenv *n = malloc(sizeof(lenv));
    *n=(lenv){.par=e->par,.count=e->count,
        .syms=malloc(sizeof(char*)*e->count),
        .vals=malloc(sizeof(lval*)*e->count)
    };
    for (int i = 0; i < e->count; i++) {
        n->syms[i] = strdup(e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}
void lenv_del(lenv *e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}
lval **lenv_find(lenv *e, lval *k) {
    for (int i = 0; i < e->count; i++) {
        if (!strcmp(e->syms[i], k->sym)) {
            return &e->vals[i];
        }
    }
    return 0;
}
void lenv_print(lenv *e) {
    for (int i = 0; i < e->count; i++) {
        char *repr = lval_repr(e->vals[i]);
        printf("%s=%s\n", e->syms[i], repr);
        free(repr);
    }
}
lval *lenv_get(lenv *e, lval *k) {
    lval **pitem = lenv_find(e, k);
    if (pitem) {
        return lval_copy(*pitem);
    }
    if (e->par) {
        return lenv_get(e->par, k);
    }
    return lval_err(LERR_UNBOUND_SYM, "Unbound Symbol '%s'!", k->sym);
}
void lenv_put(lenv *e, lval *k, lval *v) {
    lval **pitem = lenv_find(e, k);
    if (pitem) {
        lval_del(*pitem);
        *pitem = lval_copy(v);
        return;
    }
    int len = sizeof(lval*) * (e->count + 1);
    e->syms = realloc(e->syms, len);
    e->vals = realloc(e->vals, len);
    e->syms[e->count] = strdup(k->sym);
    e->vals[e->count] = lval_copy(v);
    e->count++;
}
void lenv_def(lenv *e, lval *k, lval *v) {
    while (e->par) { e = e->par; }
    lenv_put(e, k, v);
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
    return errno==0?lval_num(num):lval_errcode(LERR_BAD_NUM);
}
lval *lval_read_flt(mpc_ast_t *a) {
    errno = 0;
    double flt = strtod(a->contents, NULL);
    return errno==0?lval_flt(flt):lval_errcode(LERR_BAD_FLT);
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
    } else if (strstr(a->tag, "qexpr")) {
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
    if (v->count == 0) { // Valgrind doesn't like zero-realloc
        free(v->cell);
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
lval *lval_eval_sexpr(lenv *e, lval *v);
lval *lval_eval(lenv *e, lval *v) {
    if (v->type == LVAL_SYM) {
        lval *x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(e, v);
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
    if (x->type == LVAL_FLT) fail = lval_errcode(LERR_INVALID_OPERAND);
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
            fail = lval_errcode(LERR_DIV_ZERO);
        } else x->flt /= y->flt;
    } else {
        if (y->num == 0) {
            fail = lval_errcode(LERR_DIV_ZERO);
        } else x->num /= y->num;
    }
    return fail;
}
lval *builtin_op(lenv *e, lval *v, char *op) {
    int flt = 0;
    if (v->cell[0]->type == LVAL_FLT) { flt = 1; }
    for (int i = 0; i < v->count; i++) {
        if (flt) { LASSERT_ITYPE(op, v, i, LVAL_FLT); }
        else { LASSERT_ITYPE(op, v, i, LVAL_NUM); }
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
lval *builtin_ord(lenv *e, lval *v, char *op) {
    LASSERT_NUM(op, v, 2);
    LASSERT_ITYPE(op, v, 0, LVAL_NUM);
    LASSERT_ITYPE(op, v, 0, LVAL_NUM);
    int x = v->cell[0]->num, y = v->cell[1]->num, z = 0;
    lval_del(v);
    if (!strcmp(op, ">")) { z = x > y; }
    else if (!strcmp(op, "<=")) { z = x <= y; }
    else if (!strcmp(op, "==")) { z = x == y; }
    else if (!strcmp(op, "!=")) { z = x != y; }
    return lval_num(z);
}
lval *builtin_gt(lenv *e, lval *v) { return builtin_ord(e, v, ">"); }
lval *builtin_le(lenv *e, lval *v) { return builtin_ord(e, v, "<="); }
int lval_eq(lval *x, lval *y) {
    if (x->type != y->type) return 0;
    switch (x->type) {
        case LVAL_NUM:return x->num == y->num;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            if (x->count != y->count) return 0;
            for (int i = 0; i < x->count; i++) {
                if (!lval_eq(x->cell[i], y->cell[i])) return 0;
            }
            return 1;
        default:TODO();break;
    }
    return 0;
}
lval *builtin_cmp(lenv *e, lval *v, char *op) {
    LASSERT_NUM(op, v, 2);
    int eq = lval_eq(v->cell[0], v->cell[1]), ret = 0;
    lval_del(v);
    if (!strcmp(op, "==")) {
        ret = eq;
    } else if (!strcmp(op, "!=")) {
        ret = !eq;
    }
    return lval_num(ret);
}
lval *builtin_eq_(lenv *e, lval *v) {
    return builtin_cmp(e, v, "==");
}
lval *builtin_ne_(lenv *e, lval *v) {
    return builtin_cmp(e, v, "!=");
}
void lenv_add_builtin(lenv *e, char *name, lbuiltin builtin) {
    lval *k = lval_sym(name);
    lval *v = lval_builtin(builtin, name);
    lenv_put(e, k, v);
    lval_del(k); lval_del(v);
}
lval *builtin_add_(lenv *e, lval *v) {
    return builtin_op(e, v, "+");
}
lval *builtin_sub_(lenv *e, lval *v) {
    return builtin_op(e, v, "-");
}
lval *builtin_mul_(lenv *e, lval *v) {
    return builtin_op(e, v, "*");
}
lval *builtin_div_(lenv *e, lval *v) {
    return builtin_op(e, v, "/");
}
lval *builtin_mod_(lenv *e, lval *v) {
    return builtin_op(e, v, "%");
}
lval *builtin_pow_(lenv *e, lval *v) {
    return builtin_op(e, v, "^");
}
lval *builtin_min_(lenv *e, lval *v) {
    return builtin_op(e, v, "min");
}
lval *builtin_max_(lenv *e, lval *v) {
    return builtin_op(e, v, "max");
}
lval *builtin_list(lenv *e, lval *v) {
    v->type = LVAL_QEXPR;
    return v;
}
lval *builtin_eval(lenv *e, lval *v) {
    LASSERT_NUM("eval", v, 1);
    LASSERT_ITYPE("eval", v, 0, LVAL_QEXPR);
    lval *x = lval_take(v, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}
lval *builtin_head(lenv *e, lval *v) {
    LASSERT_NUM("head", v, 1);
    LASSERT_ITYPE("head", v, 0, LVAL_QEXPR);
    LASSERT_IMIN("head", v, 0, 1);
    lval *x = lval_take(v, 0);
    while (x->count > 1) { lval_del(lval_pop(x, 1)); }
    return x;
}
lval *builtin_tail(lenv *e, lval *v) {
    LASSERT_NUM("tail", v, 1);
    LASSERT_ITYPE("tail", v, 0, LVAL_QEXPR);
    LASSERT_IMIN("tail", v, 0, 1);
    lval *x = lval_take(v, 0);
    lval_del(lval_pop(x, 0));
    return x;
}
lval *builtin_join(lenv *e, lval *v) {
    for (int i = 0; i < v->count; i++) { LASSERT_ITYPE("join", v, i, LVAL_QEXPR); }
    lval *x = lval_pop(v, 0);
    while (v->count > 0) { x = lval_join(x, lval_pop(v, 0)); }
    lval_del(v);
    return x;
}
lval *builtin_len(lenv *e, lval *v) {
    LASSERT_NUM("len", v, 1);
    LASSERT_ITYPE("len", v, 0, LVAL_QEXPR);
    lval *x = lval_num(v->cell[0]->count);
    lval_del(v);
    return x;
}
lval *builtin_cons(lenv *e, lval *v) {
    LASSERT_NUM("cons", v, 2);
    LASSERT_ITYPE("cons", v, 0, LVAL_NUM);
    LASSERT_ITYPE("cons", v, 1, LVAL_QEXPR);
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
lval *builtin_init(lenv *e, lval *v) {
    LASSERT_NUM("init", v, 1);
    LASSERT_ITYPE("init", v, 0, LVAL_QEXPR);
    LASSERT_IMIN("init", v, 0, 1);
    lval *x = lval_take(v, 0);
    lval_del(lval_pop(x, x->count - 1));
    return x;
}
lval *builtin_var(lenv *e, lval *v, char *func) {
    LASSERT_MIN(func, v, 2);
    lval *syms = v->cell[0];
    LASSERT_ITYPE(func, v, 0, LVAL_QEXPR);
    LASSERT_IMIN(func, v, 0, 1);
    LASSERT(v, syms->count == (v->count - 1), "Function '%s' size mismatch: Q-expr=%d args=%d!", func, syms->count, v->count - 1);
    for (int i = 0; i < syms->count; i++) {
        LASSERT(v, syms->cell[i]->type == LVAL_SYM, "Function '%s' argument 0 cell %d type must be Symbol! (%s)", func, i, ltype_name(syms->cell[i]->type));
    }
    for (int i = 0; i < syms->count; i++) {
        if (!strcmp(func, "def")) {
            lval **pitem = lenv_find(e, syms->cell[i]);
            if (pitem && (*pitem)->type == LVAL_FUN) {
                lval *err = lval_err_oper("Can't override builtin symbol '%s'!",syms->cell[i]->sym);
                lval_del(v);
                return err;
            }
            lenv_def(e, syms->cell[i], v->cell[i+1]);
        } else if (!strcmp(func, "=")) {
            lenv_put(e, syms->cell[i], v->cell[i+1]);
        }
    }
    lval_del(v);
    return lval_sexpr();
}
lval *builtin_def(lenv *e, lval *v) {
    return builtin_var(e, v, "def");
}
lval *builtin_put(lenv *e, lval *v) {
    return builtin_var(e, v, "=");
}
lval *builtin_env(lenv *e, lval *v) {
    LASSERT_NUM("env", v, 1);
    lenv_print(e);
    lval_del(v);
    return lval_sexpr();
}
int g_exit = 0;
lval *builtin_exit(lenv *e, lval *v) {
    LASSERT_NUM("exit", v, 1);
    g_exit = 1;
    lval_del(v);
    return lval_sexpr();
}
lval *builtin_lambda(lenv *e, lval *v) {
    LASSERT_NUM("\\", v, 2);
    LASSERT_ITYPE("\\", v, 0, LVAL_QEXPR);
    LASSERT_ITYPE("\\", v, 1, LVAL_QEXPR);
    lval *syms = v->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(v, syms->cell[i]->type == LVAL_SYM, "Function '\\' argument 0 cell %d type must be Symbol! (%s)", i, ltype_name(syms->cell[i]->type));
    }
    lval *formals = lval_pop(v, 0);
    lval *body = lval_pop(v, 0);
    lval_del(v);
    return lval_lambda(formals, body);
}
lval *builtin_fun(lenv *e, lval *v) {
    LASSERT_NUM("fun", v, 2);
    LASSERT_ITYPE("fun", v, 0, LVAL_QEXPR);
    LASSERT_ITYPE("fun", v, 1, LVAL_QEXPR);
    LASSERT_IMIN("fun", v, 0, 2);
    lval *syms = v->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(v, syms->cell[i]->type == LVAL_SYM, "Function '_fun' argument 0 cell %d type must be Symbol! (%s)", i, ltype_name(syms->cell[i]->type));
    }
    lval *formals = lval_pop(v, 0);
    lval *fname = lval_pop(formals, 0);
    lval *body = lval_pop(v, 0);
    lval_del(v);
    lval *lamb = lval_lambda(formals, body);
    lenv_put(e, fname, lamb);
    lval_del(lamb);
    lval_del(fname);
    return lval_sexpr();
}
lval *builtin_if(lenv *e, lval *v) {
    LASSERT_NUM("if", v, 3);
    LASSERT_ITYPE("if", v, 0, LVAL_NUM);
    LASSERT_ITYPE("if", v, 1, LVAL_QEXPR);
    LASSERT_ITYPE("if", v, 2, LVAL_QEXPR);
    int cond = v->cell[0]->num;
    lval *x = lval_take(v, cond ? 1 : 2);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}
void lenv_add_builtins(lenv *e) {
    if (e->count > 0) {
        printf("%s: There are already %d items in environment\n", __func__, e->count);
        exit(1);
    }
    lenv_add_builtin(e, "+", builtin_add_);
    lenv_add_builtin(e, "-", builtin_sub_);
    lenv_add_builtin(e, "*", builtin_mul_);
    lenv_add_builtin(e, "/", builtin_div_);
    lenv_add_builtin(e, "%", builtin_mod_);
    lenv_add_builtin(e, "^", builtin_pow_);
    lenv_add_builtin(e, "min", builtin_min_);
    lenv_add_builtin(e, "max", builtin_max_);
    lenv_add_builtin(e, ">", builtin_gt);
    lenv_add_builtin(e, "<=", builtin_le);
    lenv_add_builtin(e, "==", builtin_eq_);
    lenv_add_builtin(e, "!=", builtin_ne_);
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "len", builtin_len);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "init", builtin_init);
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);
    lenv_add_builtin(e, "env", builtin_env);
    lenv_add_builtin(e, "exit", builtin_exit);
    lenv_add_builtin(e, "\\", builtin_lambda);
    lenv_add_builtin(e, "fun", builtin_fun);
    lenv_add_builtin(e, "if", builtin_if);
}
lval *builtin(lenv *e, lval *v, char *func) {
    if (!strcmp("list", func)) { return builtin_list(e, v); }
    else if (!strcmp("eval", func)) { return builtin_eval(e, v); }
    else if (!strcmp("head", func)) { return builtin_head(e, v); }
    else if (!strcmp("tail", func)) { return builtin_tail(e, v); }
    else if (!strcmp("join", func)) { return builtin_join(e, v); }
    else if (!strcmp("len", func)) { return builtin_len(e, v); }
    else if (!strcmp("cons", func)) { return builtin_cons(e, v); }
    else if (!strcmp("init", func)) { return builtin_init(e, v); }
    else if (strstr("+-/*%^ min max", func)) { return builtin_op(e, v, func); }
    lval_del(v);
    return lval_err_oper("Unknown builtin function '%s'!", func);
}
lval *lval_call(lenv *e, lval *f, lval *v) {
    if (f->builtin) { return f->builtin(e, v); }
    int given = v->count;
    int total = f->formals->count;
    while (v->count) {
        if (!f->formals->count) {
            lval_del(v);
            return lval_err_oper("Function passed too many arguments. Got %d, expected %d.", given, total);
        }
        lval *sym = lval_pop(f->formals, 0);
        if (!strcmp(sym->sym, "&")) {
            if (f->formals->count != 1) {
                lval_del(v);
                return lval_err_oper("Function format invalid. Symbol '&' not followed by single symbol.");
            }
            lval *nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, v));
            lval_del(sym); lval_del(nsym);
            break;
        }
        lval *val = lval_pop(v, 0);
        lenv_put(f->env, sym, val);
        lval_del(sym); lval_del(val);
    }
    lval_del(v);
    if (f->formals->count > 0 && !strcmp(f->formals->cell[0]->sym, "&")) {
        if (f->formals->count != 2) {
            return lval_err_oper("Function format invalid. Symbol '&' not followed by single symbol.");
        }
        lval_del(lval_pop(f->formals, 0));
        lval *sym = lval_pop(f->formals, 0);
        lval *val = lval_qexpr();
        lenv_put(f->env, sym, val);
        lval_del(sym); lval_del(val);
    }
    if (!f->formals->count) { /* All formals were bound: evaluate */
        f->env->par = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    } else { /* Return partially evaluated function */
        return lval_copy(f);
    }
}
lval *lval_eval_sexpr(lenv *e, lval *v) {
    if (v->type == LVAL_ERR) return v;
    if (v->count == 0) return v;
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }
    if (v->count == 1) return lval_take(v, 0);
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval *err = lval_err_oper("S-expression does not start with a function! (type=%d)", f->type);
        lval_del(f); lval_del(v);
        return err;
    }
    lval *result = lval_call(e, f, v);
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
        res = lval_errcode(LERR_PARSE_ERROR);
    }
    return res;
}
lval *eval(lenv *e, char *s) {
    lval *res = lread(s);
    if (res->type == LVAL_ERR) return res;
    return lval_eval_sexpr(e, res);
}
char *eval_print(lenv *e, char *s) {
    lval *ret = eval(e, s);
    if (!ret) return 0;
    char *res = lval_repr(ret);
    printf("%s\n", res ? res : "(nil)");
    lval_del(ret);
    return res;
}
int main() {
    char buf[512];
    lenv *e = lenv_new();
    g_exit = 0;
    while (!g_exit) {
        printf("lispy> ");
        if (!fgets(buf, sizeof(buf), stdin)) break;
        char *endl = strchr(buf, '\n');
        if (endl) *endl = 0;
        char *res = eval_print(e, buf);
        if (res) free(res);
    }
    puts("");
    lenv_del(e);
    return 0;
}
