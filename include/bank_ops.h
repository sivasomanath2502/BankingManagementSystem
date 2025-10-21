#ifndef BANK_OPS_H
#define BANK_OPS_H

struct Account{
	int accNo;
	int CUstID;
	double balance;
};

int view_balance(int custID,double *balance);
int update_balance(int custID,double amount,int is_deposit);
int record_transaction(int custID,const char *type,double amount);
int view_transaction_history(int custID,char *outbuf,int bufsize);
int apply_loan(int custID,double amount);
int change_password(int custID,const char *newpwd);
int add_feedback(int custID,const char *message);

#endif
