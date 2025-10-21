#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_ADDR "127.0.0.1"

void safe_write(int sock, void *buf, size_t size) {
    if (write(sock, buf, size) <= 0) {
        printf("Server disconnected. Closing client.\n");
        close(sock);
        exit(0);
    }
}


ssize_t safe_read(int sock, void *buf, size_t size) {
    ssize_t n = read(sock, buf, size);
    if (n <= 0) {
        printf("Server disconnected. Closing client.\n");
        close(sock);
        exit(0);
    }
    return n;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_ADDR, &serv.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("Connection failed");
        return 1;
    }

    char id[32], pwd[32], role[32];
    printf("User ID: "); scanf("%s", id);
    printf("Password: "); scanf("%s", pwd);

    safe_write(sock, id, sizeof(id));
    safe_write(sock, pwd, sizeof(pwd));
    safe_read(sock, role, sizeof(role));

    if (!strcmp(role, "INVALID")) {
        printf("Invalid credentials.\n");
        close(sock); return 0;
    }
    if (!strcmp(role, "SESSION_ACTIVE")) {
        printf("User already logged in.\n");
        close(sock); return 0;
    }

    printf("Login successful (%s)\n", role);
    if (strcmp(role, "Customer") != 0) return 0;

    int choice, status;
    double amt, balance;
    char buf[1024];

    while (1) {
        printf("\n--- Customer Menu ---\n");
        printf("1. View Balance\n2. Deposit\n3. Withdraw\n4. Transfer Funds\n");
        printf("5. Apply Loan\n6. Change Password\n7. Add Feedback\n");
        printf("8. View Transaction History\n9. Logout / Exit\n");
        printf("Enter choice: "); scanf("%d", &choice);
        safe_write(sock, &choice, sizeof(choice));
        if (choice == 9) break;
	
	        switch (choice) {
            case 1: 
    		    safe_read(sock, &amount, sizeof(amount));
                if (amount >= 0.0)
                    printf("Your balance: %.2f\n", amount);
                else
                    printf("Error retrieving balance.\n");
                break;

            case 2: 
                printf("Enter deposit amount: ");
                if (scanf("%lf", &amount) != 1) { fprintf(stderr,"Invalid input\n"); break; }
                safe_write(sock, &amount, sizeof(amount));
                safe_read(sock, &status, sizeof(status));
                if (status == 0) {
                    printf("Deposit successful.\n");
                } else {
                    printf("Deposit failed (code %d).\n", status);
                }
                break;

            case 3: 
                printf("Enter withdrawal amount: ");
                if (scanf("%lf", &amount) != 1) { fprintf(stderr,"Invalid input\n"); break; }
                safe_write(sock, &amount, sizeof(amount));
                safe_read(sock, &status, sizeof(status));
                if (status == 0) {
                    printf("Withdrawal successful.\n");
                } else if (status == -2) {
                    printf("Withdrawal failed: Insufficient funds.\n");
                } else {
                    printf("Withdrawal failed (code %d).\n", status);
                }
                break;

            case 4: { 
                int targetID;
                printf("Enter receiver Customer ID: ");
                if (scanf("%d", &targetID) != 1) { fprintf(stderr,"Invalid input\n"); break; }
                printf("Enter transfer amount: ");
                if (scanf("%lf", &amount) != 1) { fprintf(stderr,"Invalid input\n"); break; }
                safe_write(sock, &targetID, sizeof(targetID));
                safe_write(sock, &amount, sizeof(amount));
                safe_read(sock, &status, sizeof(status));
                if (status == 0) printf("Transfer successful.\n");
                else if (status == -2) printf("Transfer failed: Insufficient funds.\n");
                else printf("Transfer failed (code %d).\n", status);
                break;
            }

            case 5: 
                printf("Enter loan amount: ");
                if (scanf("%lf", &amount) != 1) { fprintf(stderr,"Invalid input\n"); break; }
                safe_write(sock, &amount, sizeof(amount));
                safe_read(sock, &status, sizeof(status));
                if (status == 0) printf("Loan application submitted.\n");
                else printf("Loan application failed (code %d).\n", status);
                break;

            case 6: { 
                char newpwd[32];
                printf("Enter new password: ");
                if (scanf("%31s", newpwd) != 1) { fprintf(stderr,"Invalid input\n"); break; }
                safe_write(sock, newpwd, sizeof(newpwd));
                safe_read(sock, &status, sizeof(status));
                if (status == 0) printf("Password changed successfully.\n");
                else printf("Password change failed (code %d).\n", status);
                break;
            }

            case 7: {
                char feedback[1024];
                getchar(); 
                printf("Enter feedback (single line): ");
                if (!fgets(feedback, sizeof(feedback), stdin)) { fprintf(stderr,"Input error\n"); break; }
                feedback[strcspn(feedback, "\n")] = 0;
                safe_write(sock, feedback, sizeof(feedback));
                safe_read(sock, &status, sizeof(status));
                if (status == 0) printf("Feedback added. Thank you!\n");
                else printf("Failed to add feedback (code %d).\n", status);
                break;
            }

            case 8: 
                memset(buffer, 0, sizeof(buffer));
                safe_read(sock, buffer, sizeof(buffer));
                printf("\n--- Transaction History ---\n%s\n", buffer);
                break;

            default:
                printf("Invalid choice.\n");
                break;
        }
    }

    printf(" Logged out.\n");
    close(sock);
    return 0;
}

