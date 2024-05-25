#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void exe_sub(char *);
void exe_pipe(parsed_input *);
void exe_para(parsed_input *);
void exe_seq(parsed_input *);
void exe_pipeline(pipeline);
int para_pipeline(pipeline);

void handler(int sig){
    return;
}

void exe_sub(char *subshell){
    pid_t pid;
    int status;
    parsed_input *inp = malloc(sizeof(parsed_input));
    parse_line(subshell, inp);
    pid = fork();
    if(pid == 0){
        if(inp->num_inputs > 1){
            switch(inp->separator){
                case SEPARATOR_PIPE:
                    exe_pipe(inp);
                    break;
                case SEPARATOR_SEQ:
                    exe_seq(inp);
                    break;
                case SEPARATOR_PARA:{
                    int n = 0;
                    int curr = 0;
                    for(int i=0 ; i<inp->num_inputs ; ++i){
                        if(inp->inputs[i].type == INPUT_TYPE_PIPELINE){
                            n += inp->inputs[i].data.pline.num_commands;
                        }else{
                            ++n;
                        }
                    }
                    int **fds = malloc(sizeof(int*) * n);
                    for(int i=0 ; i<n ; ++i){
                        fds[i] = malloc(sizeof(int) * 2);
                        pipe(fds[i]);
                    }
                    for(int i=0 ; i<inp->num_inputs ; ++i){
                        if(inp->inputs[i].type == INPUT_TYPE_PIPELINE){
                            for(int j=0 ; j<inp->inputs[j].data.pline.num_commands ; ++j){
                                pid = fork();
                                if(pid == 0){
                                    dup2(fds[curr][0], 0);
                                    if(j != inp->inputs[i].data.pline.num_commands-1){
                                        dup2(fds[curr+1][1], 1);
                                    }
                                    for(int k=0 ; k<n ; ++k){
                                        close(fds[k][0]);
                                        close(fds[k][1]);
                                    }
                                    char first_arg[INPUT_BUFFER_SIZE] = "/bin/";
                                    char **args = inp->inputs[i].data.pline.commands[j].args;
                                    strcat(first_arg, args[0]);
                                    execv(first_arg, args);
                                }
                                ++curr;
                            }
                        }else{
                            pid = fork();
                            if(pid == 0){
                                dup2(fds[curr][0], 0);
                                for(int j=0 ; j<n ; ++j){
                                    close(fds[j][0]);
                                    close(fds[j][1]);
                                }
                                char first_arg[INPUT_BUFFER_SIZE] = "/bin/";
                                char **args = inp->inputs[i].data.cmd.args;
                                strcat(first_arg, args[0]);
                                execvp(first_arg, args);
                            }
                            ++curr;
                        }
                    }
                    pid = fork();
                    if(pid == 0){
                        curr = 0;
                        for(int i=0 ; i<inp->num_inputs ; ++i){
                            dup2(fds[curr][1], 1);
                            if(inp->inputs[i].type == INPUT_TYPE_PIPELINE){
                                curr += inp->inputs[i].data.pline.num_commands;
                            }else{
                                ++curr;
                            }
                        }
                        char ch;
                        int all_closed = 1;
                        while(1){
                            if(read(STDIN_FILENO, &ch, 1) > 0){
                                curr = 0;
                                all_closed = 1;
                                for(int i=0 ; i<inp->num_inputs ; ++i){
                                    if(write(fds[curr][1], &ch, 1) > 0){
                                        all_closed = 0;
                                    }
                                    if(inp->inputs[i].type == INPUT_TYPE_PIPELINE){
                                        curr += inp->inputs[i].data.pline.num_commands;
                                    }else{
                                        ++curr;
                                    }
                                }
                                if(all_closed){
                                    for(int j=0 ; j<n ; ++j){
                                        close(fds[j][0]);
                                        close(fds[j][1]);
                                    }
                                    exit(1);
                                }
                            }else{
                                for(int j=0 ; j<n ; ++j){
                                    close(fds[j][0]);
                                    close(fds[j][1]);
                                }
                                exit(1);
                            }
                        }
                    }
                    for(int j=0 ; j<n ; ++j){
                        close(fds[j][0]);
                        close(fds[j][1]);
                    }
                    while(n+1 > 0){
                        wait(&status);
                        --n;
                    }
                    for(int i=0 ; i<n ; ++i){
                        free(fds[i]);
                    }
                    free(fds);
                }
            }
        }else{
            switch(inp->inputs[0].type){
                case INPUT_TYPE_COMMAND:
                    exe_pipe(inp);
                    break;
                case INPUT_TYPE_SUBSHELL:
                    exe_sub(inp->inputs[0].data.subshell);
            }
        }
        exit(1);
    }else{
        wait(&status);
    }
    free_parsed_input(inp);
    return;
}

