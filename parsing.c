#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"
#ifdef _WIN32
static char buffer[2048];
char* readline(char* prompt){
    fputs(prompt,stdout);
    fgets(buffer,2048,stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy,buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char* unused){}

#else

#include <editline/readline.h>
#include <editline/history.h>

#endif
enum{LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };
enum{LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

typedef struct lval{
    int type;
    long num;
    char* err;
    char* sym;

    int count;/*count Pointer to list of "lval*"*/
    struct lval** cell;
} lval;


/*constructor of number*/
lval* lval_num(long x){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/*constructor of err*/
lval* lval_err(char* m){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m)+1);
    strcpy(v->err,m);
    return v;
}

/*constructor of symbol*/
lval* lval_sym(char* s){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s)+1);
    strcpy(v->sym,s);
    return v;
}

/*constructor of S-expression*/
lval* lval_sexpr(void){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;


}

/*constructor of Q-expression*/
lval* lval_qexpr(void){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}



/*destructor of lval type.*/

void lval_del(lval* v){
    switch(v->type){
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err);break;
        case LVAL_SYM: free(v->sym);break;
        case LVAL_QEXPR:
        case LVAL_SEXPR: 
            for(int i = 0; i < v->count; i++){
                lval_del(v->cell[i]);
            }
            free(v->cell);
        break;
    }

    free(v);
}

/*add S-expression*/

lval* lval_add(lval* v, lval* x){
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_read_num(mpc_ast_t* t){
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");    
}

lval* lval_read(mpc_ast_t* t){
    if(strstr(t->tag,"number")){return lval_read_num(t);}
    if(strstr(t->tag,"symbol")){return lval_sym(t->contents);}
    
    lval* x = NULL;
    if(strcmp(t->tag,">") == 0) {x = lval_sexpr();}
    if(strstr(t->tag,"sexpr")) {x = lval_sexpr();}
    if(strstr(t->tag,"qexpr")) {x = lval_qexpr();}
    for (int i = 0;i < t->children_num;i++){
        if(strcmp(t->children[i]->contents, "(") == 0){continue;}
        if(strcmp(t->children[i]->contents, ")") == 0){continue;}
        if(strcmp(t->children[i]->contents, "}") == 0){continue;}
        if(strcmp(t->children[i]->contents, "{") == 0){continue;}
        if(strcmp(t->children[i]->contents, "[") == 0){continue;}
        if(strcmp(t->children[i]->contents, "]") == 0){continue;}
        if(strcmp(t->children[i]->tag, "regex") == 0){continue;}
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;

}

void lval_print(lval* v);

void lval_expr_print(lval* v,char open,char close){
    putchar(open);
    for(int i=0;i< v->count;i++){
        lval_print(v->cell[i]);
        if(i != (v->count-1)){
            putchar(' ');
        }
    }
    putchar(close);

}

void lval_print(lval* v){
    switch(v->type){
        case LVAL_NUM:
            printf("%li\n",v->num);break;
        case LVAL_ERR:
            printf("Error:%s",v->err);break;
        case LVAL_SYM:
            printf("%s",v->sym);break;
        case LVAL_SEXPR:
            lval_expr_print(v,'(',')');break; 
        case LVAL_QEXPR:
            lval_expr_print(v,'[',']');break;
    }
}

void lval_println(lval* v){
    lval_print(v);
    putchar('\n');
}


lval* lval_eval(lval* v);
lval* lval_take(lval* v,int i);
lval* lval_pop(lval* v,int i);
lval* builtin_op(lval* a,char* op);



lval* lval_eval_sexpr(lval* v) {

  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  /* Error Checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  /* Empty Expression */
  if (v->count == 0) { return v; }

  /* Single Expression */
  if (v->count == 1) { return lval_take(v, 0); }

  /* Ensure First Element is Symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }

  /* Call builtin with operator */
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

lval* lval_eval(lval* v) {
  /* Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  /* All other lval types remain the same */
  return v;
}

lval* lval_pop(lval* v, int i) {
  /* Find the item at "i" */
  lval* x = v->cell[i];

  /* Shift the memory following the item at "i" over the top of it */
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}
lval* builtin_op(lval* a, char* op) {
  
  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }
  
  /* Pop the first element */
  lval* x = lval_pop(a, 0);

  /* If no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) { x->num = -x->num; }

  /* While there are still elements remaining */
  while (a->count > 0) {

    /* Pop the next element */
    lval* y = lval_pop(a, 0);

    /* Perform operation */
    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division By Zero!"); break;
      }
      x->num /= y->num;
    }

    /* Delete element now finished with */
    lval_del(y);
  }

  /* Delete input expression and return result */
  lval_del(a);
  return x;
}

int main(int argc,char** argv){
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Simplelisp = mpc_new("simplelisp");
    mpca_lang(MPCA_LANG_DEFAULT,
            "\
            number: /-?[0-9]+/ ;\
            symbol: '+'|'-'|'*'|'/';\
            sexpr: '(' <expr>* ')' ;\
            qexpr: '{' <expr>* '}' ;\
            expr: <number> | <symbol> | <sexpr> ;\
            simplelisp: /^/ <expr>* /$/ ;\
            ",
            Number, Symbol, Sexpr, Qexpr, Expr, Simplelisp);

    
    
    puts("simple-lisp Version 0.1");
    puts("Press ctrl-c to exit\n");
    while(1){
        char* input = readline("simple-lisp>");
        add_history(input);
        
        mpc_result_t r;
        if(mpc_parse("<stdin>",input,Simplelisp,&r)){
            lval* result = lval_eval(lval_read(r.output));
            lval_println(result);
            lval_del(result);   
            /* test struct mpc_ast_t 
            mpc_ast_t* a = r.output;
            printf("Tags:%s\n",a->tag);
            printf("contents:%s\n",a->contents);
            printf("child number:%d\n",a->children_num);

            mpc_ast_t* c0 = a->children[0];
            printf("tags:%s\n",c0->tag);
            printf("contents:%s\n",c0->contents);
            */

            mpc_ast_delete(r.output);
        } else {
        
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        
        }

        free(input);
    
    }
    mpc_cleanup(5,Number, Symbol, Sexpr, Qexpr, Expr, Simplelisp);
    return 0;
}
