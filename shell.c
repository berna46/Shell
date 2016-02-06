#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#define MAXARGS 100
#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct command {
    char *cmd;              // string apenas com o comando
    int argc;               // número de argumentos
    char *argv[MAXARGS+1];  // vector de argumentos do comando
    struct command *next;   // apontador para o próximo comando
} COMMAND;

// variáveis globais
char* inputfile = NULL;     // nome de ficheiro (em caso de redireccionamento da entrada padrão)
char* outputfile = NULL;    // nome de ficheiro (em caso de redireccionamento da saída padrão)
int background_exec = 0;    // indicação de execução concorrente com a mini-shell (0/1)

// declaração de funções
COMMAND* parse(char* linha);
int print_parse(COMMAND* commlist);
void execute_commands(int number, COMMAND* commlist); //berna51
void free_commlist(COMMAND* commlist);

// include do código do parser da linha de comandos
#include "parser.c"

int main(int argc, char* argv[]) {
  char *linha;
  COMMAND *com;
  int nComm=0;

  while (1) {
    if ((linha = readline("mini_shell$ ")) == NULL)
      exit(0);
    if (strlen(linha) != 0) {
      add_history(linha);
      com = parse(linha);
      if (com) {
        nComm=print_parse(com);
        execute_commands(nComm, com);
        free_commlist(com);
      }
    }
    free(linha);
  }
}


int print_parse(COMMAND* commlist) {
  int n, i;

  printf("---------------------------------------------------------\n");
  printf("BG: %d IN: %s OUT: %s\n", background_exec, inputfile, outputfile);
  n = 1;
  while (commlist != NULL) {
    printf("#%d: cmd '%s' argc '%d' argv[] '", n, commlist->cmd, commlist->argc);
    i = 0;
    while (commlist->argv[i] != NULL) {
      printf("%s,", commlist->argv[i]);
      i++;
    }
    printf("%s'\n", commlist->argv[i]);
    commlist = commlist->next;
    n++;
  }
  printf("---------------------------------------------------------\n");
  return n-1;
}

void free_commlist(COMMAND *commlist){
    COMMAND *tmp;
    int background_exec=0;

    while(commlist!=NULL){
      tmp=commlist;
      commlist=commlist->next;
      free(tmp);
    }
}

void execute_commands(int number, COMMAND *commlist) {
  int i, j, status;
  pid_t pid;
  int fd[number-1][2];
    COMMAND *cursor;
    cursor=commlist;
    for(i=0; i<number-1; i++){
      if(pipe(fd[i])==-1){
	perror("ERROR: pipe!");
	exit(1);
      }
    }
    i=0;
    while(cursor!=NULL){
      pid=fork();
      if(pid==-1){
	perror("ERROR; fork!");
	exit(1);
      }
      else if(pid==0){
	if(inputfile!=NULL && i==0){
	  int inputCh = open(inputfile, O_RDONLY);
	  if(inputCh<0){
	    perror("ERROR: couldn't open file!");
	    exit(1);
	  }
	  dup2(inputCh, PIPE_READ);
	}
	if(outputfile!=NULL && i==number-1){
	  int outputCh = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	  if(outputCh<0){
	    perror("ERRO: couldn't open file!");
	    exit(1);
	  }
	  dup2(outputCh, PIPE_WRITE);
	  close(outputCh);
	if(i>0) dup2(fd[i-1][0], 0);
	if(i<number-1) dup2(fd[i][1], 1);
	for(j=0; j<number-1; j++){
	  close(fd[j][0]);
	  close(fd[j][1]);
	}
	execvp(cursor->cmd, cursor->argv);
      }
      cursor = cursor -> next;
      i++;
    }
    for(j=0; j<number-1; j++){
      close(fd[j][PIPE_READ]);
      close(fd[j][PIPE_WRITE]);
    }
        if(background_exec==0){
	  waitpid(pid, &status, WUNTRACED | WCONTINUED);
	}
}
