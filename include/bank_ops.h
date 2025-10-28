#ifndef BANK_OPS_H
#define BANK_OPS_H

#include <stddef.h>

// -------------------- USER AUTH --------------------
int validate_user(const char *id, const char *pwd, char *role);

// -------------------- CUSTOMER OPS --------------------
int view_balance(int custID, double *balance);
int update_balance(int custID, double amount, int isDeposit);
int record_transaction(int custID, const char *type, double amount);
int view_transaction_history(int custID, char *buffer, size_t size);

// -------------------- LOAN OPS --------------------
int apply_loan(int custID, double amount);
int view_loans(int empID, char *buffer, size_t size);
int update_loan_status(int custID, const char *status);

// -------------------- PASSWORD OPS --------------------
int change_password(int userID, const char *newpwd);
int modify_customer_password(int custID, const char *newpwd);

// -------------------- FEEDBACK OPS --------------------
int add_feedback(int custID, const char *feedback);
int view_feedbacks(char *buffer, size_t size);

// -------------------- EMPLOYEE OPS --------------------
int add_new_customer(const char *password);

// -------------------- MANAGER OPS --------------------
int toggle_customer_status(int custID, const char *new_status);
int assign_loan_to_employee(int custID, int empID);
int view_all_customers(char *buffer, size_t size);

// --- Admin Functions ---
int add_new_employee(const char *password);
int modify_user_details(int userID, const char *newpwd);
int change_user_role(int userID, const char *new_role);

#endif
