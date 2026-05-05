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

void mode_to_string(mode_t mode, char* str){
    strcpy(str,"---------");
    if(mode & S_IRUSR) str[0] = 'r';
    if(mode & S_IWUSR) str[1] = 'w';
    if(mode & S_IXUSR) str[2] = 'x';
    if(mode & S_IRGRP) str[3] = 'r';
    if(mode & S_IWGRP) str[4] = 'w';
    if(mode & S_IXGRP) str[5] = 'x';
    if(mode & S_IROTH) str[6] = 'r';
    if(mode & S_IWOTH) str[7] = 'w';
    if(mode & S_IXOTH) str[8] = 'x';
}

// --- FUNCTII PENTRU COMENZI ---

int action_add(char* district, char* role, char* user) {
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

    char cfg_path[256];
    sprintf(cfg_path, "%s/district.cfg", district);
    int cfg_fd = open(cfg_path, O_CREAT | O_WRONLY | O_EXCL, 0640);
    if(cfg_fd >= 0){
        write(cfg_fd, "severity_threshold=2\n", 21);
        chmod(cfg_path, 0640);
        close(cfg_fd);
    }

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
    nou.description[strcspn(nou.description,"\n")] = 0; 

    int fd = open(filepath, O_CREAT | O_WRONLY | O_APPEND, 0664);
    if(fd < 0){
        perror("Error opening reports.dat\n");
        return 1;
    }
    write(fd, &nou, sizeof(Report));
    chmod(filepath, 0664);
    close(fd);

    char logpath[256];
    sprintf(logpath, "%s/logged_district", district);
    if (strcmp(role, "inspector") == 0) {
        printf("Access Denied: Inspectors are not allowed to write to the operation log!\n");
        return 1; 
    }

    int monitor_notified = 0;
    int pid_fd = open(".monitor_pid",O_RDONLY);
    if(pid_fd >= 0){
        char pid_buf[32];
        memset(pid_buf,0,sizeof(pid_buf));
        int bytes_read = read(pid_fd,pid_buf,sizeof((pid_buf) - 1));
        if(bytes_read > 0){
            pid_t monitor_pid = atoi(pid_buf);
            if(monitor_pid > 0){
                if(kill(monitor_pid,SIGUSR1) == 0){
                    monitor_notified = 1;
                }
            }
        }
    }

    int log_fd = open(logpath, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if(log_fd >= 0){
        char log_entry[512];
        if(monitor_notified){
            int len = sprintf(log_entry, "%ld | %s | %s | add | Notificare Monitor : SUCCESS!\n", nou.timestamp,role,user);
            write(log_fd,log_entry,len);
        }else{
            int len = sprintf(log_entry,"%ld | %s | %s | add | Notificare Monitor : FAILED!\n",nou.timestamp,role,user);
            write(log_fd,log_entry,len);
        }
        chmod(logpath, 0644);
        close(log_fd);
    }

    char symlink_name[256];
    sprintf(symlink_name, "active_reports-%s", district);
    unlink(symlink_name); 
    if (symlink(filepath, symlink_name) == 0) {
        printf("Symlink created: %s -> %s\n", symlink_name, filepath);
    }

    printf("The report with the ID %d, saved succesfully!\n", nou.id);
    return 0;
}

int action_list(char* district, char* role, char* user) {
    char filepath[256];
    sprintf(filepath,"%s/reports.dat", district);

    struct stat st;
    if(stat(filepath,&st) < 0){
        perror("Error: reports.dat not found in this district.");
        return 1;
    }

    char perm_str[10];
    mode_to_string(st.st_mode, perm_str);

    printf("\n--- File Info: %s ---\n", filepath);
    char time_str[26];
    ctime_r(&st.st_mtime, time_str);
    time_str[strcspn(time_str, "\n")] = 0; 
    printf("Permissions: %s | Size: %ld bytes | Last Modified: %s\n", perm_str, st.st_size, time_str);
    printf("----------------------------------------------------------------------------------\n");

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening reports.dat for reading");
        return 1;
    }

    printf("%-6s | %-15s | %-12s | %-8s | %s\n", "ID", "Inspector", "Category", "Severity", "Description");
    printf("----------------------------------------------------------------------------------\n");

    Report temp;
    int count = 0;
    
    while (read(fd, &temp, sizeof(Report)) == sizeof(Report)) {
        printf("%-6d | %-15s | %-12s | %-8d | %s\n", 
               temp.id, temp.inspector, temp.category, temp.severity, temp.description);
        count++;
    }
    close(fd);

    if (count == 0) printf("No reports found.\n");
    else printf("\nTotal: %d reports listed.\n", count);
    
    if (strcmp(role, "manager") == 0) {
        char logpath[256];
        sprintf(logpath, "%s/logged_district", district);
        int log_fd = open(logpath, O_WRONLY | O_APPEND);
        if (log_fd >= 0) {
            char log_entry[256];
            int len = sprintf(log_entry, "%ld | %s | %s | list\n", time(NULL), role, user);
            write(log_fd, log_entry, len);
            close(log_fd);
        }
    }
    return 0;
}

