#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

// ---------- Safe I/O ----------
ssize_t safe_read(int sock, void *buf, size_t size) {
    ssize_t n = read(sock, buf, size - 1);
    if (n <= 0) {
        printf("\n🛑 Server closed connection. Client disconnected gracefully.\n");
        close(sock);
        exit(0);
    }
    ((char *)buf)[n] = '\0';
    return n;
}

ssize_t safe_write(int sock, const void *buf, size_t size) {
    ssize_t n = write(sock, buf, size);
    if (n <= 0) {
        printf("\n🛑 Server closed connection. Client disconnected gracefully.\n");
        close(sock);
        exit(0);
    }
    return n;
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
            default:
                printf("⚠️  Invalid choice! Please select a valid option (1–10).\n");
                break;

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
            default:
                printf("⚠️  Invalid choice! Please select a valid option (1–8).\n");
                break;
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
            default:
                printf("⚠️  Invalid choice! Please select a valid option (1–7).\n");
                break;
        }
    }
}

void admin_menu(int sock) {
    int choice, userID;
    char buffer[4096], newpwd[64], newrole[32];

    while (1) {
        printf("\n--- Admin Menu ---\n"
               "1. Add New Bank Employee\n"
               "2. Modify Customer/Employee Details\n"
               "3. Manage User Roles\n"
               "4. Change Password\n"
               "5. Logout\n"
               "6. Exit\n"
               "Enter choice: ");
        scanf("%d", &choice);
        write(sock, &choice, sizeof(choice));

        switch (choice) {
            case 1:
                printf("Enter new employee password: ");
                scanf("%s", newpwd);
                write(sock, newpwd, sizeof(newpwd));
                read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 2:
                printf("Enter User ID to modify: ");
                scanf("%d", &userID);
                printf("Enter new password: ");
                scanf("%s", newpwd);
                write(sock, &userID, sizeof(userID));
                write(sock, newpwd, sizeof(newpwd));
                read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 3:
                printf("Enter User ID to change role: ");
                scanf("%d", &userID);
                printf("Enter new role (Employee/Manager): ");
                scanf("%s", newrole);
                write(sock, &userID, sizeof(userID));
                write(sock, newrole, sizeof(newrole));
                read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 4:
                printf("Enter new admin password: ");
                scanf("%s", newpwd);
                write(sock, newpwd, sizeof(newpwd));
                read(sock, buffer, sizeof(buffer));
                printf("%s\n", buffer);
                break;

            case 5:
                printf("Logging out...\n");
                return;

            case 6:
                printf("Exiting program...\n");
                exit(0);
            default:
                printf("⚠️  Invalid choice! Please select a valid option (1–6).\n");
                break;

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
//IP Address - 10.10.3.67
        inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
            perror("❌ Connection failed");
            printf("⚠️  Server might be offline. Try again later.\n");
            close(soc
            sleep(2);
            continue;
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
        else if (strcmp(role, "Admin") == 0)
            admin_menu(sock);

        close(sock);

        char again;
        printf("\n🔁 Do you want to login again? (y/n): ");
        scanf(" %c", &again);
        if (again == 'n' || again == 'N') break;
    }

    printf("👋 Exiting client.\n");
    return 0;
}
