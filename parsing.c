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
enum{LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

typedef struct {
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

/*destructor of lval type.*/

void lval_del(lval* v){
    switch(v->type){
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err);break;
        case LVAL_SYM: free(v->sym);break;
        case LVAL_SEXPR: 
            for(int i = 0;i<v->count;i++)
                lval_del(v->cell[i]);
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
    if(strcmp(t->tag,'>') == 0) {x = lval_sexpr();}
    if(strstr(t->tag,"sexpr")) {x = lval_sexpr();}

    for (int i = 0;i < t->children_num;i++){
        if(strcmp(t->children[i]->contents, "(") == 0){continue;}
        if(strcmp(t->children[i]->contents, ")") == 0){continue;}
        if(strcmp(t->children[i]->contents, "}") == 0){continue;}
        if(strcmp(t->children[i]->contents, "{") == 0){continue;}
        if(strcmp(t->children[i]->tag, "regex") == 0){continue;}
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;

}


void lval_print(lval v){
    switch(v.type){
        case LVAL_NUM:
            printf("%li\n",v.num);break;
        case LVAL_ERR:
            if(v.err == LERR_DIV_ZERO){printf("Error:Division By Zero!!");}
            if(v.err == LERR_BAD_OP){printf("Error:Operator is not exist!!");}
            if(v.err == LERR_BAD_NUM){printf("Error:Invaild number!!");}
        break;
    }
}

void lval_println(lval v){
    lval_print(v);
    putchar('\n');
}



lval eval_op(lval x,char* op,lval y){
    if(x.type == LVAL_ERR){
        return x;
    }

    if(y.type == LVAL_ERR){
        return y;
    }
    


    if (strcmp(op,"+") == 0){ return lval_num(x.num+y.num); }
    if (strcmp(op,"-") == 0){ return lval_num(x.num-y.num); }
    if (strcmp(op,"*") == 0){ return lval_num(x.num*y.num); }
    if (strcmp(op,"/") == 0){ 
        return y.num == 0 ?lval_err(LERR_DIV_ZERO):lval_num(x.num/y.num);
    }
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t){
    if(strstr(t->tag,"number")){
        
        errno = 0;
        long x = strtol(t->contents,NULL,10);    
        return errno != ERANGE ? lval_num(x):lval_err(LERR_BAD_NUM);
    }

    char* op = t->children[1]->contents;
    lval x = eval(t->children[2]);

    int i = 3;
    while(strstr(t->children[i]->tag,"expr")){
        x = eval_op(x,op,eval(t->children[i]));
        i++;
    }
    
    return x;
}

int main(int argc,char** argv){
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Simplelisp = mpc_new("simplelisp");
    mpca_lang(MPCA_LANG_DEFAULT,
            "\
            number: /-?[0-9]+/ ;\
            symbol: '+'|'-'|'*'|'/';\
            Sexpr: '(' <expr>* ')' ;\
            expr: <number> | <symbol> | <sexpr> ;\
            simplelisp: /^/ <expr>* /$/ ;\
            ",
            Number, Symbol, Sexpr, Expr, Simplelisp);

    
    
    puts("simple-lisp Version 0.1");
    puts("Press ctrl-c to exit\n");
    while(1){
        char* input = readline("simple-lisp>");
        add_history(input);
        
        mpc_result_t r;
        if(mpc_parse("<stdin>",input,Simplelisp,&r)){
            lval result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);
            
            /* test struct mpc_ast_t 
            mpc_ast_t* a = r.output;
            printf("Tags:%s\n",a->tag);
            printf("contents:%s\n",a->contents);
            printf("child number:%d\n",a->children_num);

            mpc_ast_t* c0 = a->children[0];
            printf("tags:%s\n",c0->tag);
            printf("contents:%s\n",c0->contents);
            */


        } else {
        
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        
        }

        free(input);
    
    }
    mpc_cleanup(5,Number, Symbol, Sexpr, Expr, Simplelisp);
    return 0;
}
