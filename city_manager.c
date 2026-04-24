#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

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

int check_access(const char* path, const char* role, mode_t required_bit_manager, mode_t required_bit_inspector){
    struct stat st;

    if(stat(path, &st) < 0) return 1;

    if(strcmp(role, "manager") == 0){
        return (st.st_mode & required_bit_manager);
    }else if(strcmp(role,"inspector") == 0){
        return (st.st_mode & required_bit_inspector);
    }
    return 0;
}




int main(int argc, char ** argv){
    char* role = NULL;
    char* user = NULL;
    char* action = NULL;
    char* district = NULL;

    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "--role") == 0 && i+1 < argc){
            role = argv[++i];
        }else if(strcmp(argv[i], "--user") == 0 && i+1 < argc){
            user = argv[++i];
        }else if(strcmp(argv[i], "--add") == 0 && i+1 < argc){
            action = "add";
            district = argv[++i];
        }
    }

    if(!role || !user || !action || !district){
        printf("Eroare de sintaxa! Folosire corecta : \n");
        printf("./city-manager --role manager --user andrei --add downtown\n");
        return 1;
    }

    if(strcmp(action,"add") == 0){

        char filepath[256];
        sprintf(filepath, "%s/reports.dat", district);

        if(!check_access(district, role, S_IWUSR, S_IXGRP)){
            printf("Error : Role %s does not have write/exec permissions on district directory!\n" , role);
            return 1;
        }


        printf("Modul Adaugare Raport ---\n");
        printf("District : %s | Rol : %s | User : %s\n\n", district,role,user);

        mkdir(district, 0750);
        chmod(district, 0750);

        // --- NOU: Crearea fisierului district.cfg (0640) ---
        char cfg_path[256];
        sprintf(cfg_path, "%s/district.cfg", district);
        int cfg_fd = open(cfg_path, O_CREAT | O_WRONLY | O_EXCL, 0640);
        if(cfg_fd >= 0){
            write(cfg_fd, "severity_threshold=2\n", 21);
            chmod(cfg_path, 0640);
            close(cfg_fd);
        }
        // ---------------------------------------------------

        Report nou;
        memset(&nou,0,sizeof(Report));

        nou.id = (int)time(NULL) % 10000;
        strncpy(nou.inspector,user,31);
        nou.timestamp = time(NULL);

        printf("Reading from keyboard : \n");

        printf("Enter the x coordinate (lat) : ");
        scanf("%f", &nou.lat);

        printf("Enter the y coordinate (lon) : ");
        scanf("%f", &nou.lon);

        printf("Enter category (road,lighting,flooding) : ");
        scanf("%31s", nou.category);

        printf("Severity (1-Minor, 2-Moderate, 3-Critical) : ");
        scanf("%d", &nou.severity);

        printf("Short description : ");
        getchar();
        fgets(nou.description,127,stdin);
        nou.description[strcspn(nou.description,"\n")] = 0; // eliminate the enter


        int fd = open(filepath, O_CREAT | O_WRONLY | O_APPEND, 0664);
        if(fd < 0){
            perror("Error opening reports.dat\n");
            return 1;
        }

        write(fd, &nou, sizeof(Report));
        chmod(filepath, 0664);
        close(fd);

        // INAINTE DE LOG: Verificam daca rolul are voie sa scrie in log [cite: 32, 36]
        char logpath[256];
        sprintf(logpath, "%s/logged_district", district);
        if (strcmp(role, "inspector") == 0) {
            printf("Access Denied: Inspectors are not allowed to write to the operation log!\n");
            // Nota: In realitate, poti alege sa continui add-ul dar sa nu scrii logul, 
            // sau sa opresti totul. PDF-ul sugereaza refuzul actiunii.
            return 1; 
        }

        int log_fd = open(logpath, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if(log_fd >= 0 ){
            char log_entry[256];
            int len = sprintf(log_entry, "%ld | %s | %s | add\n", nou.timestamp, role,user); 
            write(log_fd, log_entry, len);
            chmod(logpath, 0644); // Corectat la 0644 conform PDF
            close(log_fd);
        }

        // --- NOU: Crearea Link-ului Simbolic ---
        char symlink_name[256];
        sprintf(symlink_name, "active_reports-%s", district);
        unlink(symlink_name); // Sterge link-ul vechi daca exista pentru a evita erori
        if (symlink(filepath, symlink_name) == 0) {
            printf("Symlink created: %s -> %s\n", symlink_name, filepath);
        }
        // ---------------------------------------

        printf("The report with the ID %d, saved succesfully!\n", nou.id);
    }   
    return 0;
}