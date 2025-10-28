#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

// ---------- Safe I/O ----------
ssize_t safe_read(int sock, void *buf, size_t size) {
    ssize_t n = read(sock, buf, size);
    if (n <= 0) return -1;
    ((char *)buf)[size - 1] = '\0';
    return n;
}

ssize_t safe_write(int sock, const void *buf, size_t size) {
    return write(sock, buf, size);
}


// ---------- Customer Menu ----------
void customer_menu(int sock) {
    int choice;
    double amount;
    int target;
    char buffer[4096], feedback[256], newpwd[64];

    while (1) {
        printf("\n--- Customer Menu ---\n"
               "1. View Balance\n"
               "2. Deposit Money\n"
               "3. Withdraw Money\n"
               "4. Transfer Funds\n"
               "5. Apply for a Loan\n"
               "6. Change Password\n"
               "7. Add Feedback\n"
               "8. View Transaction History\n"
               "9. Logout \n"
               "10. Exit Program\n"
               "Enter choice: ");
        scanf("%d", &choice);
        safe_write(sock, &choice, sizeof(choice));

        switch (choice) {
            case 1:
            case 8:
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 2:
                printf("Enter amount: ");
                scanf("%lf", &amount);
                safe_write(sock, &amount, sizeof(amount));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;
            case 3:
                printf("Enter amount: ");
                scanf("%lf", &amount);
                safe_write(sock, &amount, sizeof(amount));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 4:
                printf("Enter target ID: ");
                scanf("%d", &target);
                printf("Enter amount: ");
                scanf("%lf", &amount);
                safe_write(sock, &target, sizeof(target));
                safe_write(sock, &amount, sizeof(amount));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 5:
                printf("Enter loan amount: ");
                scanf("%lf", &amount);
                safe_write(sock, &amount, sizeof(amount));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 6:
                printf("Enter new password: ");
                scanf("%s",newpwd);
                safe_write(sock, newpwd, sizeof(newpwd));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 7:
                printf("Enter feedback: ");
                getchar();  // clear newline
                fgets(feedback, sizeof(feedback), stdin);
                feedback[strcspn(feedback, "\n")] = 0;
                safe_write(sock, feedback, sizeof(feedback));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 9:
                printf("Logging out...\n");
                return;

            case 10:
                printf("Exiting program...\n");
                exit(0);
        }
    }
}

// ---------- Employee Menu ----------
void employee_menu(int sock) {
    int choice, custID;
    char buffer[4096], pwd[64], status[32], newpwd[64];

    while (1) {
        printf("\n--- Employee Menu ---\n"
               "1. Add New Customer\n"
               "2. Modify Customer Details (Change Customer Password)\n"
               "3. View Assigned Loan Applications\n"
               "4. Approve/Reject Loan\n"
               "5. View Customer Transactions\n"
               "6. Change Your Password\n"
               "7. Logout\n"
               "8. Exit\n"
               "Enter choice: ");
        scanf("%d", &choice);
        safe_write(sock, &choice, sizeof(choice));

        switch (choice) {
            case 1:
                printf( "Enter password for new customer: ");
                scanf("%s",pwd);
                safe_write(sock, pwd, sizeof(pwd));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 2:
                printf("Enter Customer ID: ");
                scanf("%d", &custID);
                printf("Enter new password: ");
                scanf("%s",pwd);
                safe_write(sock, &custID, sizeof(custID));
                safe_write(sock, pwd, sizeof(pwd));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 3:
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;
            case 5:
                printf("Enter Customer ID to view passbook: ");
                scanf("%d", &custID);
                write(sock, &custID, sizeof(custID));
                read(sock, buffer, sizeof(buffer));
                printf("\n--- Transaction History for Customer %d ---\n%s\n", custID, buffer);
                break;


            case 4:
                printf("Enter Customer ID: ");
                scanf("%d", &custID);
                printf("Enter status (approved/rejected): ");
                scanf("%s", status);
                safe_write(sock, &custID, sizeof(custID));
                safe_write(sock, status, sizeof(status));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 6:
                printf("Enter your new password: ");
                scanf("%s",newpwd);
                safe_write(sock, newpwd, sizeof(newpwd));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 7:
                printf("Logging out...\n");
                return;

            case 8:
                printf("Exiting program...\n");
                exit(0);
        }
    }
}

// ---------- Manager Menu ----------
void manager_menu(int sock) {
    int choice, custID, empID;
    char buffer[4096], status[16], newpwd[64];

    while (1) {
        printf("\n--- Manager Menu ---\n"
               "1. Activate/Deactivate Customer Account\n"
               "2. Assign Loan to Employee\n"
               "3. Review Customer Feedback\n"
               "4. Change Password\n"
               "5. View All Customers\n"
               "6. Logout\n"
               "7. Exit\n"
               "Enter choice: ");
        scanf("%d", &choice);
        safe_write(sock, &choice, sizeof(choice));

        switch (choice) {
            case 1:
                printf("Enter Customer ID: ");
                scanf("%d", &custID);
                printf("Enter action (active/inactive): ");
                scanf("%s", status);
                safe_write(sock, &custID, sizeof(custID));
                safe_write(sock, status, sizeof(status));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 2:
                printf("Enter Customer ID (loan applicant): ");
                scanf("%d", &custID);
                printf("Enter Employee ID to assign: ");
                scanf("%d", &empID);
                safe_write(sock, &custID, sizeof(custID));
                safe_write(sock, &empID, sizeof(empID));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 3:
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 4:
                printf("Enter new password: ");
                scanf("%s",newpwd);
                safe_write(sock, newpwd, sizeof(newpwd));
                safe_read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 5:
                safe_read(sock, buffer, sizeof(buffer));
                printf("\n--- All Customer Accounts ---\n%s\n", buffer);
                break;

            case 6:
                printf("Logging out...\n");
                return;

            case 7:
                printf("Exiting program...\n");
                exit(0);
        }
    }
}

// ---------- Main Function ----------
int main() {
    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server = {0};
        server.sin_family = AF_INET;
        server.sin_port = htons(PORT);
        inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
            perror("Connect failed");
            return 1;
        }

        printf("\nConnected to server.\n");
        char id[32], pwd[32], role[32];

        printf("User ID: ");
        scanf("%s", id);
        printf("Password: ");
        scanf("%s",pwd);

        safe_write(sock, id, sizeof(id));
        safe_write(sock, pwd, sizeof(pwd));
        safe_read(sock, role, sizeof(role));

        if (strcmp(role, "Invalid") == 0) {
            printf("❌ Invalid login credentials.\n");
            close(sock);
            continue;
        } 
        else if (strcmp(role, "Inactive") == 0) {
            printf("❌ Your account is currently inactive. Please contact the bank.\n");
            close(sock);
            continue;
        }
        else if (strcmp(role, "AlreadyLoggedIn") == 0) {
            printf("⚠️ This user is already logged in elsewhere.\n");
            close(sock);
            continue;
        }

        printf("✅ Login successful. Role: %s\n", role);

        if (strcmp(role, "Customer") == 0)
            customer_menu(sock);
        else if (strcmp(role, "Employee") == 0)
            employee_menu(sock);
        else if (strcmp(role, "Manager") == 0)
            manager_menu(sock);

        close(sock);

        char again;
        printf("\n🔁 Do you want to login again? (y/n): ");
        scanf(" %c", &again);
        if (again == 'n' || again == 'N') break;
    }

    printf("👋 Exiting client.\n");
    return 0;
}
