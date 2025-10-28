#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/bank_ops.h"

// -------------------- USER AUTH --------------------
int validate_user(const char *id, const char *pwd, char *role) {
    FILE *fp = fopen("data/users.dat", "r");
    if (!fp) return -1;

    char fid[32], fpwd[32], frole[32], fstatus[32];
    while (fscanf(fp, "%31[^:]:%31[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(fid, id) == 0) {
            // Found the user ID — now check status and password
            if (strcmp(frole, "Customer") == 0 && strcmp(fstatus, "inactive") == 0) {
                fclose(fp);
                strcpy(role, "Inactive");
                return 2;  // special code for inactive user
            }

            if (strcmp(fpwd, pwd) == 0) {
                fclose(fp);
                strcpy(role, frole);
                return 1;  // valid login
            } else {
                fclose(fp);
                return 0;  // wrong password
            }
        }
    }

    fclose(fp);
    return 0;  // user not found
}




// -------------------- BALANCE VIEW --------------------
int view_balance(int custID, double *balance) {
    FILE *fp = fopen("data/accounts.dat", "r");
    if (!fp) return -1;

    int id;
    double bal;
    while (fscanf(fp, "%d:%lf\n", &id, &bal) == 2) {
        if (id == custID) {
            *balance = bal;
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return -1;
}

// -------------------- BALANCE UPDATE --------------------
int update_balance(int custID, double amount, int isDeposit) {
    FILE *fp = fopen("data/accounts.dat", "r");
    FILE *temp = fopen("data/tmp_accounts.dat", "w");
    if (!fp || !temp) return -1;

    int id;
    double bal;
    int found = 0;

    while (fscanf(fp, "%d:%lf\n", &id, &bal) == 2) {
        if (id == custID) {
            found = 1;
            bal = isDeposit ? bal + amount : bal - amount;
        }
        fprintf(temp, "%d:%.2f\n", id, bal);
    }

    fclose(fp);
    fclose(temp);

    remove("data/accounts.dat");
    rename("data/tmp_accounts.dat", "data/accounts.dat");

    return found ? 0 : -1;
}

// -------------------- RECORD TRANSACTION --------------------
int record_transaction(int custID, const char *type, double amount) {
    FILE *fp = fopen("data/transactions.dat", "a");
    if (!fp) return -1;

    time_t now = time(NULL);
    char *t = ctime(&now);
    t[strcspn(t, "\n")] = 0; // remove newline

    fprintf(fp, "%d:%s:%.2f:%s\n", custID, type, amount, t);
    fclose(fp);
    return 0;
}

// -------------------- TRANSACTION HISTORY (Passbook Style) --------------------
int view_transaction_history(int custID, char *buffer, size_t size) {
    FILE *fp = fopen("data/transactions.dat", "r");
    if (!fp) {
        snprintf(buffer, size, "No transactions found.\n");
        return -1;
    }

    int id;
    char type[32], ts[64];
    double amt;
    size_t len = 0;
    int found = 0;

    // Header
    len += snprintf(buffer + len, size - len,
        "---------------------------------------------------------------\n"
        "| Date & Time           | Type         | Amount (₹)   |\n"
        "---------------------------------------------------------------\n");

    // Loop through file
    while (fscanf(fp, "%d:%31[^:]:%lf:%63[^\n]\n", &id, type, &amt, ts) == 4) {
        if (id == custID) {
            found = 1;
            len += snprintf(buffer + len, size - len,
                            "| %-20s | %-12s | %-11.2f |\n",
                            ts, type, amt);
            if (len >= size) break;
        }
    }

    fclose(fp);

    if (!found) {
        snprintf(buffer, size, "No transactions found for this account.\n");
        return 0;
    }

    // Footer
    len += snprintf(buffer + len, size - len,
        "---------------------------------------------------------------\n");

    return 0;
}

// -------------------- LOANS --------------------
int apply_loan(int custID, double amount) {
    FILE *fp = fopen("data/loans.dat", "a");
    if (!fp) return -1;
    int assignedEmp = 2001; // default, can be dynamic later
    fprintf(fp, "%d:%.2f:pending:%d\n", custID, amount, assignedEmp);
    fclose(fp);
    return 0;
}

int view_loans(int empID, char *buffer, size_t size) {
    FILE *fp = fopen("data/loans.dat", "r");
    if (!fp) {
        snprintf(buffer, size, "No loan records.\n");
        return -1;
    }
    int cid, assigned;
    char status[32];
    double amt;
    size_t len = 0;

    while (fscanf(fp, "%d:%lf:%31[^:]:%d\n", &cid, &amt, status, &assigned) == 4) {
        if (assigned == empID)
            len += snprintf(buffer + len, size - len,
                            "Customer %d applied for %.2f (%s)\n",
                            cid, amt, status);
    }
    fclose(fp);
    if (len == 0)
        snprintf(buffer, size, "No assigned loans.\n");
    return 0;
}

int update_loan_status(int custID, const char *status) {
    FILE *fp = fopen("data/loans.dat", "r");
    FILE *tmp = fopen("data/tmp_loans.dat", "w");
    if (!fp || !tmp) return -1;

    int cid, assigned;
    char cstatus[32];
    double amt;
    int updated = 0;

    while (fscanf(fp, "%d:%lf:%31[^:]:%d\n", &cid, &amt, cstatus, &assigned) == 4) {
        if (cid == custID) {
            fprintf(tmp, "%d:%.2f:%s:%d\n", cid, amt, status, assigned);
            updated = 1;
        } else {
            fprintf(tmp, "%d:%.2f:%s:%d\n", cid, amt, cstatus, assigned);
        }
    }

    fclose(fp);
    fclose(tmp);
    remove("data/loans.dat");
    rename("data/tmp_loans.dat", "data/loans.dat");
    return updated ? 0 : -1;
}

// -------------------- PASSWORD --------------------
int change_password(int userID, const char *newpwd) {
    FILE *fp = fopen("data/users.dat", "r");
    FILE *tmp = fopen("data/tmp_users.dat", "w");
    if (!fp || !tmp) return -1;

    char fid[32], fpwd[64], frole[32], fstatus[32];
    char idbuf[16];
    sprintf(idbuf, "%d", userID);
    int updated = 0;

    while (fscanf(fp, "%31[^:]:%63[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(fid, idbuf) == 0) {
            // Update password for the matching ID
            fprintf(tmp, "%s:%s:%s:%s\n", fid, newpwd, frole, fstatus);
            updated = 1;
        } else {
            // Keep others unchanged
            fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, frole, fstatus);
        }
    }

    fclose(fp);
    fclose(tmp);
    remove("data/users.dat");
    rename("data/tmp_users.dat", "data/users.dat");
    return updated ? 0 : -1;
}


// -------------------- FEEDBACK --------------------
int add_feedback(int custID, const char *feedback) {
    FILE *fp = fopen("data/feedback.dat", "a");
    if (!fp) return -1;
    fprintf(fp, "%d:%s\n", custID, feedback);
    fclose(fp);
    return 0;
}

int view_feedbacks(char *buffer, size_t size) {
    FILE *fp = fopen("data/feedback.dat", "r");
    if (!fp) {
        snprintf(buffer, size, "No feedback records.\n");
        return -1;
    }

    int id;
    char fb[256];
    size_t len = 0;

    while (fscanf(fp, "%d:%255[^\n]\n", &id, fb) == 2) {
        len += snprintf(buffer + len, size - len,
                        "Customer %d: %s\n", id, fb);
        if (len >= size) break;
    }

    fclose(fp);
    if (len == 0)
        snprintf(buffer, size, "No feedback available.\n");
    return 0;
}
// -------------------- ADD NEW CUSTOMER (Improved UI + Validation) --------------------
int add_new_customer(const char *password) {
    FILE *fp = fopen("data/users.dat", "a+");
    if (!fp) return -1;

    int lastID = 1000;  // Starting customer ID
    // Find the last used customer ID
    FILE *read_fp = fopen("data/users.dat", "r");
    if (read_fp) {
        char fid[32], fpwd[32], frole[32], fstatus[32];
        while (fscanf(read_fp, "%31[^:]:%31[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
            int id = atoi(fid);
            if (id > lastID && strcmp(frole, "Customer") == 0)
                lastID = id;
        }
        fclose(read_fp);
    }

    int newID = lastID + 1;

    // Add new customer with status = active
    fprintf(fp, "%d:%s:Customer:active\n", newID, password);
    fclose(fp);

    // Also initialize account balance = 0
    FILE *acc_fp = fopen("data/accounts.dat", "a");
    if (acc_fp) {
        fprintf(acc_fp, "%d:0.00\n", newID);
        fclose(acc_fp);
    }

    return newID;
}


// -------------------- MODIFY CUSTOMER PASSWORD --------------------
int modify_customer_password(int custID, const char *newpwd) {
    FILE *fp = fopen("data/users.dat", "r");
    FILE *tmp = fopen("data/tmp_users.dat", "w");
    if (!fp || !tmp) return -1;

    char fid[32], fpwd[64], frole[32], fstatus[32];
    char idbuf[16];
    sprintf(idbuf, "%d", custID);
    int modified = 0;

    while (fscanf(fp, "%31[^:]:%63[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(fid, idbuf) == 0 && strcmp(frole, "Customer") == 0) {
            // ✅ Update password but keep role and status same
            fprintf(tmp, "%s:%s:%s:%s\n", fid, newpwd, frole, fstatus);
            modified = 1;
        } else {
            // ✅ Write back unchanged users
            fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, frole, fstatus);
        }
    }

    fclose(fp);
    fclose(tmp);
    remove("data/users.dat");
    rename("data/tmp_users.dat", "data/users.dat");

    return modified ? 0 : -1;
}

// -------------------- MANAGER FUNCTIONS --------------------

// Toggle active/inactive status for customer
int toggle_customer_status(int custID, const char *new_status) {
    FILE *fp = fopen("data/users.dat", "r");
    FILE *tmp = fopen("data/tmp_users.dat", "w");
    if (!fp || !tmp) return -1;

    char fid[32], fpwd[32], frole[32], fstatus[32];
    char idbuf[16];
    sprintf(idbuf, "%d", custID);
    int updated = 0;

    while (fscanf(fp, "%31[^:]:%31[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(fid, idbuf) == 0 && strcmp(frole, "Customer") == 0) {
            fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, frole, new_status);
            updated = 1;
        } else {
            fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, frole, fstatus);
        }
    }

    fclose(fp);
    fclose(tmp);
    remove("data/users.dat");
    rename("data/tmp_users.dat", "data/users.dat");
    return updated ? 0 : -1;
}