int action_view(char* district, char* target_id_str, char* role, char* user) {
    if (!target_id_str) {
        printf("Eroare: Lipseste ID-ul raportului. Folosire: --view <district> <id>\n");
        return 1;
    }

    int target_id = atoi(target_id_str); 
    char filepath[256];
    sprintf(filepath, "%s/reports.dat", district);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening reports.dat");
        return 1;
    }

    Report temp;
    int found = 0;
    
    while (read(fd, &temp, sizeof(Report)) == sizeof(Report)) {
        if (temp.id == target_id) {
            found = 1;
            printf("\n--- Report Details (ID: %d) ---\n", temp.id);
            printf("Inspector:   %s\n", temp.inspector);
            printf("Location:    Lat %.4f, Lon %.4f\n", temp.lat, temp.lon);
            printf("Category:    %s\n", temp.category);
            printf("Severity:    %d\n", temp.severity);
            
            char time_str[26];
            ctime_r(&temp.timestamp, time_str);
            time_str[strcspn(time_str, "\n")] = 0;
            printf("Reported At: %s\n", time_str);
            
            printf("Description: %s\n", temp.description);
            printf("-------------------------------\n");
            break; 
        }
    }
    close(fd);

    if (!found) {
        printf("Raportul cu ID %d nu a fost gasit in districtul %s.\n", target_id, district);
    } else {
        if (strcmp(role, "manager") == 0) {
            char logpath[256];
            sprintf(logpath, "%s/logged_district", district);
            int log_fd = open(logpath, O_WRONLY | O_APPEND);
            if (log_fd >= 0) {
                char log_entry[256];
                int len = sprintf(log_entry, "%ld | %s | %s | view %d\n", time(NULL), role, user, target_id);
                write(log_fd, log_entry, len);
                close(log_fd);
            }
        }
    }
    return 0;
}

int action_remove(char* district, char* target_id_str, char* role, char* user) {
    if (!target_id_str) {
        printf("Eroare: Lipseste ID-ul. Folosire: --remove_report <district> <id>\n");
        return 1;
    }
    
    if (strcmp(role, "manager") != 0) {
        printf("Access Denied: Only managers can remove reports!\n");
        return 1;
    }

    int target_id = atoi(target_id_str);
    char filepath[256];
    sprintf(filepath, "%s/reports.dat", district);

    int fd = open(filepath, O_RDWR);
    if (fd < 0) {
        perror("Error opening reports.dat");
        return 1;
    }

    Report temp;
    int found = 0;

    while (read(fd, &temp, sizeof(Report)) == sizeof(Report)) {
        if (temp.id == target_id) {
            found = 1;
            break; 
        }
    }

    if (!found) {
        printf("Raportul cu ID %d nu a fost gasit in districtul %s.\n", target_id, district);
        close(fd);
    } else {
        off_t read_pos = lseek(fd, 0, SEEK_CUR); 
        off_t write_pos = read_pos - sizeof(Report); 
        
        while (1) {
            lseek(fd, read_pos, SEEK_SET); 
            int bytes_read = read(fd, &temp, sizeof(Report));
            if (bytes_read != sizeof(Report)) break; 
            
            lseek(fd, write_pos, SEEK_SET); 
            write(fd, &temp, sizeof(Report));
            
            read_pos += sizeof(Report);
            write_pos += sizeof(Report);
        }
        
        ftruncate(fd, write_pos);
        close(fd);
        
        printf("✅ Raportul cu ID %d a fost sters complet, iar fisierul a fost rearanjat.\n", target_id);

        char logpath[256];
        sprintf(logpath, "%s/logged_district", district);
        int log_fd = open(logpath, O_WRONLY | O_APPEND);
        if (log_fd >= 0) {
            char log_entry[256];
            int len = sprintf(log_entry, "%ld | %s | %s | remove %d\n", time(NULL), role, user, target_id);
            write(log_fd, log_entry, len);
            close(log_fd);
        }
    }
    return 0;
}

