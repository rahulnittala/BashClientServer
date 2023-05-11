// Client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define BUFFER_SIZE 32768

static int findfilecommand=0;
static int sgetfilescommand=0;
static int unzip=0;
static int getfiles_cmd=0;
static int dgetfiles_cmd=0;
static int gettargz_cmd=0;
static int quit_cmd=0;

// check the input command
int verifySyntax(char enteredCommand[BUFFER_SIZE]){
    char checkSyntaxCommand[BUFFER_SIZE];
    char *checkEnteredCommand;
    strcpy(checkSyntaxCommand,enteredCommand);
	checkEnteredCommand = strtok(checkSyntaxCommand," ");
    if(strcmp(checkEnteredCommand, "findfile") == 0){
        findfilecommand=1;
    }
    if(strcmp(checkEnteredCommand, "sgetfiles") == 0){
        sgetfilescommand=1;
        char *substr = "-u";
        char *res = strstr(enteredCommand, substr);
        if(res != NULL){
            unzip=1;
        }
    }
    if(strcmp(checkEnteredCommand, "getfiles") == 0){
        getfiles_cmd=1;
        char *substr = "-u";
        char *res = strstr(enteredCommand, substr);
        if(res != NULL){
            unzip=1;
        }
    }
    if(strcmp(checkEnteredCommand, "dgetfiles") == 0){
        dgetfiles_cmd=1;
        char *substr = "-u";
        char *res = strstr(enteredCommand, substr);
        if(res != NULL){
            unzip=1;
        }
    }
    if(strcmp(checkEnteredCommand, "gettargz") == 0){
        gettargz_cmd=1;
        char *substr = "-u";
        char *res = strstr(enteredCommand, substr);
        if(res != NULL){
            unzip=1;
        }
    }
    if(strcmp(checkEnteredCommand, "quit") == 0){
        quit_cmd=1;
    }
    if(strcmp(checkEnteredCommand,"findfile")==0 || strcmp(checkEnteredCommand,"sgetfiles")==0 || strcmp(checkEnteredCommand,"dgetfiles")==0 || strcmp(checkEnteredCommand,"getfiles")==0 || strcmp(checkEnteredCommand,"gettargz")==0 || strcmp(checkEnteredCommand,"quit")==0){
		return 1;
	}
	return 0;
}

