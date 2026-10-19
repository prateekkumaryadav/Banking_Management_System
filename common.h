#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // For system calls (read, write, open, close, lseek)
#include <sys/types.h>  // For system data types
#include <sys/socket.h> // For socket programming
#include <netinet/in.h> // For internet addresses (sockaddr_in)
#include <arpa/inet.h>  // For inet_pton
#include <fcntl.h>      // For file control (open, fcntl)
#include <pthread.h>    // For threading
#include <errno.h>      // For error numbers (errno)
#include <time.h>       // For transaction timestamps

// --- Configuration ---
#define SERVER_PORT 8080 // default PORT no. 
#define MAX_CLIENTS 10 // by default max no. of users can connect (security)
#define MAX_USERNAME_LEN 50 // username length limit
#define MAX_PASSWORD_LEN 50 // password length limit
#define MAX_NAME_LEN 100
#define MAX_MSG_LEN 256 // for feedback

// --- File Paths ---
#define USER_FILE "data/users.dat"
#define ACCOUNT_FILE "data/accounts.dat"
#define TRANSACTION_FILE "data/transactions.dat"
#define LOAN_FILE "data/loans.dat"
#define FEEDBACK_FILE "data/feedback.dat"

// --- Data Structures ---
typedef enum {
    CUSTOMER = 1,
    EMPLOYEE,
    MANAGER,
    ADMIN
} UserRole;

// Stored in users.dat
typedef struct {
    int userID; // Unique ID (e.g., 1001, 1002). This will be the "Primary Key".
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    UserRole role;
    int isActive; // 1 for active, 0 for deactivated
    int sessionActive; // 0 = Not logged in, 1 = Logged in
} User;

// Stored in feedback.dat
typedef struct {
    int feedbackID;
    int customerID;
    char message[MAX_MSG_LEN];
    long timestamp;
    
} Feedback;

// Stored in accounts.dat
// For simplicity, we'll assume accountID == customer's userID
typedef struct {
    int accountID; // This will match the customer's userID
    double balance;
} Account;

// Stored in transactions.dat
typedef struct {
    int transactionID;
    int accountID;
    char type[20]; // "DEPOSIT", "WITHDRAW", "TRANSFER"
    double amount;
    double newBalance;
    long timestamp;
} Transaction;

// Stored in loans.dat
typedef struct {
    int loanID;
    int customerID; // The user ID of the applicant
    double amount;
    long timestamp;
    char status[20]; // "PENDING", "APPROVED", "REJECTED"
} Loan;

// --- Client-Server Communication Protocol ---

// This struct is used for all client-to-server requests 
// [Client-->Server] REQUEST

typedef struct {
    char command[20]; // e.g., "LOGIN", "DEPOSIT", "GET_BALANCE"
    
    // Auth info (for login)
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];

    // Data payload
    int userID; // The user ID for context
    double amount;
    int targetAccountID; // For fund transfers
    UserRole newUserRole; // For Admin to add new users
    char feedbackMessage[MAX_MSG_LEN]; // For adding feedback
} ClientRequest;


// This struct is used for all server-to-client responses 
// [Server-->Client] RESPONSE

typedef struct {
    int success; // 1 for success, 0 for failure
    char message[MAX_MSG_LEN];
    // Data payload
    UserRole role; // Sent on successful login
    int userID;
    double balance;
} ServerResponse;


// --- Function Signatures for Locking (Used by Server) ---

// This is the most important part for concurrency
// by acquiring lock other user cannot access at that time

int lock_record(int fd, int record_id, int lock_type, size_t record_size);
int unlock_record(int fd, int record_id, size_t record_size);

#endif // COMMON_H