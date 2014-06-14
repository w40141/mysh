#include <stdio.h>      /* perror, feof, ferror, fputs */
#include <stdlib.h>     /* exit */
#include <sys/types.h>  /* fork, wait, opne */
#include <sys/wait.h>   /* wait */
#include <unistd.h>     /* execvp, fork, close */
#include <string.h>     /* strcmp */
#include <fcntl.h>      /* open */
#include <sys/stat.h>   /* open */
#include <ctype.h>      /* isspace */

enum {
    MAXARGV=100,MAXLINE=4096,
};

 /* ctrl-Cを消す */
void handler(int sig){
         signal(SIGINT,handler);
        fprintf(stderr,"\nmyprompt> ");
}

/* ベクタを作る */
int strtovec(char *s, char **v, int max){
    int i=0, skip;

    if(max<1 || v==0) skip=1;
    else skip=0;

    for(;;){
        if(!skip && i>=max-1){
            v[i]=0;
            skip=1;
        }
        while(*s!='\0' && isspace(*s)) s++;
        if(*s=='\0') break;
        if(!skip) v[i]=s;
        i++;
        while(*s!='\0' && !isspace(*s)) s++;
        if(*s=='\0') break;
        *s='\0';
        s++;
    }
    if(!skip) v[i]=0;
    i++;

    return i;
}

/* 文字を読み捨てる */
static void discardline(FILE *fp){
    int c;

    while((c==getc(fp))!=EOF) if(c=='\n') break;
}

/* 文字列を標準入力から得る */
void getstr(char *prompt, char *s, int slen){
    char *p, buf[MAXLINE];

    if(slen>MAXLINE)
        fprintf(stderr,
                "getstr: buffer lenght must be <= %d (%d specified);"
                "proceeding anyway\n", MAXLINE, slen);
    fputs(prompt, stderr);
    fgets(buf, MAXLINE, stdin);
    if((p=strchr(buf, '\n'))==NULL) discardline(stdin);
    else *p='\0';

    if(strlen(buf)<slen){
        strcpy(s,buf);
    }else{
        strncpy(s, buf, slen-1);
        s[slen-1]='\n';
    }
}

int main(){
    char cmd[1024];
    char *av[MAXARGV], *userinput[MAXARGV], *infile, *outfile;
    int ac, status, i, nwords;
    pid_t cpid;

    /* ctrl-Cを消す */
    signal(SIGINT,handler);

    for(;;){
        /* 入力待ち */
        getstr("myprompt> ", cmd, sizeof(cmd));
        if(feof(stdin)){
            exit(0);
        }else if(ferror(stdin)){
            perror("getstr");
            exit(1);
        }
        if((nwords=strtovec(cmd, userinput, MAXARGV))>MAXARGV){
            fputs("too many arguments\n", stderr);
            continue;
        }
        nwords--;
        /* 入力なし */
        if(nwords==0) continue;
        /* exitで抜ける */
        if(strcmp(userinput[0],"exit")==0) break;

        ac=0;
        infile=outfile=NULL;
        for(i=0;i<nwords;i++){
            /* 標準出力の切替 */
            if(strcmp(userinput[i],">")==0){
                outfile=userinput[i+1];
                i++;
            /* 標準入力の切替 */
            }else if(strcmp(userinput[i],"<")==0){
                infile=userinput[i+1];
                i++;
            }else{
            /* コマンド文字の取得 */
                av[ac++]=userinput[i];
            }
        }
        av[ac]=NULL;

        if((cpid=fork())==-1){
            perror("fork");
            continue;
        }else if(cpid==0){
        /* 子プロセス */
            if(infile!=NULL){
                close(0);
                if(open(infile,O_RDONLY)==-1){
                    perror(infile);
                    exit(1);
                }
            }
            if(outfile!=NULL){
                close(1);
                if(open(outfile,O_WRONLY| O_CREAT| O_TRUNC, 0666)==-1){
                    perror(outfile);
                    exit(1);
                }
            }
            execvp(av[0], av);
            perror(cmd);
            exit(1);
        }
        if(wait(&status)==(pid_t)-1){
        /* 親プロセス */
            perror("wait");
            exit(1);
        }
    }
    exit(0);
}
