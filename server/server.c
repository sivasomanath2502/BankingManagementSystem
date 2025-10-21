#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include "../include/bank_ops.h"

#define PORT 8080
#define MAX_USERS 50


int server_fd;
int active_users[MAX_USERS] = {0};


void *handle_client(void *arg);
int validate_user(const char *id, const char *pwd, char *role);
int is_user_logged_in(int id);
void set_user_login(int id, int status);
void handle_shutdown(int sig);
void *console_handler(void *arg);

void handle_shutdown(int sig) {
    printf("\nCaught signal %d — shutting down server...\n", sig);

    
    for (int i = 0; i < MAX_USERS; i++) {
        if (active_users[i] != 0)
            active_users[i] = 0;
    }

    if (server_fd > 0)
        close(server_fd);

    printf("All sessions cleared. Server stopped safely.\n");
    fflush(stdout);
    exit(0);
}


int main() {
    int new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", PORT);
    fflush(stdout);
	
    pthread_t console_thread;
    pthread_create(&console_thread, NULL, console_handler, NULL);
    pthread_detach(console_thread);
    
    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s:%d\n",
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        fflush(stdout);

        pthread_t tid;
        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;
        pthread_create(&tid, NULL, handle_client, client_sock);
        pthread_detach(tid);
    }
}


void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    char id[32], pwd[32], role[32];
    int custID, choice, status;
    double amt, balance;
    char buf[1024];


    if (read(sock, id, sizeof(id)) <= 0 || read(sock, pwd, sizeof(pwd)) <= 0) {
        close(sock);
        return NULL;
    }

    custID = atoi(id);


    if (is_user_logged_in(custID)) {
        write(sock, "SESSION_ACTIVE", 15);
        close(sock);
        return NULL;
    }


    if (validate_user(id, pwd, role)) {
        write(sock, role, sizeof(role));
    } else {
        write(sock, "INVALID", 8);
        close(sock);
        return NULL;
    }

    if (strcmp(role, "Customer") == 0)
        set_user_login(custID, 1);

    printf("User %d logged in.\n", custID);
    fflush(stdout);

    while (1) {
        ssize_t n = read(sock, &choice, sizeof(choice));
        if (n <= 0) break;

        if (choice == 9 || choice == 10) break; 

        switch (choice) {
            case 1: 
                if (view_balance(custID, &balance) == 0)
                    write(sock, &balance, sizeof(balance));
                else {
                    balance = -1;
                    write(sock, &balance, sizeof(balance));
                }
                break;

            case 2:
                read(sock, &amt, sizeof(amt));
                status = update_balance(custID, amt, 1);
                write(sock, &status, sizeof(status));
                if (status == 0)
                    record_transaction(custID, "Deposit", amt);
                break;

            case 3:
                read(sock, &amt, sizeof(amt));
                status = update_balance(custID, amt, 0);
                write(sock, &status, sizeof(status));
                if (status == 0)
                    record_transaction(custID, "Withdraw", amt);
                break;

            case 4: { 
                int targetID;
                read(sock, &targetID, sizeof(targetID));
                read(sock, &amt, sizeof(amt));
                status = update_balance(custID, amt, 0);
                if (status == 0 && update_balance(targetID, amt, 1) == 0) {
                    record_transaction(custID, "Transfer", amt);
                    record_transaction(targetID, "Received", amt);
                    status = 0;
                } else {
                    update_balance(custID, amt, 1); 
                    status = -1;
                }
                write(sock, &status, sizeof(status));
                break;
            }

            case 5:
                read(sock, &amt, sizeof(amt));
                status = apply_loan(custID, amt);
                write(sock, &status, sizeof(status));
                break;

            case 6: { 
                char newpwd[32];
                read(sock, newpwd, sizeof(newpwd));
                status = change_password(custID, newpwd);
                write(sock, &status, sizeof(status));
                break;
            }

            case 7: 
                read(sock, buf, sizeof(buf));
                status = add_feedback(custID, buf);
                write(sock, &status, sizeof(status));
                break;

            case 8: 
                memset(buf, 0, sizeof(buf));
                if (view_transaction_history(custID, buf, sizeof(buf)) == 0)
                    write(sock, buf, sizeof(buf));
                else
                    write(sock, "No transactions found.\n", 24);
                break;

            default:
                printf("Unknown request from user %d\n", custID);
                break;
        }
    }

    printf("User %d logged out.\n", custID);
    set_user_login(custID, 0);
    close(sock);
    return NULL;
}

int validate_user(const char *id, const char *pwd, char *role) {
    FILE *fp = fopen("data/users.dat", "r");
    if (!fp) return 0;

    char uid[32], upwd[32], urole[32];
    while (fscanf(fp, "%[^:]:%[^:]:%s\n", uid, upwd, urole) == 3) {
        if (!strcmp(id, uid) && !strcmp(pwd, upwd)) {
            strcpy(role, urole);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int is_user_logged_in(int id) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (active_users[i] == id)
            return 1;
    }
    return 0;
}

void set_user_login(int id, int status) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (status == 1 && active_users[i] == 0) {
            active_users[i] = id;
            break;
        } else if (status == 0 && active_users[i] == id) {
            active_users[i] = 0;
            break;
        }
    }
}

void *console_handler(void *arg) {
    char cmd[16];
    while (1) {
        if (fgets(cmd, sizeof(cmd), stdin)) {
            cmd[strcspn(cmd, "\n")] = 0;
            if (strcmp(cmd, "exit") == 0) {
                printf("Exit command received.\n");
                handle_shutdown(SIGINT);
            } else if (strlen(cmd) > 0) {
                printf("Unknown command: %s (type 'exit' to stop server)\n", cmd);
                fflush(stdout);
            }
        }
    }
    return NULL;
}

