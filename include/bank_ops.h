#ifndef BANK_OPS_H
#define BANK_OPS_H

#include <stddef.h>

// ---- Authentication ----
int validate_user(const char *id, const char *pwd, char *role);

// ---- Account Operations ----
int view_balance(int custID, double *balance);
int update_balance(int custID, double amount, int isDeposit);
int record_transaction(int custID, const char *type, double amount);
int view_transaction_history(int custID, char *buffer, size_t size);

// ---- Loan Operations ----
int apply_loan(int custID, double amount);
int view_loans(int empID, char *buffer, size_t size);
int update_loan_status(int custID, const char *status);

// ---- Password & Feedback ----
int change_password(int userID, const char *newpwd);
int add_feedback(int custID, const char *feedback);
int view_feedbacks(char *buffer, size_t size);

// ---- Customer Management ----
int add_new_customer(const char *password);
int modify_customer_password(int custID, const char *newpwd);


#endif

