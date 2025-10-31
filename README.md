# Banking Management System

A **Client–Server-based Banking Management System** implemented in **C** using **TCP socket programming**.  
It supports multiple roles — **Admin**, **Employee**, and **Customer** — and enables secure, concurrent banking operations between clients and a centralized server.

---

## Features

### Client Side
- User authentication and role-based access control
- Menu-driven interface for each user role
- Safe socket communication using `safe_read()` and `safe_write()`
- Handles network and connection errors gracefully

### Server Side
- Concurrent client handling using `fork()` (or threads)
- File-based data persistence (`users.dat`, `accounts.dat`)
- Record-level locking (`fcntl`) for transaction safety
- CRUD operations on users and accounts
- Supports Admin, Employee, and Customer functionalities

---

## system Architecture

```text
+------------------+       TCP/IP        +------------------+
|   Client (App)   | <---------------->  |   Server (Core)  |
|  Admin/Employee/ |                    |  Data & Logic Mgmt|
|  Customer        |                    |                  |
+------------------+                    +------------------+
Project Structure
bash
Copy code
Banking-Management-System/
├── Makefile
├── include/
│   ├── bank_ops.h
├── server/
│   ├── server.c
├── client/
│   ├── client.c
├── data/
│   ├── users.dat
│   └── accounts.dat
|   ├── loans.dat
│   └── feedback.dat
│   └── transactions.dat
├── src/
│   ├── bank_ops.c
└── README.md
Requirements
Operating System: Linux / Ubuntu

Compiler: GCC

Libraries Used:

<pthread.h>

<sys/socket.h>

<arpa/inet.h>

<fcntl.h>

<unistd.h>

Compilation & Execution (Using Makefile)
Build the project
make all
This will compile both the server and client executables automatically.
Run the server
bash
make run-server
Run the client
bash
Copy code
make run-client
Clean build files
bash
Copy code
make clean
Roles and Operations
 Admin
Add, modify, or delete employees/customers

Manage user roles and credentials

Reset passwords

Employee
Manage customer accounts

Approve or update customer details

Perform transactions (deposit/withdrawal)

Customer
View account balance

Deposit or withdraw funds

Change password

View transaction history

Security & Reliability
Authentication via User ID and Password

File-level record locking using fcntl()

Role-based authorization system

Safe and reliable I/O operations

Concepts Implemented
TCP/IP Socket Programming

Process concurrency using fork()

File handling & record locking

Modular programming with header files

Makefile-based project automation

Sample Output
markdown
Connected to server.
User ID: 4001
Password: ********
Login successful. Role: Admin

--- Admin Menu ---
1. Add New Bank Employee
2. Modify Customer/Employee Details
3. Manage User Roles
4. Change Password
5. Logout
6. Exit
Enter choice:
