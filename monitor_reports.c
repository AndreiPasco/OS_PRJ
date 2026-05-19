#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig){
    char msg[] = "EXIT:Monitor received SIGINT. Shutting down.\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    keep_running = 0;
}

void handle_sigusr1(int sig){
    char msg[] = "ALERT:New report has been added.\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main(){
    int fd_check = open(".monitor_pid", O_RDONLY);
    if (fd_check >= 0) {
        char pid_buf[32];
        memset(pid_buf, 0, sizeof(pid_buf));
        int bytes_read = read(fd_check, pid_buf, sizeof(pid_buf) - 1);
        close(fd_check);
        
        if (bytes_read > 0) {
            pid_t existing_pid = atoi(pid_buf);
            if (existing_pid > 0 && kill(existing_pid, 0) == 0) {

                printf("ERR:Un monitor ruleaza deja cu PID %d\n", existing_pid);
                fflush(stdout); 
                return 1; 
            }
        }
    }

    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if(fd < 0){
        perror("Error creating .monitor_pid");
        return 1;
    }

    char pid_str[32];
    int len = sprintf(pid_str, "%d\n", getpid());
    write(fd, pid_str, len);
    close(fd);

    printf("START:Monitor turned on successfully. PID %d. Waiting..\n", getpid());
    fflush(stdout);


    struct sigaction sa_int, sa_usr;

    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    sa_usr.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr.sa_mask);
    sa_usr.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr, NULL);
    

    while (keep_running) {
        pause(); 
    }

    unlink(".monitor_pid");
    return 0;
}