void exe_pipe(parsed_input *inp){
    pid_t pid;
    int status;
    if(inp->num_inputs == 1){
        pid = fork();
        if(pid == 0){
            char first_arg[INPUT_BUFFER_SIZE] = "/bin/";
            char **args = inp->inputs[0].data.cmd.args;
            strcat(first_arg, args[0]);
            execvp(first_arg, args);
        }
        wait(&status);
        return;
    }
    int **fds = malloc(sizeof(int*) * (inp->num_inputs-1));
    for(int i=0 ; i<inp->num_inputs-1 ; ++i){
        fds[i] = malloc(sizeof(int) * 2);
        pipe(fds[i]);
    }
    for(int i=0 ; i<inp->num_inputs ; ++i){
        pid = fork();
        if(pid == 0){
            if(i != 0){
                dup2(fds[i-1][0], 0);
            }
            if(i != inp->num_inputs-1){
                dup2(fds[i][1], 1);
            }
            for(int j=0 ; j<inp->num_inputs-1 ; ++j){
                close(fds[j][0]);
                close(fds[j][1]);
            }
            if(inp->inputs[i].type == INPUT_TYPE_COMMAND){
                char first_arg[INPUT_BUFFER_SIZE] = "/bin/";
                char **args = inp->inputs[i].data.cmd.args;
                strcat(first_arg, args[0]);
                execv(first_arg, args);
            }else{
                exe_sub(inp->inputs[i].data.subshell);
                exit(1);
            }
        }
    }
    for(int j=0 ; j<inp->num_inputs-1 ; ++j){
        close(fds[j][0]);
        close(fds[j][1]);
    }
    int n = inp->num_inputs;
    while(n > 0){
        wait(&status);
        --n;
    }
    for(int i=0 ; i<inp->num_inputs-1 ; ++i){
        free(fds[i]);
    }
    free(fds);
    return;
}

void exe_pipeline(pipeline pline){
    pid_t pid;
    int status;
    int **fds = malloc(sizeof(int*) * (pline.num_commands-1));
    for(int i=0 ; i<pline.num_commands-1 ; ++i){
        fds[i] = malloc(sizeof(int) * 2);
        pipe(fds[i]);
    }
    for(int i=0 ; i<pline.num_commands ; ++i){
        pid = fork();
        if(pid == 0){
            if(i != 0){
                dup2(fds[i-1][0], 0);
            }
            if(i != pline.num_commands-1){
                dup2(fds[i][1], 1);
            }
            for(int j=0 ; j<pline.num_commands-1 ; ++j){
                close(fds[j][0]);
                close(fds[j][1]);
            }
            char first_arg[INPUT_BUFFER_SIZE] = "/bin/";
            char **args = pline.commands[i].args;
            strcat(first_arg, args[0]);
            execv(first_arg, args);
        }
    }
    for(int j=0 ; j<pline.num_commands-1 ; ++j){
        close(fds[j][0]);
        close(fds[j][1]);
    }
    int n = pline.num_commands;
    while(n > 0){
        wait(&status);
        --n;
    }
    for(int i=0 ; i<pline.num_commands-1 ; ++i){
        free(fds[i]);
    }
    free(fds);
    return;
}

