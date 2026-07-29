#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "../include/bank_ops.h"

// --------------------------------------------------------------------
// One mutex per data file. All threads run inside the same server
// process (thread-per-client model), so plain pthread mutexes are
// sufficient -- we don't need cross-process file locks (flock/fcntl)
// here. Every function that touches a given .dat file takes the
// matching lock before doing ANY read of it and holds it until the
// write-back (rename) is done, so a read-modify-write sequence can
// never be interleaved with another thread's read-modify-write on
// the same file.
//
// Lock ordering rule (to avoid deadlock in functions that touch two
// files): always acquire users_lock before accounts_lock.
// --------------------------------------------------------------------
static pthread_mutex_t users_lock        = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t accounts_lock     = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t loans_lock        = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t transactions_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t feedback_lock     = PTHREAD_MUTEX_INITIALIZER;

// -------------------- USER AUTH --------------------
int validate_user(const char *id, const char *pwd, char *role) {
    pthread_mutex_lock(&users_lock);

    FILE *fp = fopen("data/users.dat", "r");
    if (!fp) {
        pthread_mutex_unlock(&users_lock);
        return -1;
    }

    char fid[32], fpwd[32], frole[32], fstatus[32];
    while (fscanf(fp, "%31[^:]:%31[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(fid, id) == 0) {
            if (strcmp(frole, "Customer") == 0 && strcmp(fstatus, "inactive") == 0) {
                fclose(fp);
                strcpy(role, "Inactive");
                pthread_mutex_unlock(&users_lock);
                return 2;  // special code for inactive user
            }

            if (strcmp(fpwd, pwd) == 0) {
                fclose(fp);
                strcpy(role, frole);
                pthread_mutex_unlock(&users_lock);
                return 1;  // valid login
            } else {
                fclose(fp);
                pthread_mutex_unlock(&users_lock);
                return 0;  // wrong password
            }
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&users_lock);
    return 0;  // user not found
}


// -------------------- BALANCE VIEW --------------------
int view_balance(int custID, double *balance) {
    pthread_mutex_lock(&accounts_lock);

    FILE *fp = fopen("data/accounts.dat", "r");
    if (!fp) {
        pthread_mutex_unlock(&accounts_lock);
        return -1;
    }

    int id;
    double bal;
    while (fscanf(fp, "%d:%lf\n", &id, &bal) == 2) {
        if (id == custID) {
            *balance = bal;
            fclose(fp);
            pthread_mutex_unlock(&accounts_lock);
            return 0;
        }
    }
    fclose(fp);
    pthread_mutex_unlock(&accounts_lock);
    return -1;
}

// -------------------- BALANCE UPDATE (unconditional credit/debit) --------------------
// Used for deposits, and for the "credit the other side" leg of a
// transfer. Callers that need a check-then-act debit (withdraw,
// transfer-out) MUST use debit_if_sufficient() instead -- calling
// view_balance() then update_balance() separately re-opens the race
// window this file is meant to close.
int update_balance(int custID, double amount, int isDeposit) {
    pthread_mutex_lock(&accounts_lock);

    FILE *fp = fopen("data/accounts.dat", "r");
    FILE *temp = fopen("data/tmp_accounts.dat", "w");
    if (!fp || !temp) {
        if (fp) fclose(fp);
        if (temp) fclose(temp);
        pthread_mutex_unlock(&accounts_lock);
        return -1;
    }

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
    rename("data/tmp_accounts.dat", "data/accounts.dat");

    pthread_mutex_unlock(&accounts_lock);
    return found ? 0 : -1;
}

// -------------------- ATOMIC CHECK-THEN-DEBIT --------------------
// Reads the balance and, if sufficient, deducts amount -- all while
// holding accounts_lock, so no other thread can observe or modify
// this account's balance in between the check and the deduction.
// Returns: 0 = success (new_balance set), -1 = insufficient funds,
// -2 = account not found.
int debit_if_sufficient(int custID, double amount, double *new_balance) {
    pthread_mutex_lock(&accounts_lock);

    FILE *fp = fopen("data/accounts.dat", "r");
    FILE *temp = fopen("data/tmp_accounts.dat", "w");
    if (!fp || !temp) {
        if (fp) fclose(fp);
        if (temp) fclose(temp);
        pthread_mutex_unlock(&accounts_lock);
        return -2;
    }

    int id;
    double bal;
    int found = 0;
    int result = -2;

    while (fscanf(fp, "%d:%lf\n", &id, &bal) == 2) {
        if (id == custID) {
            found = 1;
            if (amount > bal) {
                result = -1;  // insufficient funds -- leave balance unchanged
            } else {
                bal -= amount;
                result = 0;
                if (new_balance) *new_balance = bal;
            }
        }
        fprintf(temp, "%d:%.2f\n", id, bal);
    }

    fclose(fp);
    fclose(temp);
    rename("data/tmp_accounts.dat", "data/accounts.dat");

    pthread_mutex_unlock(&accounts_lock);
    return found ? result : -2;
}

// -------------------- RECORD TRANSACTION --------------------
int record_transaction(int custID, const char *type, double amount) {
    pthread_mutex_lock(&transactions_lock);

    FILE *fp = fopen("data/transactions.dat", "a");
    if (!fp) {
        pthread_mutex_unlock(&transactions_lock);
        return -1;
    }

    time_t now = time(NULL);
    char *t = ctime(&now);
    t[strcspn(t, "\n")] = 0; // remove newline

    fprintf(fp, "%d:%s:%.2f:%s\n", custID, type, amount, t);
    fclose(fp);
    pthread_mutex_unlock(&transactions_lock);
    return 0;
}

// -------------------- TRANSACTION HISTORY (Passbook Style) --------------------
int view_transaction_history(int custID, char *buffer, size_t size) {
    pthread_mutex_lock(&transactions_lock);

    FILE *fp = fopen("data/transactions.dat", "r");
    if (!fp) {
        snprintf(buffer, size, "No transactions found.\n");
        pthread_mutex_unlock(&transactions_lock);
        return -1;
    }

    int id;
    char type[32], ts[64];
    double amt;
    size_t len = 0;
    int found = 0;

    len += snprintf(buffer + len, size - len,
        "---------------------------------------------------------------\n"
        "| Date & Time           | Type         | Amount (Rs.)   |\n"
        "---------------------------------------------------------------\n");

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
        pthread_mutex_unlock(&transactions_lock);
        return 0;
    }

    len += snprintf(buffer + len, size - len,
        "---------------------------------------------------------------\n");

    pthread_mutex_unlock(&transactions_lock);
    return 0;
}

