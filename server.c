// Server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h> // structure for storing address information
#include <sys/socket.h> // for socket API's
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>

#define BUFFER_SIZE 32768

int getFileByCondition(char *homedir, int size1,int size2,int client_socket,int date, char date1[], char date2[]){
	char command[BUFFER_SIZE];
    if(date == 1){
        snprintf(command, sizeof(command), "find %s -type f -newermt %s ! -newermt %s -print0 | tar -czvf temp.tar.gz --null -T -", homedir, date1, date2);
    }else{
        snprintf(command, sizeof(command), "find %s -type f -size +%dc -size -%dc -print0 | tar -czvf temp.tar.gz --null -T -", homedir, size1, size2);
    }
    // execute the command
    int status = system(command);

    if (status == 0) {
        char *filename = "temp.tar.gz";
        
        // opening the tar file and sending the data to client
        FILE* file = fopen(filename, "rb");
        if (!file) {
           perror("Error opening file");
           exit(EXIT_FAILURE);
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        send(client_socket, &file_size, sizeof(file_size), 0);

        char buffer[BUFFER_SIZE];
        size_t bytes_read;

        while ((bytes_read = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
            send(client_socket, buffer, bytes_read, 0);
        }

        fclose(file);
        
        // removing the file after it is used
        remove(filename);

        return 1;
    } 
    else {
        fprintf(stderr, "Error: No files found!\n");
    }
    return 0;
}

int check_extension(char* filename, char* extension) {
    char* ext = strrchr(filename, '.');
    if (ext == NULL) {
        return 0;
    }
    ext++;
    if (strcmp(ext, extension) == 0) {
        return 1;
    }
    return 0;
}

// search for file in a directory
char *search_file(char *dir_name, char *file_name, int getpath, int check_ext) {
    // printf("entered search file\n");
    DIR *dir;
    int file_found = 0;
    struct dirent *entry;
    struct stat filestat;
    time_t created_time;
    char *fileInfo=malloc(BUFFER_SIZE);

    if ((dir = opendir(dir_name)) == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", dir_name);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char path[BUFFER_SIZE];

        snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);

        if (entry->d_type == DT_DIR) {
            search_file(path, file_name, getpath, check_ext);
        }
        // for gettargz command
        else if(check_ext == 1){
            if (check_extension(entry->d_name, file_name)) {
                fileInfo = "1";
                closedir(dir);
                return fileInfo;
            }
        }
        else if (strcmp(entry->d_name, file_name) == 0) {
            file_found = 1;
            // printf("File found at path: %s\n", path);
            
            if(getpath == 1){
                fileInfo = path;
                closedir(dir);
                return fileInfo;
            }
            if(stat(path, &filestat)<0){
                perror("stat error");
                exit(EXIT_FAILURE);
            }
            created_time = filestat.st_ctime;
            sprintf(fileInfo,"File Found -> filename: '%s', size: %ld bytes, Created on: %s",entry->d_name,filestat.st_size,ctime(&created_time));
            // printf("fileInfo: %s\n", fileInfo);
            closedir(dir);
            return fileInfo;
        }
    }
    if(file_found == 0){
        fileInfo = NULL;
    }
    closedir(dir);
    return fileInfo;
}

int getFileByCommand(char tar_command[], int client_socket){
    printf("tar_command = %s\n", tar_command);
    int status = system(tar_command);

    if (status == 0) {
        char *filename = "temp.tar.gz";
        // opening the tar file and sending the data to client
        FILE* file = fopen(filename, "rb");
        if (!file) {
           perror("Error opening file");
           exit(EXIT_FAILURE);
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        send(client_socket, &file_size, sizeof(file_size), 0);

        char buffer[BUFFER_SIZE];
        size_t bytes_read;

        while ((bytes_read = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
            send(client_socket, buffer, bytes_read, 0);
        }

        fclose(file);
        
        // removing the file after it is used
        remove(filename);

        return 1;
    } 
    else {
        fprintf(stderr, "Error: command failed\n");
    }
    return 0;
}

// get argument count for input command
int arguments_count(char *command){
    char *temp_cmd;
    char *args = NULL;
    strcpy(temp_cmd, command);
    args = strtok(temp_cmd, " ");
    int arg_count = 0;

    while(args != NULL){
        arg_count++;
        args = strtok(NULL, " ");
    }
    return arg_count;
}

// processing clients request/commands
void processclient(int client_socket) {
    char buffer[BUFFER_SIZE];
    int n;

    // get homedirectory
    char *homedir = getenv("HOME");
    if (homedir == NULL) {
        perror("getenv");
        exit(EXIT_FAILURE);
    }

    // for Macbook
    // homedir = "/Users/naveen/Desktop";

    while (1) {
        char *receivedCommand=NULL;
		// Wait for the client to send a command
        memset(buffer, 0, sizeof(buffer));
        n = read(client_socket, buffer, sizeof(buffer));
        if (n < 0) {
            perror("read failed");
            exit(EXIT_FAILURE);
        }
       	buffer[n] = '\0';
        // printf("Received data from client: %s\n", buffer);
		
        receivedCommand = strtok(buffer, " \r\n");
		
		//findfile
        if(strcmp(receivedCommand, "findfile") == 0){
            // printf("entered findfile\n");
            int rcvdcmdCount = 1;
            char *secondParameter=NULL;
            while(receivedCommand !=NULL){
                if(rcvdcmdCount==2){
                    secondParameter = receivedCommand;
                    break;
                }
                receivedCommand=strtok(NULL," ");
                rcvdcmdCount++;
            }
            char *fileInfo=malloc(BUFFER_SIZE);
            fileInfo = search_file(homedir,secondParameter, 0, 0);
            // printf("fileInfo = %s\n", fileInfo);
            if(fileInfo == NULL){
                fileInfo="0";
            }
            n = write(client_socket,fileInfo,strlen(fileInfo));
            if (n < 0) {
                perror("write failed");
                exit(EXIT_FAILURE);
            }
            printf("> Response sent to client!\n");
        }	
		//sgetfiles
		else if(strcmp(receivedCommand, "sgetfiles") == 0){
			int rcvdcmdCount = 1;
			char *secondParameter=NULL;
			char *thirdParameter=NULL;
			int size1,size2;
			while(receivedCommand !=NULL){
				if(rcvdcmdCount == 2){
					secondParameter = receivedCommand;
					size1 = atoi(secondParameter);
				}
				if(rcvdcmdCount == 3){
					thirdParameter = receivedCommand;
					size2 = atoi(thirdParameter);
					break;
				}
				receivedCommand = strtok(NULL," ");
				rcvdcmdCount++;
			}
			int response = getFileByCondition(homedir,size1,size2,client_socket, 0, "0", "0");
            printf("> Response sent to client!\n");
		}
        // getfiles
        else if(strcmp(receivedCommand, "getfiles") == 0){
            int rcvdcmdCount = 1;
            char *token;
            char *arg_files[6];
            int i=0, files_count = 0;
            while(receivedCommand != NULL && files_count<7){
                if(i > 0){
                    arg_files[i-1] = receivedCommand;
                    files_count++;
                }
                receivedCommand = strtok(NULL," ");
                i++;
            }
            char *msg="0";
            int count = 0;
            char tar_command[BUFFER_SIZE];
            char *cmd = "tar -czvf temp.tar.gz";
            snprintf(tar_command, sizeof(tar_command), "%s", cmd);
            
            // checking if atleast one file exist or not and creating a tar command
            for(int j=0;j<files_count;j++){
                char *fileInfo=malloc(BUFFER_SIZE);
                fileInfo = search_file(homedir, arg_files[j], 1, 0);
                // printf("fileInfo = %s j = %d\n", fileInfo, j);
                if(fileInfo != NULL){
                    msg="1";
                    snprintf(tar_command, sizeof(tar_command), "%s %s", tar_command, fileInfo);
                    count++;
                }
            }

            // sending a message to client whether files exist or not
            n = write(client_socket,msg,strlen(msg));
            if (n < 0) {
                perror("write failed");
                exit(EXIT_FAILURE);
            }

            // if atleast one file exist proceed to sending data
            if(strcmp(msg, "1") == 0){
                int response = getFileByCommand(tar_command,client_socket);
                printf("> tar file sent to client!\n");
            }
        }
        // dgetfiles
        else if(strcmp(receivedCommand, "dgetfiles") == 0){
            int rcvdcmdCount = 1;
            char *secondParameter=NULL;
            char *thirdParameter=NULL;
            char date1[11],date2[11];
            while(receivedCommand !=NULL){
                if(rcvdcmdCount == 2){
                    secondParameter = receivedCommand;
                }
                if(rcvdcmdCount == 3){
                    thirdParameter = receivedCommand;
                    break;
                }
                receivedCommand = strtok(NULL," ");
                rcvdcmdCount++;
            }
            struct tm tmp_date={0};
            // Convert input string to a date
            if (strptime(secondParameter, "%Y-%m-%d", &tmp_date) == NULL) {
                printf("Error: Invalid date format\n");
                exit(EXIT_FAILURE);
            }
            // Reduce the date by one day
            tmp_date.tm_mday--;
            // Normalize the date if necessary (e.g. if reducing from March 1 to February 28)
            time_t timestamp = mktime(&tmp_date);
            localtime_r(&timestamp, &tmp_date);

            strftime(date1, sizeof(date1), "%Y-%m-%d", &tmp_date);
            
            struct tm tmp_date2={0};
            // Convert input string to a date
            if (strptime(thirdParameter, "%Y-%m-%d", &tmp_date2) == NULL) {
                printf("Error: Invalid date format\n");
                exit(EXIT_FAILURE);
            }

            // Reduce the date by one day
            tmp_date2.tm_mday++;

            // Normalize the date if necessary (e.g. if reducing from March 1 to February 28)
            timestamp = mktime(&tmp_date2);
            localtime_r(&timestamp, &tmp_date2);

            strftime(date2, sizeof(date2), "%Y-%m-%d", &tmp_date2);

            printf("date1 = %s, date2 = %s\n", date1, date2);

            int response = getFileByCondition(homedir,0,0,client_socket,1,date1,date2);
            printf("> Response sent to client!\n");
        }
        // gettargz
        else if(strcmp(receivedCommand, "gettargz") == 0){
            int rcvdcmdCount = 1;
            char *token;
            char *arg_extensions[6];
            int i=0, extensions_count = 0;
            while(receivedCommand != NULL && extensions_count<7){
                if(i > 0){
                    arg_extensions[i-1] = receivedCommand;
                    extensions_count++;
                }
                receivedCommand = strtok(NULL," ");
                i++;
            }
            char *msg="0";
            int count = 0;
            char tar_command[BUFFER_SIZE];
            snprintf(tar_command, sizeof(tar_command), "find %s -type f -name", homedir);
            
            // checking if atleast one file exist or not and creating a tar command
            for(int j=0;j<extensions_count;j++){
                char *fileInfo=malloc(BUFFER_SIZE);
                fileInfo = search_file(homedir, arg_extensions[j], 0, 1);
                if(fileInfo != NULL){
                    msg="1";
                    snprintf(tar_command, sizeof(tar_command), "%s '*.%s'", tar_command, arg_extensions[j]);
                    count++;
                }
            }

            // sending a message to client whether files exist or not
            n = write(client_socket,msg,strlen(msg));
            if (n < 0) {
                perror("write failed");
                exit(EXIT_FAILURE);
            }

            // if atleast one file exist proceed to sending data
            if(strcmp(msg, "1") == 0){
                char *cmd = "-print0 | tar -czvf temp.tar.gz --null -T -";
                snprintf(tar_command, sizeof(tar_command), "%s %s", tar_command, cmd);
                int response = getFileByCommand(tar_command,client_socket);
                printf("> tar file sent to client!\n");
            }
        }
        // quit command
        else if(strcmp(receivedCommand, "quit") == 0){
            printf("Client selected quit command!\n");
        }
    }
	close(client_socket);
}

// SERVER
int main(int argc, char *argv[]) {
    int server_socket, client_socket, pid, opt=1, port_num;
    // IPv4 addresses
    struct sockaddr_in server_address, client_address;

    printf("Enter port number: ");
    scanf("%d", &port_num);

    // Create the server socket - AF_INET for internet and SOCK_STREAM for TCP
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind the server socket to a port and listen for incoming connections
    
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons((uint16_t)port_num);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // num of connections
    if (listen(server_socket, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", port_num);
    int num_clients = 0;
    while (1) {
        // Accept an incoming client connection
        socklen_t client_address_len = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        num_clients++;
        printf("Client-%d connected!\n", num_clients);

        if(num_clients <= 4 || (num_clients > 8 && num_clients%2 != 0)){
            printf("This connection will be handled by the server!\n");
            // Fork a child process to handle the client request
            pid = fork();

            if (pid == -1) {
                perror("fork failed");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                // We are in the child process
                // Close the server socket in the child process
                close(server_socket);

                // Call the processclient function to handle the client request
                printf("Entering process client function\n");
                processclient(client_socket);

                // Close the client socket in the child process
                close(client_socket);

                // Exit the child process
                exit(EXIT_SUCCESS);
            }
        }else{
            printf("This connection will be handled by the mirror!\n");
        }
    }

    // Close the server socket
    close(server_socket);

    return 0;
}
