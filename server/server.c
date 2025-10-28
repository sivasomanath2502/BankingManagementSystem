#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include "../include/bank_ops.h"

#define PORT 8080
#define MAX_USERS 100

typedef struct {
    int userID;
    int active;
} Session;

Session sessions[MAX_USERS];
pthread_mutex_t session_lock = PTHREAD_MUTEX_INITIALIZER;

int server_running = 1;
int server_fd;

int is_user_logged_in(int userID) {
    pthread_mutex_lock(&session_lock);
    for (int i = 0; i < MAX_USERS; i++) {
        if (sessions[i].active && sessions[i].userID == userID) {
            pthread_mutex_unlock(&session_lock);
            return 1;
        }
    }
    pthread_mutex_unlock(&session_lock);
    return 0;
}

void add_session(int userID) {
    pthread_mutex_lock(&session_lock);
    for (int i = 0; i < MAX_USERS; i++) {
        if (!sessions[i].active) {
            sessions[i].active = 1;
            sessions[i].userID = userID;
            break;
        }
    }
    pthread_mutex_unlock(&session_lock);
}

void remove_session(int userID) {
    pthread_mutex_lock(&session_lock);
    for (int i = 0; i < MAX_USERS; i++) {
        if (sessions[i].active && sessions[i].userID == userID) {
            sessions[i].active = 0;
            sessions[i].userID = 0;
            break;
        }
    }
    pthread_mutex_unlock(&session_lock);
}

void *server_command_listener(void *arg) {
    char cmd[32];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (fgets(cmd, sizeof(cmd), stdin)) {
            cmd[strcspn(cmd, "\n")] = 0;
            if (strcasecmp(cmd, "exit") == 0) {
                server_running = 0;
                printf("🛑 Server shutting down...\n");
                shutdown(server_fd, SHUT_RDWR);
                close(server_fd);
                break;
            }
        }
    }
    return NULL;
}

void handle_sigint(int sig) {
    printf("\n🛑 Caught Ctrl+C. Shutting down server...\n");
    server_running = 0;
    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    exit(0);
}