// -------------------- LOANS --------------------
int apply_loan(int custID, double amount) {
    pthread_mutex_lock(&loans_lock);
    FILE *fp = fopen("data/loans.dat", "a");
    if (!fp) {
        pthread_mutex_unlock(&loans_lock);
        return -1;
    }
    int assignedEmp = 0;
    fprintf(fp, "%d:%.2f:pending:%d\n", custID, amount, assignedEmp);
    fclose(fp);
    pthread_mutex_unlock(&loans_lock);
    return 0;
}

int view_loans(int empID, char *buffer, size_t size) {
    pthread_mutex_lock(&loans_lock);
    FILE *fp = fopen("data/loans.dat", "r");
    if (!fp) {
        snprintf(buffer, size, "No loan records.\n");
        pthread_mutex_unlock(&loans_lock);
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
    pthread_mutex_unlock(&loans_lock);
    return 0;
}

int update_loan_status(int custID, const char *status) {
    pthread_mutex_lock(&loans_lock);
    FILE *fp = fopen("data/loans.dat", "r");
    FILE *tmp = fopen("data/tmp_loans.dat", "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        pthread_mutex_unlock(&loans_lock);
        return -1;
    }

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
    rename("data/tmp_loans.dat", "data/loans.dat");
    pthread_mutex_unlock(&loans_lock);
    return updated ? 0 : -1;
}

// -------------------- PASSWORD --------------------
int change_password(int userID, const char *newpwd) {
    pthread_mutex_lock(&users_lock);
    FILE *fp = fopen("data/users.dat", "r");
    FILE *tmp = fopen("data/tmp_users.dat", "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        pthread_mutex_unlock(&users_lock);
        return -1;
    }

    char fid[32], fpwd[64], frole[32], fstatus[32];
    char idbuf[16];
    sprintf(idbuf, "%d", userID);
    int updated = 0;

    while (fscanf(fp, "%31[^:]:%63[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(fid, idbuf) == 0) {
            fprintf(tmp, "%s:%s:%s:%s\n", fid, newpwd, frole, fstatus);
            updated = 1;
        } else {
            fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, frole, fstatus);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename("data/tmp_users.dat", "data/users.dat");
    pthread_mutex_unlock(&users_lock);
    return updated ? 0 : -1;
}


// -------------------- FEEDBACK --------------------
int add_feedback(int custID, const char *feedback) {
    pthread_mutex_lock(&feedback_lock);
    FILE *fp = fopen("data/feedback.dat", "a");
    if (!fp) {
        pthread_mutex_unlock(&feedback_lock);
        return -1;
    }
    fprintf(fp, "%d:%s\n", custID, feedback);
    fclose(fp);
    pthread_mutex_unlock(&feedback_lock);
    return 0;
}

int view_feedbacks(char *buffer, size_t size) {
    pthread_mutex_lock(&feedback_lock);
    FILE *fp = fopen("data/feedback.dat", "r");
    if (!fp) {
        snprintf(buffer, size, "No feedback records.\n");
        pthread_mutex_unlock(&feedback_lock);
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
    pthread_mutex_unlock(&feedback_lock);
    return 0;
}

// -------------------- ADD NEW CUSTOMER --------------------
int add_new_customer(const char *password) {
    pthread_mutex_lock(&users_lock);

    int lastID = 1000;
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

    FILE *fp = fopen("data/users.dat", "a");
    if (!fp) {
        pthread_mutex_unlock(&users_lock);
        return -1;
    }
    fprintf(fp, "%d:%s:Customer:active\n", newID, password);
    fclose(fp);
    pthread_mutex_unlock(&users_lock);

    // Separate lock/file -- ID allocation above is already safe, so
    // this doesn't need to be inside the users_lock critical section.
    pthread_mutex_lock(&accounts_lock);
    FILE *acc_fp = fopen("data/accounts.dat", "a");
    if (acc_fp) {
        fprintf(acc_fp, "%d:0.00\n", newID);
        fclose(acc_fp);
    }
    pthread_mutex_unlock(&accounts_lock);

    return newID;
}


// -------------------- MODIFY CUSTOMER PASSWORD --------------------
int modify_customer_password(int custID, const char *newpwd) {
    pthread_mutex_lock(&users_lock);
    FILE *fp = fopen("data/users.dat", "r");
    FILE *tmp = fopen("data/tmp_users.dat", "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        pthread_mutex_unlock(&users_lock);
        return -1;
    }

    char fid[32], fpwd[64], frole[32], fstatus[32];
    char idbuf[16];
    sprintf(idbuf, "%d", custID);
    int modified = 0;

    while (fscanf(fp, "%31[^:]:%63[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(fid, idbuf) == 0 && strcmp(frole, "Customer") == 0) {
            fprintf(tmp, "%s:%s:%s:%s\n", fid, newpwd, frole, fstatus);
            modified = 1;
        } else {
            fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, frole, fstatus);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename("data/tmp_users.dat", "data/users.dat");
    pthread_mutex_unlock(&users_lock);
    return modified ? 0 : -1;
}

// -------------------- MANAGER FUNCTIONS --------------------

int toggle_customer_status(int custID, const char *new_status) {
    pthread_mutex_lock(&users_lock);
    FILE *fp = fopen("data/users.dat", "r");
    FILE *tmp = fopen("data/tmp_users.dat", "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        pthread_mutex_unlock(&users_lock);
        return -1;
    }

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
    rename("data/tmp_users.dat", "data/users.dat");
    pthread_mutex_unlock(&users_lock);
    return updated ? 0 : -1;
}

int assign_loan_to_employee(int custID, int empID) {
    pthread_mutex_lock(&loans_lock);
    FILE *fp = fopen("data/loans.dat", "r");
    FILE *tmp = fopen("data/tmp_loans.dat", "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        pthread_mutex_unlock(&loans_lock);
        return -1;
    }

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
    rename("data/tmp_loans.dat", "data/loans.dat");
    pthread_mutex_unlock(&loans_lock);
    return assigned_flag ? 0 : -1;
}

// -------------------- VIEW ALL CUSTOMERS --------------------
int view_all_customers(char *buffer, size_t size) {
    // Touches both users.dat and accounts.dat -- lock order is always
    // users_lock then accounts_lock to match every other function
    // that could (in principle) need both, so we can never deadlock.
    pthread_mutex_lock(&users_lock);
    pthread_mutex_lock(&accounts_lock);

    FILE *fp_users = fopen("data/users.dat", "r");
    FILE *fp_acc = fopen("data/accounts.dat", "r");
    if (!fp_users || !fp_acc) {
        snprintf(buffer, size, "Unable to read customer records.\n");
        if (fp_users) fclose(fp_users);
        if (fp_acc) fclose(fp_acc);
        pthread_mutex_unlock(&accounts_lock);
        pthread_mutex_unlock(&users_lock);
        return -1;
    }

    int ids[1000];
    double balances[1000];
    int count = 0, id;
    double bal;
    while (count < 1000 && fscanf(fp_acc, "%d:%lf\n", &id, &bal) == 2) {
        ids[count] = id;
        balances[count++] = bal;
    }

    size_t len = 0;
    len += snprintf(buffer + len, size - len,
        "---------------------------------------------------------------\n"
        "| Customer ID | Balance (Rs.)   | Status     |\n"
        "---------------------------------------------------------------\n");

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
    pthread_mutex_unlock(&accounts_lock);
    pthread_mutex_unlock(&users_lock);
    return 0;
}

// -------------------- ADMIN FUNCTIONS --------------------

int add_new_employee(const char *password) {
    pthread_mutex_lock(&users_lock);

    FILE *fp = fopen("data/users.dat", "r");
    if (!fp) {
        pthread_mutex_unlock(&users_lock);
        return -1;
    }

    int lastID = 2000;
    char fid[32], fpwd[64], frole[32], fstatus[32];

    while (fscanf(fp, "%31[^:]:%63[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        int id = atoi(fid);
        if ((strcmp(frole, "Employee") == 0 || strcmp(frole, "Manager") == 0) && id > lastID) {
            lastID = id;
        }
    }
    fclose(fp);

    int newID = lastID + 1;

    fp = fopen("data/users.dat", "a");
    if (!fp) {
        pthread_mutex_unlock(&users_lock);
        return -1;
    }
    fprintf(fp, "%d:%s:Employee:active\n", newID, password);
    fclose(fp);

    pthread_mutex_unlock(&users_lock);
    return newID;
}

int modify_user_details(int userID, const char *newpwd) {
    pthread_mutex_lock(&users_lock);
    FILE *fp = fopen("data/users.dat", "r");
    FILE *tmp = fopen("data/tmp_users.dat", "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        pthread_mutex_unlock(&users_lock);
        return -1;
    }

    char fid[32], fpwd[64], frole[32], fstatus[32];
    char idbuf[16];
    sprintf(idbuf, "%d", userID);
    int updated = 0;

    while (fscanf(fp, "%31[^:]:%63[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(fid, idbuf) == 0) {
            fprintf(tmp, "%s:%s:%s:%s\n", fid, newpwd, frole, fstatus);
            updated = 1;
        } else {
            fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, frole, fstatus);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename("data/tmp_users.dat", "data/users.dat");
    pthread_mutex_unlock(&users_lock);
    return updated ? 0 : -1;
}

int change_user_role(int userID, const char *new_role) {
    pthread_mutex_lock(&users_lock);
    FILE *fp = fopen("data/users.dat", "r");
    FILE *tmp = fopen("data/tmp_users.dat", "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        pthread_mutex_unlock(&users_lock);
        return -1;
    }

    char fid[32], fpwd[64], frole[32], fstatus[32];
    char idbuf[16];
    sprintf(idbuf, "%d", userID);
    int updated = 0;

    while (fscanf(fp, "%31[^:]:%63[^:]:%31[^:]:%31s\n", fid, fpwd, frole, fstatus) == 4) {
        if (strcmp(fid, idbuf) == 0) {
            if ((strcmp(frole, "Employee") == 0 && strcmp(new_role, "Manager") == 0) ||
                (strcmp(frole, "Manager") == 0 && strcmp(new_role, "Employee") == 0)) {
                fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, new_role, fstatus);
                updated = 1;
            } else {
                fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, frole, fstatus);
            }
        } else {
            fprintf(tmp, "%s:%s:%s:%s\n", fid, fpwd, frole, fstatus);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename("data/tmp_users.dat", "data/users.dat");
    pthread_mutex_unlock(&users_lock);
    return updated ? 0 : -1;
}