int action_filter(char* district, char* filter_field, char* filter_value, char* role, char* user) {
    if (!filter_field || !filter_value) {
        printf("Eroare: Argumente incomplete. Folosire: --filter <district> <camp> <valoare>\n");
        printf("Campuri suportate: category, severity, inspector\n");
        return 1;
    }

    char filepath[256];
    sprintf(filepath, "%s/reports.dat", district);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening reports.dat for filtering");
        return 1;
    }

    printf("\n--- Rezultate Filtrare: [%s == %s] ---\n", filter_field, filter_value);
    printf("%-6s | %-15s | %-12s | %-8s | %s\n", "ID", "Inspector", "Category", "Severity", "Description");
    printf("----------------------------------------------------------------------------------\n");

    Report temp;
    int count = 0;
    
    while (read(fd, &temp, sizeof(Report)) == sizeof(Report)) {
        int match = 0;
        
        if (strcmp(filter_field, "category") == 0) {
            if (strcmp(temp.category, filter_value) == 0) match = 1;
        } 
        else if (strcmp(filter_field, "severity") == 0) {
            if (temp.severity == atoi(filter_value)) match = 1; 
        } 
        else if (strcmp(filter_field, "inspector") == 0) {
            if (strcmp(temp.inspector, filter_value) == 0) match = 1;
        }

        if (match) {
            printf("%-6d | %-15s | %-12s | %-8d | %s\n", 
                   temp.id, temp.inspector, temp.category, temp.severity, temp.description);
            count++;
        }
    }
    close(fd);

    if (count == 0) printf("Nu s-au gasit rapoarte care sa corespunda criteriului.\n");
    else printf("\nTotal: %d rapoarte gasite.\n", count);

    if (strcmp(role, "manager") == 0) {
        char logpath[256];
        sprintf(logpath, "%s/logged_district", district);
        int log_fd = open(logpath, O_WRONLY | O_APPEND);
        if (log_fd >= 0) {
            char log_entry[256];
            int len = sprintf(log_entry, "%ld | %s | %s | filter %s %s\n", 
                              time(NULL), role, user, filter_field, filter_value);
            write(log_fd, log_entry, len);
            close(log_fd);
        }
    }
    return 0;
}

int remove_district(char* district, char* role, char* user){
    if(strcmp(role, "manager") != 0){
        printf("Access denied. Only managers can remove districts.");
        return 1;
    }

    char symlink_name[256];
    sprintf(symlink_name,"active_reports-%s", district);

    printf("Removing the district...\n");

    pid_t pid = fork();

    if(pid < 0){
        perror("Error creating process.\n");
        return 1;
    }else if(pid == 0){
        execlp("rm", "rm","-rf", district,symlink_name, NULL);

        perror("Error executing command.");
        exit(1);
    }else{
        int status;
        wait(&status);

        if(WIFEXITED(status) && WEXITSTATUS(status)==0){
            printf("✅ District %s is removed.\n", district);
        }else{
            printf("❌ Erorr removing the district.\n");
        }
    }
    return 0;   
}


//main//

int main(int argc, char ** argv){
    char* filter_field = NULL; 
    char* filter_value = NULL;
    char* role = NULL;
    char* user = NULL;
    char* action = NULL;
    char* district = NULL;
    char* target_id_str = NULL;

    
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "--role") == 0 && i+1 < argc){
            role = argv[++i];
        }else if(strcmp(argv[i], "--user") == 0 && i+1 < argc){
            user = argv[++i];
        }else if(strcmp(argv[i], "--add") == 0 && i+1 < argc){
            action = "add";
            district = argv[++i];
        }else if(strcmp(argv[i], "--list") == 0 && i+1 < argc){
            action = "list";
            district = argv[++i];
        }else if(strcmp(argv[i],"--view") == 0 && i+2 < argc){
            action = "view";
            district = argv[++i];
            target_id_str = argv[++i];
        }else if(strcmp(argv[i], "--remove_report") == 0 && i+2 < argc){
            action = "remove_report";
            district = argv[++i];
            target_id_str = argv[++i];
        }else if(strcmp(argv[i], "--remove_district") == 0 && i+1 < argc){
            action = "remove_district";
            district = argv[++i];
        }else if(strcmp(argv[i],"--filter") == 0 && i + 3  < argc){
            action = "filter";
            district = argv[++i];
            filter_field = argv[++i];
            filter_value = argv[++i];
        }
    }

    if(!role || !user || !action || !district){
        printf("Eroare de sintaxa! Folosire corecta : \n");
        printf("./city-manager --role manager --user andrei --add downtown\n");
        return 1;
    }

    
    if(strcmp(action,"add") == 0){
        return action_add(district, role, user);
    }
    else if(strcmp(action,"list") == 0){
        return action_list(district, role, user);
    }
    else if (strcmp(action, "view") == 0) {
        return action_view(district, target_id_str, role, user);
    }
    else if (strcmp(action, "remove_report") == 0) {
        return action_remove(district, target_id_str, role, user);
    }
    else if (strcmp(action, "remove_district") == 0) {
        return remove_district(district,role,user);
    }else if(strcmp(action,"filter") == 0){
        return action_filter(district,filter_field,filter_value,role,user);
    }

    return 0;
}