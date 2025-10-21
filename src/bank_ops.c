#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include "../include/bank_ops.h"

int view_balance(int custID,double *balance){
	FILE *fp = fopen("data/accounts.dat","r");
	if(!fp) return -1;
	int acc,id;
	double bal;
	char line[100];
	while(fgets(line,sizeof(line),fp)){
		if(sscanf(line,"%d:%d:%lf",&acc,&id,&bal) == 3 && id == custID){
			*balance = bal;
			fclose(fp);
			return 0;;
		}
	}
	fclose(fp);
	return -1;
}

int update_balance(int custID, double amount, int is_deposit) {
    FILE *fp = fopen("data/accounts.dat", "r+");
    if (!fp) return -3;
    char tempFile[] = "data/tmp_accounts.dat";
    FILE *tfp = fopen(tempFile, "w");
    if (!tfp) { fclose(fp); return -3; }

    int acc, id, found = 0;
    double bal;
    char line[100];
    int status = -1;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%d:%d:%lf", &acc, &id, &bal) == 3) {
            if (id == custID) {
                found = 1;
                if (is_deposit) bal += amount;
                else if (bal >= amount) bal -= amount;
                else status = -2; 
                fprintf(tfp, "%d:%d:%.2f\n", acc, id, bal);
            } else {
                fputs(line, tfp);
            }
        }
    }

    fclose(fp);
    fclose(tfp);

    if (found) rename(tempFile, "data/accounts.dat");
    else remove(tempFile);

    return (status == -2) ? -2 : (found ? 0 : -1);
}

int record_transaction(int custID, const char *type, double amount) {
    FILE *fp = fopen("data/transactions.dat", "a");
    if (!fp) return -1;
    fprintf(fp, "Customer %d - %s %.2f\n", custID, type, amount);
    fclose(fp);
    return 0;
}

int view_transaction_history(int custID, char *outbuf, int bufsize) {
    FILE *fp = fopen("data/transactions.dat", "r");
    if (!fp) return -1;
    char line[200];
    int len = 0;
    outbuf[0] = '\0';
    while (fgets(line, sizeof(line), fp)) {
        int id;
        if (sscanf(line, "Customer %d -", &id) == 1 && id == custID) {
            if (len + strlen(line) < bufsize - 1) {
                strcat(outbuf, line);
                len += strlen(line);
            }
        }
    }
    fclose(fp);
    return len ? 0 : -1;
}

int apply_loan(int custID, double amount) {
    FILE *fp = fopen("data/loans.dat", "a");
    if (!fp) return -1;
    fprintf(fp, "Customer %d applied for loan %.2f\n", custID, amount);
    fclose(fp);
    return 0;
}

int change_password(int custID, const char *newpwd) {
    FILE *fp = fopen("data/users.dat", "r+");
    if (!fp) return -1;
    char tempFile[] = "data/tmp_users.dat";
    FILE *tfp = fopen(tempFile, "w");
    if (!tfp) { fclose(fp); return -1; }

    char id[32], pwd[32], role[32], line[100];
    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%[^:]:%[^:]:%s", id, pwd, role);
        if (atoi(id) == custID)
            fprintf(tfp, "%s:%s:%s\n", id, newpwd, role);
        else
            fputs(line, tfp);
    }

    fclose(fp);
    fclose(tfp);
    rename(tempFile, "data/users.dat");
    return 0;
}

int add_feedback(int custID, const char *message) {
    FILE *fp = fopen("data/feedback.dat", "a");
    if (!fp) return -1;
    fprintf(fp, "Customer %d: %s\n", custID, message);
    fclose(fp);
    return 0;
}
