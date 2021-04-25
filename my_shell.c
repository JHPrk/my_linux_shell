#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_LINE 80
#define MAX_PATH 60
#define HISTORY_SIZE 10

int interpret_command(char **args, int *pipe_index, int *is_background, char *readline, int *read_start);
int fork_and_run(char **args, int *pipe_index, int *is_background, char *readline, int *read_start, int args_num);
void run_commands(char* command, char **args);
void print_args(char **args, int args_num);
void history_push_back(char* line);
char* get_history_data(int index);
void print_args(char **args, int args_num);

// history 구조체. 실행한 명령어를 HISTORY_SIZE의 크기만큼 저장하는 circular queue이다.
struct history_queue
{
    int size;
    int data_len;
    int back;
    int front;
    char *queue[MAX_LINE];
} history;
char* cur_line;

int main(void)
{
    char *args[MAX_LINE/2 + 1];
    int should_run = 1;
    char readline[MAX_LINE];
    int pipe_index = -1;
    int is_background = 0;
    int read_start = 0;
    int args_num = 0;
    int result = 0;
    char dir_path[MAX_PATH];
    getcwd(dir_path,MAX_PATH);
    history.size = HISTORY_SIZE+1;
    history.data_len = 0;
    history.back = 0;
    history.front = 0;

    while(should_run)
    {
        printf("%s:osh>", dir_path);
        // get commands by line
        fgets(readline, MAX_LINE, stdin);
        read_start = 0;
        printf("line : %s\n", readline);

        // intrepreting line -> args**형식으로 execvp에 맞는 인수 형태로 바꾸어준다.
        args_num = interpret_command(args, &pipe_index, &is_background, readline, &read_start);
        for(int i = 0; i < args_num; i++)
        {
            printf("args %d : %s\n", i, args[i]);
        }

        // exit, quit 명령어일 경우 종료
        if(strcmp(args[0],"exit") == 0 || strcmp(args[0],"quit") == 0)
            break;
        // 이전 히스토리를 실행하는 명령어는 히스토리에 기록하지 않는다.
        else if(strcmp(args[0],"!") != 0 && strcmp(args[0],"!!") != 0)
        {
            // record history
            cur_line = (char*)malloc(sizeof(char) * MAX_LINE);
            strcpy(cur_line, readline);
            history_push_back(cur_line);
        }
        // cd 명령어, 디렉토리를 바꿔준다.
        if(strcmp(args[0],"cd") == 0)
        {
            if(args[1] == NULL)
            {
                strcpy(dir_path, getenv("HOME"));
                chdir(dir_path);
            }
            else if(args[2] != NULL)
            {
                printf("USAGE: cd [dir]\n");
            }
            else
            {
                if(chdir(args[1]))
                {
                    printf("No directory\n");
                    continue;
                }
                getcwd(dir_path,MAX_PATH);
            }
        }
        else if(!(fork_and_run(args, &pipe_index, &is_background, readline, &read_start, args_num)))
            break;
        fflush(stdout);
    }
    return 0;
}

void history_push_back(char* line)
{
    history.queue[history.back] = line;
    history.back = (history.back+1)%history.size; 
    if(history.back == history.front)
    {
        history.front = (history.front+1)%history.size;
    }
    else
        history.data_len++;
    printf("put history at %d, %s\n", history.back, line);
    
}
char* get_history_data(int index)
{
    if(index > history.data_len || history.data_len == 0) 
        return NULL;
    return history.queue[(history.back+history.size - index)%history.size];
}
void print_history()
{
    for(int i = 0; i < history.data_len; i++)
    {
        printf("%d %s",history.data_len - i, history.queue[(history.front+i)%history.size]);
    }
}
void print_args(char **args, int args_num)
{
    char *p = args[0];
    for(int i = 0; i < 15; i++)
    {
        if(*p != '\0')    
            printf("%c ",*p);
        else
            printf(" - i : %d\n", i);
        *p++;
    }
    for(int i = 0; i < args_num; i++)
    {
        printf("args %d : %s\n", i, args[i]);
    }
}
int fork_and_run(char **args, int *pipe_index, int *is_background, char *readline, int *read_start, int args_num)
{
    int pid = fork();
    if(pid < 0) { // fork error
        perror("fork error\n");
        return -1;
    }
    else if(pid == 0) { // child process
        run_commands(args[0], args);
        exit(0);
    }
    else { // parent process
        // background로 실행하는 경우 명령어의 실행이 끝날때까지 기다리지 않는다.
        if(*is_background == 0)
            wait(NULL);
    }
    return 1;
}

void run_commands(char* command, char **args)
{
    if(strcmp(command,"history") == 0)
    {
        print_history();
    }
    else if(strcmp(command,"!") == 0)
    {
        int index = atoi(args[1]);
        if(!index)
        {
            printf("! 명령어 뒤에는 정수형 index가 와야 합니다!\n");
            return;
        }
        char* re_com = get_history_data(index);
        if(re_com == NULL)
        {
            printf("%d번째 뒤의 명령어가 존재하지 않습니다!\n", index);
            print_history();
            return;
        }
        printf("get history at %s : %s\n", args[1], re_com);
        int pipe_index = -1;
        int is_background = 0;
        int read_start = 0;
        int args_num = 0;
        args_num = interpret_command(args, &pipe_index, &is_background, re_com, &read_start);
        fork_and_run(args, &pipe_index, &is_background, re_com, &read_start, args_num);
    }
    else if(strcmp(command,"!!") == 0)
    {
        char* re_com = get_history_data(1);
        printf("get history at %d : %s\n", 1, re_com);
        int pipe_index = -1;
        int is_background = 0;
        int read_start = 0;
        int args_num = 0;
        args_num = interpret_command(args, &pipe_index, &is_background, re_com, &read_start);
        fork_and_run(args, &pipe_index, &is_background, re_com, &read_start, args_num);
    }
    else // default
    {
        if(execvp(args[0],args) == -1) {
            perror("명령어 실행 오류!");
        }
    }

}
int interpret_command(char **args, int *pipe_index, int *is_background, char *readline, int *read_start)
{
    char ch;
    int args_cnt = 0;
    int commands_index = 0;
    char *command;
    int str_read = 0;
    while((ch = readline[(*read_start)++]) != '\0')
    {
        if(isspace(ch))
        {
            if(str_read != 0)
            {
                command[str_read++] = '\0';
                str_read = 0;
            }
        }
        else switch (ch)
        {
        case '|':
            *pipe_index = *read_start;
            if(str_read != 0)
            {
                command[str_read++] = '\0';
                args[args_cnt] = NULL;
            }
            return args_cnt+1;
        case '&':
            *is_background = 1;
            break;
        default:
            if(str_read == 0)
            {
                command = (char*)malloc(sizeof(char) * (MAX_LINE/2 + 1));
                args[args_cnt++] = command;
            }
            command[str_read++] = ch;
            break;
        }
    }
    if(str_read != 0)
        command[str_read++] = '\0';
    args[args_cnt] = NULL;
    return args_cnt+1;
}