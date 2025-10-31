🏦 Bank Management System (Socket Programming in C)

📘 Overview

This project implements a multi-user Bank Management System using Socket Programming in C with multi-threading, file-based persistence, and role-based access control.

The system allows Customers, Bank Employees, Managers, and Administrators to perform their respective banking operations concurrently, ensuring data consistency, session management, and synchronization.


---

🧩 Features by Role

👤 Customer

🔐 Login System (Single Session Enforcement)

💰 View Account Balance

💵 Deposit / Withdraw Money

🔁 Transfer Funds

🏦 Apply for Loan

✏ Change Password

🗒 Add Feedback

📜 View Transaction History

🚪 Logout / Exit



---

🧑‍💼 Bank Employee

🔐 Login System (Single Session Enforcement)

➕ Add New Customer

✏ Modify Customer Details (Change Password)

🏦 View Assigned Loan Applications

✅ Approve / Reject Loans

📒 View Customer Transactions (Passbook)

🔑 Change Password

🚪 Logout / Exit



---

👨‍💼 Manager

🔐 Login System (Single Session Enforcement)

⚙ Activate / Deactivate Customer Accounts

🧾 Assign Loan Applications to Employees

💬 Review Customer Feedback

👁 View All Customers

🔑 Change Password

🚪 Logout / Exit



---

🧑‍💻 Administrator

🔐 Login System (Single Session Enforcement)

🧍 Add New Bank Employee

📝 Modify Customer / Employee Details

🔄 Manage User Roles (Employee ↔ Manager)

🔑 Change Password

🚪 Logout / Exit



---

🧠 Technical Highlights

Feature	Description

Architecture	Client–Server using TCP Sockets
Concurrency	Multi-threaded server using pthread
Session Control	Only one active session per user
Synchronization	Mutex locks for session management
Persistence	File-based data storage (.dat files)
Data Integrity	Temporary file replacement for atomic writes
Error Handling	Graceful handling of disconnects and invalid inputs



---

💾 Data Files Structure

All data is stored in the data/ directory:

File	Description	Example

users.dat	Stores user credentials and roles	1001:password:Customer:active
accounts.dat	Customer account balances	1001:5000.00
transactions.dat	Transaction history	1001:Deposit:1000.00:Tue Oct 28 15:00:00 2025
loans.dat	Loan applications and status	1001:20000.00:pending:2001
feedback.dat	Customer feedback	1001:Very good service!



---

⚙ Setup & Compilation

🧱 1. Clone Repository

git clone https://github.com/<your-username>/bank-management-system.git
cd bank-management-system

🛠 2. Compile the Server and Client

gcc server/server.c src/bank_ops.c -o server -lpthread
gcc client/client.c -o client

🚀 3. Run the Server

./server

💻 4. Run the Client (in another terminal)

./client


---

🔁 Sample Login Credentials

Role	ID	Password	Status

Customer	1001	password	active
Employee	2001	password	active
Manager	3001	password	active
Administrator	4001	password	active
Customer (Inactive)	1002	password	inactive



---

🧮 Concurrency & Synchronization

Concept	Implementation

Thread-per-client	Each client handled by separate pthread
Mutex lock	Protects global session array to prevent duplicate logins
File updates	Temporary files used for atomic write (tmp → main)
Simultaneous Access	Multiple clients can perform transactions without data corruption



---

💡 ACID Property Compliance

Property	Implementation

Atomicity	Each update operation uses temporary files for rollback safety
Consistency	Validation checks before balance, loan, and role updates
Isolation	Independent threads and per-session synchronization
Durability	Data stored persistently in .dat files



---

🧩 Project Structure

```bash
bank-management-system/
├── client/
│   └── client.c
├── server/
│   └── server.c
├── src/
│   └── bank_ops.c
├── include/
│   └── bank_ops.h
├── data/
│   ├── users.dat
│   ├── accounts.dat
│   ├── transactions.dat
│   ├── loans.dat
│   └── feedback.dat
└── README.md
```

---


Criteria	Implementation

Blueprint / Class Diagram	Included in documentation (UML + architecture)
Working Code for Each Module	Fully implemented across client, server, and ops
Concurrency, Synchronization, ACID	Achieved via pthread, mutexes, and atomic file ops

<img width="1011" height="732" alt="image" src="https://github.com/user-attachments/assets/ebf060e6-cbf5-4e82-8af3-f0e036d0ad87" />


---

✨ Acknowledgements

Developed as part of the System Software Laboratory Project demonstrating:

Socket programming in C

Multi-threaded concurrent server

File-based transaction persistence

Role-based access control in distributed systems