// get argument count for input command
int arguments_count(char command[BUFFER_SIZE]){
    char temp_cmd[BUFFER_SIZE];
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

struct date {
    int day;
    int month;
    int year;
};

// verifying size arguments in the command
int verify_arguments(char command[BUFFER_SIZE]){
    char temp_cmd[BUFFER_SIZE];
    char *args = NULL;
    strcpy(temp_cmd, command);
    args = strtok(temp_cmd, " ");
    int arg_count = 0;
    if(dgetfiles_cmd == 1){
        struct date date1, date2;
        while(args != NULL){
            arg_count++;
            if(arg_count == 2){
                sscanf(args, "%d-%d-%d", &date1.year, &date1.month, &date1.day);
            }
            else if(arg_count == 3){
                sscanf(args, "%d-%d-%d", &date2.year, &date2.month, &date2.day);
            }
            args = strtok(NULL, " ");
        }
        // checking the conditions for date1 and date2
        if (date1.year < date2.year) {
            return 1;
        } else if (date1.year > date2.year) {
            return 0;
        } else {
            if (date1.month < date2.month) {
                return 1;
            } else if (date1.month > date2.month) {
                return 0;
            } else {
                if (date1.day <= date2.day) {
                    return 1;
                } else {
                    return 0;
                }
            }
        }
    }
    // for sgetfiles command
    else{
        int size1,size2;
        char temp_cmd[BUFFER_SIZE];
        char *args = NULL;
        strcpy(temp_cmd, command);
        args = strtok(temp_cmd, " ");
        int arg_count = 0;

        while(args != NULL){
            arg_count++;
            if(arg_count == 2){
                size1 = atoi(args);
            }
            else if(arg_count == 3){
                size2 = atoi(args);
            }
            args = strtok(NULL, " ");
        }

        // conditions for size1 and size2 validation
        if(size1<=size2 && size1>=0 && size2>=0){
            return 1;
        }
    }
    return 0;
}

// function to unzip
void unzip_func(){
    int r = system("tar -xvf temp.tar.gz");
    if(r == 0){
        printf("\n All files Unzipped successfully!\n");
    }else{
        printf("Unzipping the tar file failed!\n");
    }
}

// CLIENT
int main(int argc, char *argv[]) {
	int client_socket;
    struct sockaddr_in server_address;

    // check the port and address arguments
    if(argc != 3){
        printf("Please enter port_number and IP_address as arguments!\nExample './client port_number server_ip_address'\n");
    }else{
        int port_num = atoi(argv[1]);

        // Create the client socket
        if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        // Connect the client socket to the server
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons((uint16_t)port_num);

        if (inet_pton(AF_INET, argv[2], &server_address.sin_addr) < 0) {
            perror("inet_pton failed");
            exit(EXIT_FAILURE);
        }

        if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
            perror("connect failed");
            exit(EXIT_FAILURE);
        }

        // Run an infinite loop waiting for the user to enter one of the commands
        char command[BUFFER_SIZE];

        while (1) {
            sgetfilescommand=0;
            findfilecommand=0;
            unzip=0;
            getfiles_cmd=0;
            dgetfiles_cmd=0;
            gettargz_cmd=0;
            quit_cmd=0;
            
            printf(">> Enter a command: ");
            fflush(stdout);

            // Read a command from the user
            memset(command, 0, sizeof(command));
            fgets(command, sizeof(command), stdin);

            // Remove the newline character from the end of the command
            command[strcspn(command, "\n")] = 0;

            // Verify the syntax of the command
            int valid_command = 0;

            //code to verify the syntax of the command
            
            if(verifySyntax(command)==1){
                valid_command=1;
            }
            
            // If the command is valid, send it to the server
            if (valid_command) {
                int arg_count = arguments_count(command);
                // COMMAND - quit
                if(quit_cmd == 1){
                    if((arg_count != 1)){
                        printf("Invalid command! quit command contains no arguments\n");
                    }else{
                        // Send quit command message to server
                        int res = write(client_socket, command, strlen(command));
                        if (res < 0) {
                            perror("write failed");
                            exit(EXIT_FAILURE);
                        }
                    }
                    close(client_socket);
                    return 0;
                }
                // COMMAND - gettargz
                if(gettargz_cmd == 1){
                    // check the argument count
                    if((arg_count < 2) || (arg_count > 8) || (unzip == 0 && arg_count > 7)){
                        printf("Invalid number of arguments! file types should be >= 1 and <=6!\n");
                    }else{
                        char filename[BUFFER_SIZE], buffer[BUFFER_SIZE];
                        int file_fd;

                        // Send command to server
                        int res = write(client_socket, command, strlen(command));
                        if (res < 0) {
                            perror("write failed");
                            exit(EXIT_FAILURE);
                        }

                        // Read the response from the server and print it to the console
                        char read_buffer[BUFFER_SIZE];
                        memset(read_buffer, 0, sizeof(read_buffer));
                        int n = read(client_socket, read_buffer, sizeof(read_buffer));
                        if (n < 0) {
                            perror("read failed\n");
                            exit(EXIT_FAILURE);
                        }
                        if(strcmp(read_buffer, "0") == 0){
                            printf("No file found!\n");
                        }else{
                            // receive file data from server
                            long file_size;
                            recv(client_socket, &file_size, sizeof(file_size), 0);

                            size_t bytes_read = 0;

                            // create tar file
                            FILE* file = fopen("temp.tar.gz", "wb");
                            if (!file) {
                                perror("Error creating file");
                                exit(EXIT_FAILURE);
                            }

                            // write the file data into the tar file created
                            while (bytes_read < file_size) {
                                size_t remaining = file_size - bytes_read;
                                size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);

                                ssize_t result = recv(client_socket, buffer, to_read, 0);
                                if (result < 0) {
                                    perror("Error receiving file");
                                    exit(EXIT_FAILURE);
                                }

                                fwrite(buffer, sizeof(char), result, file);
                                bytes_read += result;
                            }
                            printf("File received from server Successfully!\n");

                            fclose(file);

                            // condition to unzip the tar file
                            if(unzip == 1){
                                unzip_func();
                            }
                        }
                    }
                }

                // COMMAND - dgetfiles
                if(dgetfiles_cmd == 1){
                    // check the argument count
                    if(arg_count < 3 || arg_count > 4){
                        printf("Invalid number of arguments!\n");
                    }else{
                        int proceed = verify_arguments(command);

                        // validating the input size arguments
                        if(proceed == 0){
                            printf("Invalid inputs date1 and date2 values given!\n");
                        }
                        else{
                            char filename[BUFFER_SIZE], buffer[BUFFER_SIZE];
                            int file_fd;

                            // Send command to server
                            int res = write(client_socket, command, strlen(command));
                            if (res < 0) {
                                perror("write failed");
                                exit(EXIT_FAILURE);
                            }

                            // receive data from server
                            long file_size;
                            recv(client_socket, &file_size, sizeof(file_size), 0);

                            size_t bytes_read = 0;

                            // create tar file
                            FILE* file = fopen("temp.tar.gz", "wb");
                            if (!file) {
                                perror("Error creating file");
                                exit(EXIT_FAILURE);
                            }

                            // write the file data into the tar file created
                            while (bytes_read < file_size) {
                                size_t remaining = file_size - bytes_read;
                                size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);

                                ssize_t result = recv(client_socket, buffer, to_read, 0);
                                if (result < 0) {
                                    perror("Error receiving file");
                                    exit(EXIT_FAILURE);
                                }

                                fwrite(buffer, sizeof(char), result, file);
                                bytes_read += result;
                            }
                            printf("File received from server Successfully!\n");

                            fclose(file);

                            // condition to unzip the tar file
                            if(unzip == 1){
                                unzip_func();
                            }
                        }
                    } 
                }

                // COMMAND - getfiles
                if(getfiles_cmd == 1){
                    // check the argument count
                    if((arg_count < 2) || (arg_count > 8) || (unzip == 0 && arg_count > 7)){
                        printf("Invalid number of arguments! files count should be >= 1 and <=6!\n");
                    }else{
                        char filename[BUFFER_SIZE], buffer[BUFFER_SIZE];
                        int file_fd;

                        // Send command to server
                        int res = write(client_socket, command, strlen(command));
                        if (res < 0) {
                            perror("write failed");
                            exit(EXIT_FAILURE);
                        }

                        // Read the response from the server and print it to the console
                        char read_buffer[BUFFER_SIZE];
                        memset(read_buffer, 0, sizeof(read_buffer));
                        int n = read(client_socket, read_buffer, sizeof(read_buffer));
                        if (n < 0) {
                            perror("read failed\n");
                            exit(EXIT_FAILURE);
                        }
                        if(strcmp(read_buffer, "0") == 0){
                            printf("No file found!\n");
                        }else{
                            // receive file data from server
                            long file_size;
                            recv(client_socket, &file_size, sizeof(file_size), 0);

                            size_t bytes_read = 0;

                            // create tar file
                            FILE* file = fopen("temp.tar.gz", "wb");
                            if (!file) {
                                perror("Error creating file");
                                exit(EXIT_FAILURE);
                            }

                            // write the file data into the tar file created
                            while (bytes_read < file_size) {
                                size_t remaining = file_size - bytes_read;
                                size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);

                                ssize_t result = recv(client_socket, buffer, to_read, 0);
                                if (result < 0) {
                                    perror("Error receiving file");
                                    exit(EXIT_FAILURE);
                                }

                                fwrite(buffer, sizeof(char), result, file);
                                bytes_read += result;
                            }
                            printf("File received from server Successfully!\n");

                            fclose(file);

                            // condition to unzip the tar file
                            if(unzip == 1){
                                unzip_func();
                            }
                        }
                    }
                }

                // COMMAND - sgetfiles
                else if(sgetfilescommand==1){
                    // check the argument count
                    if(arg_count < 3 || arg_count > 4){
                        printf("Invalid number of arguments!\n");
                    }else{
                        int proceed = verify_arguments(command);

                        // validating the input size arguments
                        if(proceed == 0){
                            printf("Invalid input size1 and size2 values given!\n");
                        }else{
                            char filename[BUFFER_SIZE], buffer[BUFFER_SIZE];
                            int file_fd;

                            // Send command to server
                            int res = write(client_socket, command, strlen(command));
                            if (res < 0) {
                                perror("write failed");
                                exit(EXIT_FAILURE);
                            }

                            // receive data from server
                            long file_size;
                            recv(client_socket, &file_size, sizeof(file_size), 0);

                            size_t bytes_read = 0;

                            // create tar file
                            FILE* file = fopen("temp.tar.gz", "wb");
                            if (!file) {
                                perror("Error creating file");
                                exit(EXIT_FAILURE);
                            }

                            // write the file data into the tar file created
                            while (bytes_read < file_size) {
                                size_t remaining = file_size - bytes_read;
                                size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);

                                ssize_t result = recv(client_socket, buffer, to_read, 0);
                                if (result < 0) {
                                    perror("Error receiving file");
                                    exit(EXIT_FAILURE);
                                }

                                fwrite(buffer, sizeof(char), result, file);
                                bytes_read += result;
                            }
                            printf("File received from server Successfully!\n");

                            fclose(file);

                            // condition to unzip the tar file
                            if(unzip == 1){
                                unzip_func();
                            }
                        }
                    } 
                }
                
                // COMMAND - findfile
                else if(findfilecommand){
                    // check the argument count
                    if(arg_count != 2){
                        printf("Invalid number of arguments!\n");
                    }else{
                        // Send command to server
                        int n = write(client_socket, command, strlen(command));
                        if (n < 0) {
                            perror("write failed");
                            exit(EXIT_FAILURE);
                        }

                        // Read the response from the server and print it to the console
                        char buffer[BUFFER_SIZE];
                        memset(buffer, 0, sizeof(buffer));
                        n = read(client_socket, buffer, sizeof(buffer));

                        if (n < 0) {
                            perror("read failed\n");
                            exit(EXIT_FAILURE);
                        }
                        if(strcmp(buffer, "0") == 0){
                            printf("File not found!\n");
                        }else{
                            printf("Result: %s\n", buffer);
                        }
                    }
                }
            } else {
                printf("Invalid command syntax\n");
            }
        }
    }

    // Close the client socket
    close(client_socket);

    return 0;
}
