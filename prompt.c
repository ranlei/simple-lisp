#include<stdio.h>
static char input[2048];

int main(int argc,char** argv){
    puts("simple-lisp Version 0.1");
    puts("Press ctrl-c to exit\n");
    while(1){
        fputs("simple-lisp>",stdout);
        fgets(input,2048,stdin);
        printf("No,you are a %S",input);
    
    }
    return 0;
}
