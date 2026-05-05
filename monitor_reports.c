#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig){
    char msg[] = "\n[Monitor] Received SIGINT. Closing monitor...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    keep_running = 0;
}

void handle_sigusr1(int sig){
    char msg[] = "[Monitor] Alert : New report has been addded.\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main(){
    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if(fd < 0){
        perror("Error creating monitor.pid");
        return 1;
    }

    char pid_str[32];
    int len = sprintf(pid_str, "%d\n", getpid());
    write(fd,pid_str,len);
    close(fd);

    printf("[Monitor] Turned on successfully. PID %d. Waiting..\n",getpid());

    struct sigaction sa_int, sa_usr;


    // config sigint
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    //config sigusr1
    sa_usr.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr.sa_mask);
    sa_usr.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr, NULL);
    while (keep_running) {
        pause(); 
    }

    //stergem fisierul
    unlink(".monitor_pid");
    printf("[Monitor] Fisierul .monitor_pid a fost sters. Executie terminata.\n");

    return 0;

}