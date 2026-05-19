#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include<signal.h>

void trim_newline(char* str){
    int len = strlen(str);
    if(len > 0 && str[len - 1] == '\n') str[len-1] = '\0';
}

void cmd_start_monitor(){
    pid_t hub_mon = fork();

    if(hub_mon < 0) return;

    if(hub_mon == 0){
        int pipefd[2];
        if(pipe(pipefd) == -1) exit(1);

        pid_t monitor_pid = fork();
        if(monitor_pid == 0){
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execl("./monitor_reports", "monitor_reports", NULL); 
            exit(1);
        }else{
            close(pipefd[1]);
            char buffer[256];
            int n;

            while((n = read(pipefd[0],buffer,sizeof(buffer)-1))> 0){ 
                buffer[n] = '\0';
                if(strncmp(buffer,"ERR:", 4) == 0){
                    printf("\n[HUB_MON] Error : %s", buffer + 4);
                }else if(strncmp(buffer,"START:",6) == 0){
                    printf("\n[HUB_MON] Info : %s", buffer +6);
                }else if(strncmp(buffer,"ALERT:",6) == 0){
                    printf("\n[HUB_MON] 🚨 Alert : %s", buffer+ 6);
                }else if(strncmp(buffer,"EXIT:",5) == 0){
                    printf("\n[HUB_MON] Exit : %s", buffer + 5);
                }else{
                    printf("\n[HUB_MON] : %s ", buffer);
                }

                printf("city_hub> ");
                fflush(stdout);
            }
            close(pipefd[0]);
            wait(NULL); 
            exit(0);
        }
    }else{
        printf("Monitor started on background. (Process HUB PID %d.)\n", hub_mon);
    }
}

void cmd_calculate_scores(char* input){
    char* token = strtok(input, " ");
    token = strtok(NULL, " ");

    char* districts[20];
    int count = 0;
    while(token != NULL && count < 20){
        districts[count++] = token;
        token = strtok(NULL, " ");
    }

    if(count == 0){
        printf("Error : specify at least one district.\n");
        return;
    }

    int pipes[20][2];
    pid_t pids[20];

    printf("COMBINED REPORT : \n");

    for(int i = 0; i < count; i++){
        if(pipe(pipes[i]) == -1){
            continue;
        }

        pids[i] = fork();

        if(pids[i] == 0){
            close(pipes[i][0]); 
            dup2(pipes[i][1], STDOUT_FILENO);
            close(pipes[i][1]);

            execl("./scorer", "scorer", districts[i], NULL);
            perror("Eroare exec scorer");
            exit(1);
        }else{
            close(pipes[i][1]);
        }
    }

    for(int i = 0; i < count; i++){
        char buffer[256];
        int bytes_read;
        while((bytes_read = read(pipes[i][0],buffer,sizeof(buffer) -1)) > 0){
            buffer[bytes_read] = '\0';
            printf("%s",buffer);
        }
        close(pipes[i][0]);
        waitpid(pids[i],NULL,0);
    }
    printf("=================\n\n");
}

int main(){
    char input[256];
    char input_copy[256];

    printf("Welcome to City Hub : \n");

    while(1){
        printf("city_hub> ");
        if(fgets(input,sizeof(input), stdin) == NULL) break;
        trim_newline(input);

        if(strlen(input) == 0){
            continue; 
        }
        strcpy(input_copy, input);

        if(strcmp(input,"exit") == 0){
            printf("Closing City Hub.. \n");
            int pid_fd = open(".monitor_pid", O_RDONLY);
            if(pid_fd >=0) {
                char pid_buf[32];
                if(read(pid_fd, pid_buf, 31) > 0){
                    kill(atoi(pid_buf), SIGINT);
                }
                close(pid_fd);
            }
            break;
        }else if(strcmp(input, "start_monitor") == 0){
            cmd_start_monitor();
        }else if(strncmp(input, "calculate_scores", 16) == 0){
            cmd_calculate_scores(input_copy); 
        }else{
            printf("Unknown command. \n"); 
        }
    }
    return 0;
}