int para_pipeline(pipeline pline){
    int n = 0;
    pid_t pid;
    int **fds = malloc(sizeof(int*) * (pline.num_commands-1));
    for(int i=0 ; i<pline.num_commands-1 ; ++i){
        fds[i] = malloc(sizeof(int) * 2);
        pipe(fds[i]);
    }
    n = pline.num_commands;
    for(int i=0 ; i<pline.num_commands ; ++i){
        pid = fork();
        if(pid == 0){
            if(i != 0){
                dup2(fds[i-1][0], 0);
            }
            if(i != pline.num_commands-1){
                dup2(fds[i][1], 1);
            }
            for(int j=0 ; j<pline.num_commands-1 ; ++j){
                close(fds[j][0]);
                close(fds[j][1]);
            }
            char first_arg[INPUT_BUFFER_SIZE] = "/bin/";
            char **args = pline.commands[i].args;
            strcat(first_arg, args[0]);
            execv(first_arg, args);
        }
    }
    for(int j=0 ; j<pline.num_commands-1 ; ++j){
        close(fds[j][0]);
        close(fds[j][1]);
    }
    for(int i=0 ; i<pline.num_commands-1 ; ++i){
        free(fds[i]);
    }
    free(fds);
    return n;
}

void exe_para(parsed_input *inp){
    pid_t pid;
    int status;
    int n = 0;
    for(int i=0 ; i<inp->num_inputs ; ++i){
        if(inp->inputs[i].type == INPUT_TYPE_PIPELINE){
            n += para_pipeline(inp->inputs[i].data.pline);
        }else{
            ++n;
            pid = fork();
            if(pid == 0){
                switch(inp->inputs[i].type){
                    case INPUT_TYPE_COMMAND:{
                        char first_arg[INPUT_BUFFER_SIZE] = "/bin/";
                        char **args = inp->inputs[i].data.cmd.args;
                        strcat(first_arg, args[0]);
                        execvp(first_arg, args);
                    }
                    default:{
                        exe_sub(inp->inputs[i].data.subshell);
                    }
                }
            }
        }
    }
    while(n > 0){
        wait(&status);
        --n;
    }
    return;
}

void exe_seq(parsed_input *inp){
    pid_t pid;
    int status;
    for(int i=0 ; i<inp->num_inputs ; ++i){
        if(inp->inputs[i].type == INPUT_TYPE_PIPELINE){
            exe_pipeline(inp->inputs[i].data.pline);
        }else{
            pid = fork();
            if(pid == 0){
                switch(inp->inputs[i].type){
                    case INPUT_TYPE_COMMAND:{
                        char first_arg[INPUT_BUFFER_SIZE] = "/bin/";
                        char **args = inp->inputs[i].data.cmd.args;
                        strcat(first_arg, args[0]);
                        execvp(first_arg, args);
                        break;
                    }
                    default:{
                        exe_sub(inp->inputs[i].data.subshell);
                    }
                }
                exit(1);
            }else{
                wait(&status);
            }
        }
    }
    return;
}

int main(){
    parsed_input *inp = malloc(sizeof(parsed_input));
    char line[INPUT_BUFFER_SIZE];
    signal(SIGPIPE, handler);
    while(1){
        printf("/> ");
        fgets(line, sizeof(line), stdin);
        if(strcmp(line, "quit\n") == 0){
            break;
        }
        parse_line(line, inp);
        if(inp->num_inputs > 1){
            switch(inp->separator){
                case SEPARATOR_PIPE:
                    exe_pipe(inp);
                    break;
                case SEPARATOR_PARA:
                    exe_para(inp);
                    break;
                case SEPARATOR_SEQ:
                    exe_seq(inp);
            }
        }else{
            switch(inp->inputs[0].type){
                case INPUT_TYPE_COMMAND:
                    exe_pipe(inp);
                    break;
                case INPUT_TYPE_SUBSHELL:
                    exe_sub(inp->inputs[0].data.subshell);
            }
        }
        free_parsed_input(inp);
    }
    return 0;
}