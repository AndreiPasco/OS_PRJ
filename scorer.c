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

typedef struct{
    int id;
    char inspector[32];
    float lat;
    float lon;
    char category[32];
    int severity;
    time_t timestamp;
    char description[128];
}Report;

typedef struct{
    char name[32];
    int score;
}InspectorScore;

int main(int argc, char** argv){
    if(argc != 2){
        printf("No arguments.\n");
        return 1;
    }

    char* district = argv[1];
    char filepath[256];
    
    sprintf(filepath, "%s/reports.dat", district); 

    int fd = open(filepath, O_RDONLY);
    if(fd < 0){
        printf("Nu exista rapoarte pentru %s.\n", district);
        return 0;
    }

    InspectorScore scores[50];
    int num_inspectors = 0;
    Report temp;

    while(read(fd,&temp,sizeof(Report)) == sizeof(Report)){
        int found = 0;
        for(int i = 0; i < num_inspectors; i++){
            if(strcmp(scores[i].name, temp.inspector) == 0){
                scores[i].score += temp.severity;
                found = 1;
                break;
            }
        }
        if(!found && num_inspectors < 50){
            strcpy(scores[num_inspectors].name, temp.inspector);
            scores[num_inspectors].score = temp.severity;
            num_inspectors++;
        }
    }
    close(fd);

    printf("\n[ District: %s ]\n", district);
    if(num_inspectors == 0){
        printf("No inspector found.\n");
    }else{
        for(int i = 0; i < num_inspectors; i++){
            printf(" -> Inspector : %-10s | workload score : %d\n", scores[i].name, scores[i].score);
        }
    }
    
    return 0;
}