void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    char id[32], pwd[32], role[32];
    int choice, status;
    char buf[4096];
    double amount;

    read(sock, id, sizeof(id));
    read(sock, pwd, sizeof(pwd));

    if (!validate_user(id, pwd, role)) {
        write(sock, "Invalid", sizeof("Invalid"));
        close(sock);
        return NULL;
    }

    int userID = atoi(id);
    if (is_user_logged_in(userID)) {
        strcpy(role, "AlreadyLoggedIn");
        write(sock, role, sizeof(role));
        printf("⚠️ Duplicate login attempt for %d\n", userID);
        close(sock);
        return NULL;
    }

    add_session(userID);
    write(sock, role, sizeof(role));
    printf("✅ %s logged in (ID=%d)\n", role, userID);

    // ---------------- CUSTOMER ----------------
    if (strcmp(role, "Customer") == 0) {
        while (read(sock, &choice, sizeof(choice)) > 0) {
            switch (choice) {
                case 1: { // View Balance
                    double bal;
                    view_balance(userID, &bal);
                    snprintf(buf, sizeof(buf), "Balance: %.2f", bal);
                    write(sock, buf, strlen(buf) + 1);
                    break;
                }
                case 2: { // Deposit
                    read(sock, &amount, sizeof(amount));
                    update_balance(userID, amount, 1);
                    record_transaction(userID, "Deposit", amount);
                    write(sock, "Deposit successful.", 20);
                    break;
                }
                case 3: { // Withdraw
                    read(sock, &amount, sizeof(amount));
                    double bal;
                    view_balance(userID, &bal);
                    if (amount > bal)
                        write(sock, "Insufficient balance.", 22);
                    else {
                        update_balance(userID, amount, 0);
                        record_transaction(userID, "Withdraw", amount);
                        write(sock, "Withdrawal successful.", 23);
                    }
                    break;
                }
                case 4: { // Transfer
                    int target;
                    read(sock, &target, sizeof(target));
                    read(sock, &amount, sizeof(amount));
                    double bal;
                    view_balance(userID, &bal);
                    if (amount > bal)
                        write(sock, "Insufficient balance.", 22);
                    else {
                        update_balance(userID, amount, 0);
                        update_balance(target, amount, 1);
                        record_transaction(userID, "TransferOut", amount);
                        record_transaction(target, "TransferIn", amount);
                        write(sock, "Transfer successful.", 21);
                    }
                    break;
                }
                case 5: { // Loan
                    read(sock, &amount, sizeof(amount));
                    apply_loan(userID, amount);
                    write(sock, "Loan applied successfully.", 27);
                    break;
                }
                case 6: { // Change Password
                    char newpwd[64];
                    read(sock, newpwd, sizeof(newpwd));
                    change_password(userID, newpwd);
                    write(sock, "Password changed successfully.", 31);
                    break;
                }
                case 7: { // Feedback
                    char feedback[256];
                    read(sock, feedback, sizeof(feedback));
                    add_feedback(userID, feedback);
                    write(sock, "Feedback added.", 16);
                    break;
                }
                case 8: { // Transactions
                    view_transaction_history(userID, buf, sizeof(buf));
                    write(sock, buf, strlen(buf) + 1);
                    break;
                }
                case 9: // Logout
                    printf("👋 Customer %d logged out.\n", userID);
                    remove_session(userID);
                    close(sock);
                    return NULL;
                case 10: // Exit
                    printf("👋 Customer %d exited.\n", userID);
                    remove_session(userID);
                    close(sock);
                    return NULL;
            }
        }
    }

   // ---------------- EMPLOYEE ----------------
    else if (strcmp(role, "Employee") == 0) {
        while (read(sock, &choice, sizeof(choice)) > 0) {
            switch (choice) {
                case 1: { // Add New Customer
                    char pwd[64];
                    read(sock, pwd, sizeof(pwd));

                    int newID = add_new_customer(pwd);
                    if (newID > 0)
                        snprintf(buf, sizeof(buf), "✅ Customer %d added (default balance: ₹0.00)", newID);
                    else if (newID == -2)
                        snprintf(buf, sizeof(buf), "❌ Password too short. Minimum 3 characters required.");
                    else
                        snprintf(buf, sizeof(buf), "❌ Failed to add customer. Try again.");

                    write(sock, buf, strlen(buf) + 1);
                    break;
                }

                case 2: { // Modify Customer Password
                    int cid;
                    char newpwd[64];
                    read(sock, &cid, sizeof(cid));
                    read(sock, newpwd, sizeof(newpwd));
                    status = modify_customer_password(cid, newpwd);
                    write(sock, status == 0 ? "✅ Password updated." : "❌ Update failed.", 22);
                    break;
                }
                case 3: // View Assigned Loans
                    view_loans(userID, buf, sizeof(buf));
                    write(sock, buf, strlen(buf) + 1);
                    break;
                case 4: { // Approve/Reject Loan
                    int cid;
                    char st[32];
                    read(sock, &cid, sizeof(cid));
                    read(sock, st, sizeof(st));
                    status = update_loan_status(cid, st);
                    write(sock, status == 0 ? "✅ Loan status updated." : "❌ Update failed.", 25);
                    break;
                }
                case 5: // View Customer Feedbacks
                    view_feedbacks(buf, sizeof(buf));
                    write(sock, buf, strlen(buf) + 1);
                    break;
                case 6: { // View Customer Transactions
                    int custID;
                    read(sock, &custID, sizeof(custID));

                    view_transaction_history(custID, buf, sizeof(buf));
                    write(sock, buf, strlen(buf) + 1);
                    break;
                }   
                case 7:
                    printf("👋 Employee %d logged out.\n", userID);
                    remove_session(userID);
                    close(sock);
                    return NULL;
                case 8:
                    printf("👋 Employee %d exited.\n", userID);
                    remove_session(userID);
                    close(sock);
                    return NULL;
            }
        }
    }


    remove_session(userID);
    close(sock);
    return NULL;
}

int main() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    signal(SIGINT, handle_sigint);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("🚀 Server started on port %d\n", PORT);
    pthread_t cmd_thread;
    pthread_create(&cmd_thread, NULL, server_command_listener, NULL);

    while (server_running) {
        int client_sock = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_sock < 0) {
            if (!server_running) break;
            continue;
        }
        printf("🔗 Client connected.\n");
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, new_sock);
        pthread_detach(tid);
    }

    printf("✅ Server stopped.\n");
    return 0;
}

