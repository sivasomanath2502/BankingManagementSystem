#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/bank_ops.h"

// -------------------- USER AUTH --------------------
int validate_user(const char *id, const char *pwd, char *role) {
    FILE *fp = fopen("data/users.dat", "r");
    if (!fp) return 0;

    char fid[32], fpwd[32], frole[32];
    while (fscanf(fp, "%31[^:]:%31[^:]:%31s\n", fid, fpwd, frole) == 3) {
        if (strcmp(fid, id) == 0 && strcmp(fpwd, pwd) == 0) {
            strcpy(role, frole);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
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

    char fid[32], fpwd[32], frole[32];
    char idbuf[16];
    sprintf(idbuf, "%d", userID);
    int changed = 0;

    while (fscanf(fp, "%31[^:]:%31[^:]:%31s\n", fid, fpwd, frole) == 3) {
        if (strcmp(fid, idbuf) == 0) {
            fprintf(tmp, "%s:%s:%s\n", fid, newpwd, frole);
            changed = 1;
        } else {
            fprintf(tmp, "%s:%s:%s\n", fid, fpwd, frole);
        }
    }

    fclose(fp);
    fclose(tmp);
    remove("data/users.dat");
    rename("data/tmp_users.dat", "data/users.dat");
    return changed ? 0 : -1;
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
    if (password == NULL || strlen(password) < 3)
        return -2;  // Invalid password (too short)

    FILE *fp_users = fopen("data/users.dat", "a+");
    FILE *fp_acc = fopen("data/accounts.dat", "a+");
    if (!fp_users || !fp_acc) return -1;

    // --- Find highest Customer ID only ---
    int lastCustID = 1000;  // start from 1001 for first customer
    rewind(fp_users);
    char fid[32], fpwd[32], frole[32];

    while (fscanf(fp_users, "%31[^:]:%31[^:]:%31s\n", fid, fpwd, frole) == 3) {
        int id = atoi(fid);
        if (strcmp(frole, "Customer") == 0 && id > lastCustID)
            lastCustID = id;
    }

    int newCustID = lastCustID + 1;

    // --- Append new records ---
    fprintf(fp_users, "%d:%s:Customer\n", newCustID, password);
    fprintf(fp_acc, "%d:0.00\n", newCustID);

    fclose(fp_users);
    fclose(fp_acc);

    return newCustID;  // return generated ID
}

// -------------------- MODIFY CUSTOMER PASSWORD --------------------
int modify_customer_password(int custID, const char *newpwd) {
    FILE *fp = fopen("data/users.dat", "r");
    FILE *tmp = fopen("data/tmp_users.dat", "w");
    if (!fp || !tmp) return -1;

    char fid[32], fpwd[32], frole[32];
    char idbuf[16];
    sprintf(idbuf, "%d", custID);
    int modified = 0;

    while (fscanf(fp, "%31[^:]:%31[^:]:%31s\n", fid, fpwd, frole) == 3) {
        if (strcmp(fid, idbuf) == 0 && strcmp(frole, "Customer") == 0) {
            fprintf(tmp, "%s:%s:%s\n", fid, newpwd, frole);
            modified = 1;
        } else {
            fprintf(tmp, "%s:%s:%s\n", fid, fpwd, frole);
        }
    }

    fclose(fp);
    fclose(tmp);
    remove("data/users.dat");
    rename("data/tmp_users.dat", "data/users.dat");

    return modified ? 0 : -1;
}