// Assign pending loan to an employee
int assign_loan_to_employee(int custID, int empID) {
    FILE *fp = fopen("data/loans.dat", "r");
    FILE *tmp = fopen("data/tmp_loans.dat", "w");
    if (!fp || !tmp) return -1;

    int cid, assigned;
    char status[32];
    double amt;
    int assigned_flag = 0;

    while (fscanf(fp, "%d:%lf:%31[^:]:%d\n", &cid, &amt, status, &assigned) == 4) {
        if (cid == custID && strcmp(status, "pending") == 0) {
            fprintf(tmp, "%d:%.2f:%s:%d\n", cid, amt, status, empID);
            assigned_flag = 1;
        } else {
            fprintf(tmp, "%d:%.2f:%s:%d\n", cid, amt, status, assigned);
        }
    }

    fclose(fp);
    fclose(tmp);
    remove("data/loans.dat");
    rename("data/tmp_loans.dat", "data/loans.dat");
    return assigned_flag ? 0 : -1;
}

// -------------------- VIEW ALL CUSTOMERS --------------------
int view_all_customers(char *buffer, size_t size) {
    FILE *fp_users = fopen("data/users.dat", "r");
    FILE *fp_acc = fopen("data/accounts.dat", "r");
    if (!fp_users || !fp_acc) {
        snprintf(buffer, size, "Unable to read customer records.\n");
        if (fp_users) fclose(fp_users);
        if (fp_acc) fclose(fp_acc);
        return -1;
    }

    // --- Load balances into memory ---
    int ids[1000];
    double balances[1000];
    int count = 0, id;
    double bal;
    while (fscanf(fp_acc, "%d:%lf\n", &id, &bal) == 2) {
        ids[count] = id;
        balances[count++] = bal;
    }

    // --- Prepare output header ---
    size_t len = 0;
    len += snprintf(buffer + len, size - len,
        "---------------------------------------------------------------\n"
        "| Customer ID | Balance (₹)   | Status     |\n"
        "---------------------------------------------------------------\n");

    // --- Match customers with balances ---
    char fid[32], fpwd[32], frole[32], fstatus[32];
    while (fscanf(fp_users, "%31[^:]:%31[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(frole, "Customer") == 0) {
            int custID = atoi(fid);
            double custBal = 0.0;
            for (int i = 0; i < count; i++) {
                if (ids[i] == custID) {
                    custBal = balances[i];
                    break;
                }
            }
            len += snprintf(buffer + len, size - len,
                            "| %-11d | %-13.2f | %-10s |\n", custID, custBal, fstatus);
            if (len >= size) break;
        }
    }

    len += snprintf(buffer + len, size - len,
        "---------------------------------------------------------------\n");

    fclose(fp_users);
    fclose(fp_acc);
    return 0;
}

