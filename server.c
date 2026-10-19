#include "common.h"

// --- Global File Descriptors ---
// We open files once and share FDs across threads
int user_fd;
int account_fd;
int transaction_fd;
int loan_fd;
int feedback_fd;

// -------------------------------------------------------

// --- Function Prototypes for Handlers ---
void* handle_client(void* arg);

void handle_login(int client_socket, ClientRequest *req, ServerResponse *res);

// --- Client/Customer Requests
void handle_get_balance(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_deposit(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_withdraw(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_transfer(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_apply_loan(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_view_transactions(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_add_feedback(int client_socket, ClientRequest *req, ServerResponse *res);

// --- Employee
void handle_add_customer(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_modify_customer(int client_socket, ClientRequest *req, ServerResponse *res);

// --- Manager
void handle_toggle_account_status(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_list_pending_loans(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_process_loan(int client_socket, ClientRequest *req, ServerResponse *res);
void handle_review_feedback(int client_socket, ClientRequest *req, ServerResponse *res);

// --- Admin
void handle_add_user(int client_socket, ClientRequest *req, ServerResponse *res);
int get_next_user_id();
void handle_admin_modify_user(int client_socket, ClientRequest *req, ServerResponse *res);

// -------------------------------------------------------

// --- Helper Functions ---
// int get_account_id_from_user_id(int userID);
int get_next_transaction_id();
void log_transaction(int accountID, const char* type, double amount, double newBalance);
int get_next_loan_id();
int get_next_feedback_id();

// -------------------------------------------------------

// --- The Main Server Loop ---
int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    // 1. Open Data Files Globally using system calls: open()
    // O_RDWR: Read/Write
    // O_CREAT: Create if doesn't exist
    // 0666: Read/Write permissions for all users
    user_fd = open(USER_FILE, O_RDWR | O_CREAT, 0666);
    account_fd = open(ACCOUNT_FILE, O_RDWR | O_CREAT, 0666);
    transaction_fd = open(TRANSACTION_FILE, O_RDWR | O_CREAT | O_APPEND, 0666);
    loan_fd = open(LOAN_FILE, O_RDWR | O_CREAT, 0666);
    feedback_fd = open(FEEDBACK_FILE, O_RDWR | O_CREAT, 0666);
    
    if (user_fd == -1 || account_fd == -1 || transaction_fd == -1 || loan_fd == -1 || feedback_fd == -1) {
        perror("Error opening data files");
        exit(EXIT_FAILURE);
    }

    // 2. Setup Server Socket ---
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Allowing address reuse
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Socket listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    // 3. Accept Connections Loop ---
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Socket accept failed");
            continue;
        }

        printf("New connection accepted...\n");

        // 4. Create a New Thread for Each Client ---
        int *p_client_socket = malloc(sizeof(int));
        *p_client_socket = client_socket;

        if (pthread_create(&thread_id, NULL, handle_client, p_client_socket) != 0) {
            perror("Failed to create thread");
            close(client_socket);
            free(p_client_socket);
        }
        
        // Detaching the thread so we don't need to join() it
        pthread_detach(thread_id);
    }

    close(user_fd);
    close(account_fd);
    close(transaction_fd);
    close(loan_fd);
    close(server_socket);
    close(feedback_fd);

    return 0;
}

// --- Client Handler Thread ---
void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg); // Free the malloc'd socket descriptor

    ClientRequest req;
    ServerResponse res;

    // We need to store the state of the logged-in user for this thread
    int logged_in_userID = -1;
    UserRole logged_in_role = 0;

    // Read requests from the client
    ssize_t read_bytes;
    while ((read_bytes = read(client_socket, &req, sizeof(ClientRequest))) > 0) {
        memset(&res, 0, sizeof(ServerResponse)); // Clear response struct for safety

        // 1. Login Logic ---
        if (strcmp(req.command, "LOGIN") == 0) {
            handle_login(client_socket, &req, &res);
            if (res.success) {
                // Store session state
                logged_in_userID = res.userID;
                logged_in_role = res.role;
            }
        } 
        
        // 2. Logged-In User Logic ---
        else if (logged_in_userID != -1) {
            // Ensure the request's userID matches the logged-in user
            // This is a basic security check
            if (req.userID != logged_in_userID) {
                 res.success = 0;
                 strcpy(res.message, "Error: Request UserID mismatch.");
            }

            // --- Customer Actions ---
            else if (logged_in_role == CUSTOMER) {
                if (strcmp(req.command, "GET_BALANCE") == 0) {
                    handle_get_balance(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "DEPOSIT") == 0) {
                    handle_deposit(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "WITHDRAW") == 0) {
                    handle_withdraw(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "TRANSFER") == 0) {
                    handle_transfer(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "APPLY_LOAN") == 0) {
                    handle_apply_loan(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "GET_TRANSACTIONS") == 0) {
                    handle_view_transactions(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "ADD_FEEDBACK") == 0) {
                    handle_add_feedback(client_socket, &req, &res);
                }
                else {
                    res.success = 0;
                    strcpy(res.message, "Error: Unknown command for Customer.");
                }
            }

            // --- Employee Actions ---
            else if (logged_in_role == EMPLOYEE) {
                if (strcmp(req.command, "ADD_CUSTOMER") == 0) {
                    handle_add_customer(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "MODIFY_CUSTOMER") == 0) {
                    handle_modify_customer(client_socket, &req, &res);
                }
                else {
                    res.success = 0;
                    strcpy(res.message, "Error: Unknown command for Employee.");
                }
            }

            // --- Manager Actions ---
            else if (logged_in_role == MANAGER) {
                if (strcmp(req.command, "TOGGLE_STATUS") == 0) {
                    handle_toggle_account_status(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "LIST_PENDING_LOANS") == 0) {
                    handle_list_pending_loans(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "PROCESS_LOAN") == 0) {
                    handle_process_loan(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "REVIEW_FEEDBACK") == 0) {
                    handle_review_feedback(client_socket, &req, &res);
                }
                else {
                    res.success = 0;
                    strcpy(res.message, "Error: Unknown command for Manager.");
                }
            }

            // --- Admin Actions ---
            else if (logged_in_role == ADMIN) {
                if (strcmp(req.command, "ADD_USER") == 0) {
                    handle_add_user(client_socket, &req, &res);
                }
                else if (strcmp(req.command, "ADMIN_MOD_USER") == 0) {
                    handle_admin_modify_user(client_socket, &req, &res);
                }
                else {
                    res.success = 0;
                    strcpy(res.message, "Error: Unknown command for Admin.");
                }
            }

            // --- Manager or other roles ---
            else { // This now catches unassigned roles
                res.success = 0;
                strcpy(res.message, "Error: Role features not implemented.");
            }
        }
        
        // 3. Not Logged-In ---
        else {
            res.success = 0;
            strcpy(res.message, "Error: You must be logged in to perform this action.");
        }

        // 4. Send Response Back to Client ---
        if (write(client_socket, &res, sizeof(ServerResponse)) <= 0) {
            perror("Write to client failed");
            break; // Client disconnected
        }
    }

    // Session clean up on disconnect ---
    // If a user was logged in, we must clear their session flag. helps us in maintaining concurrency/security
    if (logged_in_userID != -1) {
        printf("User %d disconnecting, clearing session flag.\n", logged_in_userID);
        
        // Locking the specific user record for working to clear his sessionActive
        int record_index = lock_record(user_fd, logged_in_userID, F_WRLCK, sizeof(User));
        
        if (record_index != -1) {
            User user;
            lseek(user_fd, record_index * sizeof(User), SEEK_SET);
            if (read(user_fd, &user, sizeof(User)) == sizeof(User)) {
                user.sessionActive = 0; // Set session to inactive
                
                // Seek back and write the change
                lseek(user_fd, record_index * sizeof(User), SEEK_SET);
                write(user_fd, &user, sizeof(User));
            } else {
                perror("Error reading user record for session cleanup");
            }
            // When exiting Unlock the record
            unlock_record(user_fd, logged_in_userID, sizeof(User));
        } else {
            perror("Error: Could not find user record for session cleanup");
        }
    }

    printf("Client disconnected.\n");
    close(client_socket);
    return NULL;
}

// This is the most important function using fcntl() to lock a "specific record" in a file.
int lock_record(int fd, int record_id, int lock_type, size_t record_size) {
    // Assuming record_id starts from 1.
    // If our IDs are not sequential (e.g., 1001, 1002),
    // we must find the "record index" (0, 1, 2...) first.
    // A better way is: "record_index = record_id - 1001;" if IDs start at 1001.
    
    // For simplicity, we are just using the ID as the index.
    // THIS IS A SIMPLIFICATION, but a real system needs an index file or to search.
    // now search it
    
    lseek(fd, 0, SEEK_SET); // Rewind file
    int current_id;
    int record_index = -1;
    char buffer[record_size];

    int index_counter = 0;
    while(read(fd, buffer, record_size) == record_size) {
        // We peek at the first int in the struct, which is the ID
        current_id = *(int*)buffer; 
        if (current_id == record_id) {
            record_index = index_counter;
            break;
        }
        index_counter++;
    }

    if (record_index == -1) { // Record not found
        return -1;
    }

    // Now, applying the locks ---
    struct flock lock;
    // F_WRLCK (Write) or F_RDLCK (Read)
    lock.l_type = lock_type;
    
    // Lock from the beginning of the file
    lock.l_whence = SEEK_SET;    
    
    // Byte offset of the record
    lock.l_start = record_index * record_size; 
    
    // Lock only this record's bytes
    lock.l_len = record_size;    
    
    // Process ID (not really used here)
    lock.l_pid = getpid();       

    // F_SETLKW: Set lock and "waits" if it's held by another process.
    // This is what which makes our thread "blocked" until the resource is/gets freed.
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("fcntl lock failed");
        return -1;
    }
    
    return record_index; // Return the index for later use
}

int unlock_record(int fd, int record_id, size_t record_size) {
    // Same logic to find the record index
    lseek(fd, 0, SEEK_SET);
    int current_id;
    int record_index = -1;
    char buffer[record_size];
    int index_counter = 0;
    while(read(fd, buffer, record_size) == record_size) {
        current_id = *(int*)buffer;
        if (current_id == record_id) {
            record_index = index_counter;
            break;
        }
        index_counter++;
    }

    if (record_index == -1) return -1; // Should not happen if locked

    struct flock lock;
    lock.l_type = F_UNLCK; // Unlock
    lock.l_whence = SEEK_SET;
    lock.l_start = record_index * record_size;
    lock.l_len = record_size;
    lock.l_pid = getpid();

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("fcntl unlock failed");
        return -1;
    }
    return 0;
}

// --- Handler Implementations ---
void handle_login(int client_socket, ClientRequest *req, ServerResponse *res) {
    User user;
    int found = 0;

    // --- Lock ---
    // We MUST use a WRITE LOCK now, because we need to modify
    // the user.sessionActive flag. This serializes all logins,
    // but makes the check-and-set operation atomic.
    lock_record(user_fd, 0, F_WRLCK, 0); // Lock the whole file

    lseek(user_fd, 0, SEEK_SET); // Rewind to start
    
    while (read(user_fd, &user, sizeof(User)) == sizeof(User)) {
        if (strcmp(user.username, req->username) == 0 &&
            strcmp(user.password, req->password) == 0) {
            
            if (!user.isActive) {
                strcpy(res->message, "Login Failed: Account is deactivated.");
                res->success = 0;
            } 
            else if (user.sessionActive) {  // NEW SESSION CHECK
                strcpy(res->message, "Login Failed: User already logged in.");
                res->success = 0;
            }
            else { // Success! Set session flag and write it back
                strcpy(res->message, "Login Successful!");
                res->success = 1;
                res->role = user.role;
                res->userID = user.userID;
                
                user.sessionActive = 1; // Set flag
                
                // Seek back to the start of this record
                lseek(user_fd, -sizeof(User), SEEK_CUR); 
                // Write the updated record
                write(user_fd, &user, sizeof(User)); 
            }
            found = 1;
            break;
        }
    }

    if (!found) {
        strcpy(res->message, "Login Failed: Invalid credentials.");
        res->success = 0;
    }
    
    // Unlock ---
    unlock_record(user_fd, 0, 0); // Unlock whole file
    // Response of this is sent by the main handler_client loop
}

void handle_get_balance(int client_socket, ClientRequest *req, ServerResponse *res) {
    Account acc;    
    // We are keeping accountID == userID for clients
    int accountID = req->userID; 

    // Lock Record (Read Lock) ---
    int record_index = lock_record(account_fd, accountID, F_RDLCK, sizeof(Account));
    if (record_index == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Account not found.");
        unlock_record(account_fd, accountID, sizeof(Account)); // Unlock just in case
        return;
    }

    // Seek to the record
    lseek(account_fd, record_index * sizeof(Account), SEEK_SET);
    
    if (read(account_fd, &acc, sizeof(Account)) == sizeof(Account)) {
        res->success = 1;
        res->balance = acc.balance;
        sprintf(res->message, "Your balance is: %.2f", acc.balance);
    } else {
        res->success = 0;
        strcpy(res->message, "Error: Could not read account data.");
    }
    
    // Unlock Record ---
    unlock_record(account_fd, accountID, sizeof(Account));
}

void handle_deposit(int client_socket, ClientRequest *req, ServerResponse *res) {
    if (req->amount <= 0) {
        res->success = 0;
        strcpy(res->message, "Error: Deposit amount must be positive.");
        return;
    }

    Account acc;
    int accountID = req->userID; // Assume accountID == userID
    double old_balance;

    // Lock Record (WRITE Lock) ---
    // This is the critical part, as any other thread (e.g., withdraw) trying to lock this *same record* will WAIT.
    int record_index = lock_record(account_fd, accountID, F_WRLCK, sizeof(Account));
    if (record_index == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Account not found.");
        unlock_record(account_fd, accountID, sizeof(Account));
        return;
    }

    // 1. Read ---
    lseek(account_fd, record_index * sizeof(Account), SEEK_SET);
    if (read(account_fd, &acc, sizeof(Account)) != sizeof(Account)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to read account for deposit.");
        unlock_record(account_fd, accountID, sizeof(Account));
        return;
    }

    // 2. Modify ---
    old_balance = acc.balance;
    acc.balance += req->amount;

    // 3. Write Back ---
    lseek(account_fd, record_index * sizeof(Account), SEEK_SET); // Seek back
    if (write(account_fd, &acc, sizeof(Account)) != sizeof(Account)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to write updated balance.");
        // We are in a bad state here, but we must unlock
    } else {
        res->success = 1;
        res->balance = acc.balance;
        sprintf(res->message, "Deposit successful. New balance: %.2f", acc.balance);
        
        // 4. Log Transaction ---
        // This should also be locked, but appending is "mostly" safe.
        // A proper implementation would lock the transaction file.
        log_transaction(accountID, "DEPOSIT", req->amount, acc.balance);
    }

    // 4. Unlock Record ---
    unlock_record(account_fd, accountID, sizeof(Account));
}

void log_transaction(int accountID, const char* type, double amount, double newBalance) {
    // This function just appends, but for a real system, you'd lock transaction_fd
    
    Transaction trans;
    trans.transactionID = get_next_transaction_id(); // Needs to be atomic
    trans.accountID = accountID;
    strcpy(trans.type, type);
    trans.amount = amount;
    trans.newBalance = newBalance;
    trans.timestamp = time(NULL);

    // write() to an append-only file is atomic
    write(transaction_fd, &trans, sizeof(Transaction));
}

int get_next_transaction_id() {
    // This finds the last ID and adds 1.
    // For simplicity, we just read the last record, this should be protected by a mutex or file lock.
    Transaction trans;
    long file_size = lseek(transaction_fd, 0, SEEK_END);
    if (file_size == 0) return 1;

    lseek(transaction_fd, -sizeof(Transaction), SEEK_END);
    read(transaction_fd, &trans, sizeof(Transaction));
    return trans.transactionID + 1;
}

void handle_transfer(int client_socket, ClientRequest *req, ServerResponse *res) {
    // 1. Validation ---
    if (req->amount <= 0) {
        res->success = 0;
        strcpy(res->message, "Error: Transfer amount must be positive.");
        return;
    }
    if (req->userID == req->targetAccountID) {
        res->success = 0;
        strcpy(res->message, "Error: Cannot transfer funds to your own account.");
        return;
    }

    int senderID = req->userID;
    int receiverID = req->targetAccountID;
    
    Account sender_acc, receiver_acc;
    int sender_idx, receiver_idx;

    // 2.DEADLOCK AVOIDANCE---
    // To prevent deadlock (Thread 1 locks A, waits for B; Thread 2 locks B, waits for A)
    // we "always" lock accounts in a consistent order, by their ID, locking the account with the lower ID first.
    
    int id1 = (senderID < receiverID) ? senderID : receiverID;
    int id2 = (senderID > receiverID) ? senderID : receiverID;

    // Lock first account (lowest ID)
    int record_index1 = lock_record(account_fd, id1, F_WRLCK, sizeof(Account));
    if (record_index1 == -1) {
        res->success = 0;
        strcpy(res->message, "Error: One of the accounts does not exist.");
        // No lock was acquired, so just return.
        return;
    }

    // Lock second account (highest ID)
    int record_index2 = lock_record(account_fd, id2, F_WRLCK, sizeof(Account));
    if (record_index2 == -1) {
        res->success = 0;
        strcpy(res->message, "Error: One of the accounts does not exist.");
        // *Crucial*: We must release the first lock!
        unlock_record(account_fd, id1, sizeof(Account));
        return;
    }

    // 3. At this point, we have successfully locked BOTH records ---
    // Now, figure out which index belongs to sender/receiver
    sender_idx = (senderID == id1) ? record_index1 : record_index2;
    receiver_idx = (receiverID == id1) ? record_index1 : record_index2;

    // 4. Read both account records ---
    lseek(account_fd, sender_idx * sizeof(Account), SEEK_SET);
    if (read(account_fd, &sender_acc, sizeof(Account)) != sizeof(Account)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to read sender account.");
        // Unlock both and return
        unlock_record(account_fd, id1, sizeof(Account));
        unlock_record(account_fd, id2, sizeof(Account));
        return;
    }
    
    lseek(account_fd, receiver_idx * sizeof(Account), SEEK_SET);
    if (read(account_fd, &receiver_acc, sizeof(Account)) != sizeof(Account)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to read receiver account.");
        // Unlock both and return
        unlock_record(account_fd, id1, sizeof(Account));
        unlock_record(account_fd, id2, sizeof(Account));
        return;
    }

    // 5. Perform Business Logic (Check funds) ---
    if (sender_acc.balance < req->amount) {
        res->success = 0;
        strcpy(res->message, "Error: Insufficient funds.");
        // Unlock both and return
        unlock_record(account_fd, id1, sizeof(Account));
        unlock_record(account_fd, id2, sizeof(Account));
        return;
    }

    // 6. Perform Atomic Update ---
    sender_acc.balance -= req->amount;
    receiver_acc.balance += req->amount;

    // 7. Write Sender Data Back ---
    lseek(account_fd, sender_idx * sizeof(Account), SEEK_SET);
    if (write(account_fd, &sender_acc, sizeof(Account)) != sizeof(Account)) {
        res->success = 0;
        strcpy(res->message, "FATAL ERROR: Failed to write sender update. Transaction cancelled.");
        // In a real system, we would need to roll back, For now, we just report the error and unlock.
        unlock_record(account_fd, id1, sizeof(Account));
        unlock_record(account_fd, id2, sizeof(Account));
        return;
    }

    // 8. Write Receiver Data Back ---
    lseek(account_fd, receiver_idx * sizeof(Account), SEEK_SET);
    if (write(account_fd, &receiver_acc, sizeof(Account)) != sizeof(Account)) {
        res->success = 0;
        strcpy(res->message, "FATAL ERROR: Failed to write receiver update. Transaction inconsistent.");
        // This is the worst-case scenario: sender is debited, receiver is not credited.
        // This violates Atomicity. A real system needs a transaction journal, we will "try" to roll back the sender's debit.
        sender_acc.balance += req->amount; // Rollback
        lseek(account_fd, sender_idx * sizeof(Account), SEEK_SET);
        write(account_fd, &sender_acc, sizeof(Account)); // Try to write the rollback
        
        unlock_record(account_fd, id1, sizeof(Account));
        unlock_record(account_fd, id2, sizeof(Account));
        return;
    }

    // 9. Success! Log both sides of the transaction ---
    log_transaction(senderID, "TRANSFER_OUT", req->amount, sender_acc.balance);
    log_transaction(receiverID, "TRANSFER_IN", req->amount, receiver_acc.balance);

    res->success = 1;
    res->balance = sender_acc.balance; // Send sender's new balance
    sprintf(res->message, "Transfer of %.2f to account %d successful.", req->amount, receiverID);

    // 10.Unlock both records ---
    unlock_record(account_fd, id1, sizeof(Account));
    unlock_record(account_fd, id2, sizeof(Account));
}

void handle_withdraw(int client_socket, ClientRequest *req, ServerResponse *res) {
    if (req->amount <= 0) {
        res->success = 0;
        strcpy(res->message, "Error: Withdraw amount must be positive.");
        return;
    }

    Account acc;
    int accountID = req->userID; // we are keeping accountID == userID
    double old_balance;

    // Lock Record (WRITE Lock) ---
    // This will block if a deposit or transfer is in progress on this account.
    int record_index = lock_record(account_fd, accountID, F_WRLCK, sizeof(Account));
    if (record_index == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Account not found.");
        unlock_record(account_fd, accountID, sizeof(Account));
        return;
    }

    // 1. Read ---
    lseek(account_fd, record_index * sizeof(Account), SEEK_SET);
    if (read(account_fd, &acc, sizeof(Account)) != sizeof(Account)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to read account for withdrawal.");
        unlock_record(account_fd, accountID, sizeof(Account));
        return;
    }
    
    // 2.VALIDATE FUNDS---
    // This is the critical check that must happen after locking.
    if (acc.balance < req->amount) {
        res->success = 0;
        strcpy(res->message, "Error: Insufficient funds.");
        // We must unlock before returning
        unlock_record(account_fd, accountID, sizeof(Account));
        return;
    }

    // 3. Modify ---
    old_balance = acc.balance;
    acc.balance -= req->amount;

    // 4. Write Back ---
    lseek(account_fd, record_index * sizeof(Account), SEEK_SET); // Seek back
    if (write(account_fd, &acc, sizeof(Account)) != sizeof(Account)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to write updated balance.");
    } else {
        res->success = 1;
        res->balance = acc.balance;
        sprintf(res->message, "Withdrawal successful. New balance: %.2f", acc.balance);
        
        // 5. Log Transaction ---
        log_transaction(accountID, "WITHDRAW", req->amount, acc.balance);
    }

    // 6. Unlock Record ---
    unlock_record(account_fd, accountID, sizeof(Account));
}

void handle_add_customer(int client_socket, ClientRequest *req, ServerResponse *res) {
    User new_user, temp_user;
    Account new_account;
    
    // 1. Lock user file for uniqueness check and append ---
    // We lock the *entire file* to prevent race conditions
    // where two employees add a user with the same ID or username, we use a Write Lock because we will be appending.
    
    // Use ID 0 and length 0 as a signal for "whole file lock"
    if (lock_record(user_fd, 0, F_WRLCK, 0) == -1) {
         res->success = 0;
         strcpy(res->message, "Error: Could not lock user database. Try again.");
         return;
    }

    // 2. Check for username uniqueness ---
    lseek(user_fd, 0, SEEK_SET); // Rewind
    while (read(user_fd, &temp_user, sizeof(User)) == sizeof(User)) {
        if (strcmp(temp_user.username, req->username) == 0) {
            res->success = 0;
            strcpy(res->message, "Error: Username already exists.");
            unlock_record(user_fd, 0, 0); // Unlock and return
            return;
        }
    }

    // 3. Get new unique User ID ---
    int newID = get_next_user_id(); // This helper also uses user_fd

    // 4. Create new User struct ---
    new_user.userID = newID;
    strcpy(new_user.username, req->username);
    strcpy(new_user.password, req->password);
    new_user.role = CUSTOMER;
    new_user.isActive = 1;

    // 5. Append new User to file ---
    // We are already at the end of the file from the read loop
    lseek(user_fd, 0, SEEK_END);
    if (write(user_fd, &new_user, sizeof(User)) != sizeof(User)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to write new user to database.");
        unlock_record(user_fd, 0, 0);
        return;
    }
    
    // 6. Unlock user file ---
    unlock_record(user_fd, 0, 0);

    // 7. Lock account file to add new account ---
    if (lock_record(account_fd, 0, F_WRLCK, 0) == -1) {
         res->success = 0;
         strcpy(res->message, "Error: User created, but failed to lock account database.");
         // This is an inconsistent state, but we'll report it.
         return;
    }

    // 8. Create and append new Account struct ---
    new_account.accountID = newID; // Match the new User ID
    new_account.balance = 0.0;     // New accounts start with 0

    lseek(account_fd, 0, SEEK_END);
    if (write(account_fd, &new_account, sizeof(Account)) != sizeof(Account)) {
        res->success = 0;
        strcpy(res->message, "Error: User created, but failed to write new account.");
        unlock_record(account_fd, 0, 0);
        return;
    }
    
    // 9. Unlock account file ---
    unlock_record(account_fd, 0, 0);

    // 10. Success ---
    res->success = 1;
    res->userID = newID; // Send the new ID back to the employee
    sprintf(res->message, "New customer %s (ID: %d) created.", new_user.username, new_user.userID);
}

// Helper to get the next user ID.
// This function assumes user_fd is already write-locked!
int get_next_user_id() {
    User user;
    long file_size = lseek(user_fd, 0, SEEK_END);
    if (file_size == 0) {
        // No users, start at 1001 (or whatever base)
        // But setup.c creates users, so let's pick a customer start
        return 4001; 
    }

    // Seek to the last User record
    lseek(user_fd, -sizeof(User), SEEK_END);
    if (read(user_fd, &user, sizeof(User)) == sizeof(User)) {
        // Increment the last user's ID. This is a simple (but not robust) way to get a new ID, a better way would be to have a separate "sequence" file.
        return user.userID + 1;
    }
    
    // Fallback
    return 5001; 
}

void handle_add_user(int client_socket, ClientRequest *req, ServerResponse *res) {
    User new_user, temp_user;
    
    // 1. Validation ---
    UserRole roleToAdd = req->newUserRole;
    if (roleToAdd != CUSTOMER && roleToAdd != EMPLOYEE && roleToAdd != MANAGER) {
        res->success = 0;
        strcpy(res->message, "Error: Invalid role specified.");
        return;
    }

    // 2. Lock user file ---
    if (lock_record(user_fd, 0, F_WRLCK, 0) == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Could not lock user database. Try again.");
        return;
    }

    // 3. Check for username uniqueness ---
    lseek(user_fd, 0, SEEK_SET); // Rewind
    while (read(user_fd, &temp_user, sizeof(User)) == sizeof(User)) {
        if (strcmp(temp_user.username, req->username) == 0) {
            res->success = 0;
            strcpy(res->message, "Error: Username already exists.");
            unlock_record(user_fd, 0, 0); // Unlock and return
            return;
        }
    }

    // 4. Get new unique User ID ---
    int newID = get_next_user_id(); // This helper still works

    // 5. Create new User struct ---
    new_user.userID = newID;
    strcpy(new_user.username, req->username);
    strcpy(new_user.password, req->password);
    new_user.role = roleToAdd; // Set the role from the request
    new_user.isActive = 1;

    // 6. Append new User to file ---
    lseek(user_fd, 0, SEEK_END);
    if (write(user_fd, &new_user, sizeof(User)) != sizeof(User)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to write new user to database.");
        unlock_record(user_fd, 0, 0);
        return;
    }
    
    // 7. Unlock user file ---
    unlock_record(user_fd, 0, 0);

    // 8. Create an account *only* if it's a customer ---
    if (roleToAdd == CUSTOMER) {
        if (lock_record(account_fd, 0, F_WRLCK, 0) == -1) {
             res->success = 0;
             strcpy(res->message, "Error: User created, but failed to lock account database.");
             return;
        }

        Account new_account;
        new_account.accountID = newID; // Match the new User ID
        new_account.balance = 0.0;     // New accounts start with 0

        lseek(account_fd, 0, SEEK_END);
        if (write(account_fd, &new_account, sizeof(Account)) != sizeof(Account)) {
            res->success = 0;
            strcpy(res->message, "Error: User created, but failed to write new account.");
            unlock_record(account_fd, 0, 0);
            return;
        }
        
        unlock_record(account_fd, 0, 0);
    }

    // 9. Success ---
    res->success = 1;
    res->userID = newID; // Send the new ID back
    sprintf(res->message, "New user %s (ID: %d, Role: %d) created.", new_user.username, new_user.userID, new_user.role);
}

void handle_modify_customer(int client_socket, ClientRequest *req, ServerResponse *res) {
    int targetID = req->targetAccountID; // The Customer ID to modify
    char newUsername[MAX_USERNAME_LEN];
    char newPassword[MAX_PASSWORD_LEN];
    strcpy(newUsername, req->username);
    strcpy(newPassword, req->password);
    
    User user;
    int found = 0;
    int username_changed = (strcmp(newUsername, ".") != 0);
    int password_changed = (strcmp(newPassword, ".") != 0);

    // Checking if any change was actually requested
    if (!username_changed && !password_changed) {
        res->success = 0;
        strcpy(res->message, "Error: No changes requested.");
        return;
    }

    // 1. Locking the ENTIRE user file ---
    // We need a full write lock to check for username uniqueness safely.
    if (lock_record(user_fd, 0, F_WRLCK, 0) == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Could not lock user database.");
        return;
    }

    // 2. Uniqueness Check (if username is being changed) ---
    if (username_changed) {
        lseek(user_fd, 0, SEEK_SET); // Rewind
        User temp_user;
        while (read(user_fd, &temp_user, sizeof(User)) == sizeof(User)) {
            // Check if any *other* user has this username
            if (strcmp(temp_user.username, newUsername) == 0 && temp_user.userID != targetID) {
                res->success = 0;
                strcpy(res->message, "Error: Username already exists.");
                unlock_record(user_fd, 0, 0); // Unlock and return
                return;
            }
        }
    }

    // 3. Find the user and perform the update ---
    lseek(user_fd, 0, SEEK_SET); // Rewind again to find and update
    long current_offset = 0;
    
    while (read(user_fd, &user, sizeof(User)) == sizeof(User)) {
        if (user.userID == targetID) {
            found = 1;
            // Security Check
            if (user.role != CUSTOMER) {
                res->success = 0;
                strcpy(res->message, "Error: You can only modify Customer accounts.");
                break; // Exit loop
            }

            // Modify the struct in memory
            if (username_changed) {
                strcpy(user.username, newUsername);
            }
            if (password_changed) {
                strcpy(user.password, newPassword);
            }

            // Write the modified struct back
            lseek(user_fd, current_offset, SEEK_SET); // Seek back
            if (write(user_fd, &user, sizeof(User)) != sizeof(User)) {
                res->success = 0;
                strcpy(res->message, "Error: Failed to write updated user data.");
            } else {
                res->success = 1;
                strcpy(res->message, "Customer details updated successfully.");
            }
            break; // Exit loop
        }
        current_offset += sizeof(User); // Track our position
    }

    if (!found) {
        res->success = 0;
        strcpy(res->message, "Error: User not found.");
    }

    // 4. Unlock the file ---
    unlock_record(user_fd, 0, 0);
}

void handle_toggle_account_status(int client_socket, ClientRequest *req, ServerResponse *res) {
    int targetID = req->targetAccountID; // The Customer ID to toggle
    User user;

    // 1. Lock the specific user record ---
    int record_index = lock_record(user_fd, targetID, F_WRLCK, sizeof(User));
    if (record_index == -1) {
        res->success = 0;
        strcpy(res->message, "Error: User not found.");
        unlock_record(user_fd, targetID, sizeof(User));
        return;
    }

    // 2. Read the user's data ---
    lseek(user_fd, record_index * sizeof(User), SEEK_SET);
    if (read(user_fd, &user, sizeof(User)) != sizeof(User)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to read user data.");
        unlock_record(user_fd, targetID, sizeof(User));
        return;
    }

    // 3.Security Check---
    // A manager should ONLY be able to toggle CUSTOMER accounts
    if (user.role != CUSTOMER) {
        res->success = 0;
        strcpy(res->message, "Error: You can only modify Customer accounts.");
        unlock_record(user_fd, targetID, sizeof(User));
        return;
    }

    // 4. Modify the struct (Flip the flag) ---
    if (user.isActive) {
        user.isActive = 0; // Deactivate
    } else {
        user.isActive = 1; // Activate
    }

    // 5. Write the modified struct back ---
    lseek(user_fd, record_index * sizeof(User), SEEK_SET); // Seek back
    if (write(user_fd, &user, sizeof(User)) != sizeof(User)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to write updated user data.");
    } else {
        res->success = 1;
        sprintf(res->message, "Customer %d status set to: %s", 
                user.userID, user.isActive ? "ACTIVE" : "DEACTIVATED");
    }

    // 6. Unlock the record ---
    unlock_record(user_fd, targetID, sizeof(User));
}

void handle_admin_modify_user(int client_socket, ClientRequest *req, ServerResponse *res) {
    int targetID = req->targetAccountID; // The User ID to modify
    char newUsername[MAX_USERNAME_LEN];
    char newPassword[MAX_PASSWORD_LEN];
    strcpy(newUsername, req->username);
    strcpy(newPassword, req->password);
    
    User user;
    int found = 0;
    int username_changed = (strcmp(newUsername, ".") != 0);
    int password_changed = (strcmp(newPassword, ".") != 0);

    // Checking if any change was actually requested by client
    if (!username_changed && !password_changed) {
        res->success = 0;
        strcpy(res->message, "Error: No changes requested.");
        return;
    }

    // 1. Lock the ENTIRE user file ---
    if (lock_record(user_fd, 0, F_WRLCK, 0) == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Could not lock user database.");
        return;
    }

    // 2. Uniqueness Check (if username is being changed) ---
    if (username_changed) {
        lseek(user_fd, 0, SEEK_SET); // Rewind
        User temp_user;
        while (read(user_fd, &temp_user, sizeof(User)) == sizeof(User)) {
            // Check if any *other* user has this username
            if (strcmp(temp_user.username, newUsername) == 0 && temp_user.userID != targetID) {
                res->success = 0;
                strcpy(res->message, "Error: Username already exists.");
                unlock_record(user_fd, 0, 0); // Unlock and return
                return;
            }
        }
    }

    // 3. Find the user and perform the update ---
    lseek(user_fd, 0, SEEK_SET); // Rewind again to find and update
    long current_offset = 0;
    
    while (read(user_fd, &user, sizeof(User)) == sizeof(User)) {
        if (user.userID == targetID) {
            found = 1;
            
            // ---Admin Security Check---
            // Admin can modify Customer, Employee, or Manager.
            // Admin can also modify themselves.
            // Admin CANNOT modify another Admin.
            if (user.role == ADMIN && user.userID != req->userID) {
                res->success = 0;
                strcpy(res->message, "Error: An Admin cannot modify another Admin's details.");
                break; // Exit loop
            }

            // Modify the struct in memory
            if (username_changed) {
                strcpy(user.username, newUsername);
            }
            if (password_changed) {
                strcpy(user.password, newPassword);
            }

            // Write the modified struct back
            lseek(user_fd, current_offset, SEEK_SET); // Seek back
            if (write(user_fd, &user, sizeof(User)) != sizeof(User)) {
                res->success = 0;
                strcpy(res->message, "Error: Failed to write updated user data.");
            } else {
                res->success = 1;
                sprintf(res->message, "User %d details updated successfully.", user.userID);
            }
            break; // Exit loop
        }
        current_offset += sizeof(User); // Track our position
    }

    if (!found) {
        res->success = 0;
        strcpy(res->message, "Error: User not found.");
    }

    // 4. Unlock the file ---
    unlock_record(user_fd, 0, 0);
}

void handle_apply_loan(int client_socket, ClientRequest *req, ServerResponse *res) {
    if (req->amount <= 0) {
        res->success = 0;
        strcpy(res->message, "Error: Loan amount must be positive.");
        return;
    }

    // Lock the entire loan file for appending ---
    if (lock_record(loan_fd, 0, F_WRLCK, 0) == -1) {
         res->success = 0;
         strcpy(res->message, "Error: Could not lock loan database. Try again.");
         return;
    }

    Loan new_loan;
    new_loan.loanID = get_next_loan_id(); // Get new ID while locked
    new_loan.customerID = req->userID;
    new_loan.amount = req->amount;
    new_loan.timestamp = time(NULL);
    strcpy(new_loan.status, "PENDING");

    // Append new loan to file ---
    lseek(loan_fd, 0, SEEK_END);
    if (write(loan_fd, &new_loan, sizeof(Loan)) != sizeof(Loan)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to write new loan to database.");
    } else {
        res->success = 1;
        sprintf(res->message, "Loan application for %.2f submitted with ID %d.", 
                new_loan.amount, new_loan.loanID);
    }

    // Unlock loan file ---
    unlock_record(loan_fd, 0, 0);
}

// This function also assumes loan_fd is already write-locked!
int get_next_loan_id() {
    Loan loan;
    long file_size = lseek(loan_fd, 0, SEEK_END);
    if (file_size == 0) {
        return 1; // First loan
    }

    // Seek to the last Loan record
    lseek(loan_fd, -sizeof(Loan), SEEK_END);
    if (read(loan_fd, &loan, sizeof(Loan)) == sizeof(Loan)) {
        return loan.loanID + 1; // Increment the last ID
    }
    
    // Fallback
    return 1; 
}

void handle_list_pending_loans(int client_socket, ClientRequest *req, ServerResponse *res) {
    Loan loan;
    char buffer[MAX_MSG_LEN] = {0}; // Full list buffer
    char line[100]; // Buffer for one line
    int found = 0;

    // Lock the whole loan file for reading
    if (lock_record(loan_fd, 0, F_RDLCK, 0) == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Cannot access loan database.");
        return;
    }

    lseek(loan_fd, 0, SEEK_SET);
    while (read(loan_fd, &loan, sizeof(Loan)) == sizeof(Loan)) {
        if (strcmp(loan.status, "PENDING") == 0) {
            sprintf(line, "  ID: %d | Customer: %d | Amount: %.2f\n", 
                    loan.loanID, loan.customerID, loan.amount);
            strcat(buffer, line);
            found = 1;
        }
    }

    unlock_record(loan_fd, 0, 0); // Release file lock

    if (found) {
        res->success = 1;
        strcpy(res->message, buffer);
    } else {
        res->success = 0;
        strcpy(res->message, "No pending loan applications found.\n");
    }
}

void handle_process_loan(int client_socket, ClientRequest *req, ServerResponse *res) {
    int loanID = req->targetAccountID;
    int action = (int)req->amount; // 1 for Approve, 0 for Reject
    int customerID = -1;
    
    Loan loan;
    Account acc;

    // 1. Find the Customer ID (Read-only search) ---
    // We do a "dirty read" just to find the customerID for locking.
    lock_record(loan_fd, 0, F_RDLCK, 0); // Read lock whole file
    lseek(loan_fd, 0, SEEK_SET);
    while (read(loan_fd, &loan, sizeof(Loan)) == sizeof(Loan)) {
        if (loan.loanID == loanID) {
            customerID = loan.customerID;
            break;
        }
    }
    unlock_record(loan_fd, 0, 0); // Unlock

    if (customerID == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Loan ID not found.");
        return;
    }

    // 2.Atomic Update: Lock BOTH files---
    // We lock in a consistent order to prevent deadlock:
    // 1. Account File, 2. Loan File
    
    int account_idx = lock_record(account_fd, customerID, F_WRLCK, sizeof(Account));
    if (account_idx == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Could not lock customer account.");
        return;
    }

    int loan_idx = lock_record(loan_fd, loanID, F_WRLCK, sizeof(Loan));
    if (loan_idx == -1) {
        res->success = 0;
        strcpy(res->message, "Error: Could not lock loan record.");
        unlock_record(account_fd, customerID, sizeof(Account)); // MUST unlock
        return;
    }

    // 3. Both locks acquired. Re-read loan to be safe. ---
    lseek(loan_fd, loan_idx * sizeof(Loan), SEEK_SET);
    if (read(loan_fd, &loan, sizeof(Loan)) != sizeof(Loan)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to read loan record after lock.");
    } 
    else if (strcmp(loan.status, "PENDING") != 0) {
        res->success = 0;
        strcpy(res->message, "Error: Loan has already been processed.");
    }
    else {
        // 4. Process the action ---
        if (action == 1) { // APPROVE
            lseek(account_fd, account_idx * sizeof(Account), SEEK_SET);
            read(account_fd, &acc, sizeof(Account));
            
            acc.balance += loan.amount; // Deposit funds
            
            lseek(account_fd, account_idx * sizeof(Account), SEEK_SET);
            write(account_fd, &acc, sizeof(Account)); // Write back account
            
            strcpy(loan.status, "APPROVED");
            lseek(loan_fd, loan_idx * sizeof(Loan), SEEK_SET);
            write(loan_fd, &loan, sizeof(Loan)); // Write back loan
            
            log_transaction(customerID, "LOAN_DEPOSIT", loan.amount, acc.balance);
            
            res->success = 1;
            strcpy(res->message, "Loan approved. Funds deposited.");
        } 
        else { // REJECT
            strcpy(loan.status, "REJECTED");
            lseek(loan_fd, loan_idx * sizeof(Loan), SEEK_SET);
            write(loan_fd, &loan, sizeof(Loan)); // Write back loan
            
            res->success = 1;
            strcpy(res->message, "Loan rejected.");
        }
    }

    // 5. Unlock BOTH files (in reverse order) ---
    unlock_record(loan_fd, loanID, sizeof(Loan));
    unlock_record(account_fd, customerID, sizeof(Account));
}

void handle_view_transactions(int client_socket, ClientRequest *req, ServerResponse *res) {
    int customerID = req->userID; // This is the accountID
    Transaction trans;
    
    char buffer[MAX_MSG_LEN] = {0}; // Buffer for the final response
    char line[100]; // Buffer for a single line
    int count = 0;

    // 1. Lock the transaction file for reading ---
    if (lock_record(transaction_fd, 0, F_RDLCK, 0) == -1) {
         res->success = 0;
         strcpy(res->message, "Error: Could not lock transaction database.");
         return;
    }

    // 2. Find the end of the file ---
    long file_size = lseek(transaction_fd, 0, SEEK_END);
    if (file_size == 0) {
        res->success = 1; // Not an error, just no transactions
        strcpy(res->message, "No transactions found.\n");
        unlock_record(transaction_fd, 0, 0);
        return;
    }

    // 3. Read backwards from the end ---
    // Start at the beginning of the *last* record
    long current_offset = file_size - sizeof(Transaction); 

    while (current_offset >= 0 && count < 5) {
        // Seek to the start of the record
        lseek(transaction_fd, current_offset, SEEK_SET);
        
        if (read(transaction_fd, &trans, sizeof(Transaction)) == sizeof(Transaction)) {
            // Check if this transaction belongs to our customer
            if (trans.accountID == customerID) {
                // Format the line
                sprintf(line, "[ID: %d] %s: %.2f (Balance: %.2f)\n", 
                        trans.transactionID, trans.type, trans.amount, trans.newBalance);
                
                // Add to our main buffer (this builds the list in reverse-chrono order)
                strcat(buffer, line);
                count++;
            }
        }
        
        // Move to the *previous* record
        current_offset -= sizeof(Transaction);
    }

    // 4. Unlock the file ---
    unlock_record(transaction_fd, 0, 0);

    // 5. Send the result ---
    if (count == 0) {
        strcpy(res->message, "No transactions found for your account.\n");
    } else {
        strcpy(res->message, buffer);
    }
    res->success = 1;
}

void handle_add_feedback(int client_socket, ClientRequest *req, ServerResponse *res) {
    // Lock the entire feedback file for appending ---
    if (lock_record(feedback_fd, 0, F_WRLCK, 0) == -1) {
         res->success = 0;
         strcpy(res->message, "Error: Could not lock feedback database. Try again.");
         return;
    }

    Feedback new_feedback;
    new_feedback.feedbackID = get_next_feedback_id(); // Get new ID while locked
    new_feedback.customerID = req->userID;
    strncpy(new_feedback.message, req->feedbackMessage, MAX_MSG_LEN);
    new_feedback.timestamp = time(NULL);

    // Append new feedback to file ---
    lseek(feedback_fd, 0, SEEK_END);
    if (write(feedback_fd, &new_feedback, sizeof(Feedback)) != sizeof(Feedback)) {
        res->success = 0;
        strcpy(res->message, "Error: Failed to write new feedback to database.");
    } else {
        res->success = 1;
        strcpy(res->message, "Feedback submitted successfully. Thank you!");
    }

    // Unlock feedback file ---
    unlock_record(feedback_fd, 0, 0);
}

// This function also assumes feedback_fd is already write-locked!
int get_next_feedback_id() {
    Feedback feedback;
    long file_size = lseek(feedback_fd, 0, SEEK_END);
    if (file_size == 0) {
        return 1; // First feedback
    }

    // Seek to the last Feedback record
    lseek(feedback_fd, -sizeof(Feedback), SEEK_END);
    if (read(feedback_fd, &feedback, sizeof(Feedback)) == sizeof(Feedback)) {
        return feedback.feedbackID + 1; // Increment the last ID
    }
    
    // Fallback
    return 1; 
}

void handle_review_feedback(int client_socket, ClientRequest *req, ServerResponse *res) {
    Feedback feedback;
    
    char buffer[MAX_MSG_LEN] = {0}; // Buffer for the final response
    char line[MAX_MSG_LEN + 50];  // Buffer for a single line
    int found = 0;

    // 1. Lock the feedback file for reading ---
    if (lock_record(feedback_fd, 0, F_RDLCK, 0) == -1) {
         res->success = 0;
         strcpy(res->message, "Error: Could not lock feedback database.");
         return;
    }

    // 2. Read from the start ---
    lseek(feedback_fd, 0, SEEK_SET);
    
    while (read(feedback_fd, &feedback, sizeof(Feedback)) == sizeof(Feedback)) {
        found = 1;
        // Format the line
        sprintf(line, "[ID: %d | Cust: %d] %s\n", 
                feedback.feedbackID, feedback.customerID, feedback.message);
        
        // 3. Check for buffer overflow before appending ---
        if (strlen(buffer) + strlen(line) < MAX_MSG_LEN - 1) {
            strcat(buffer, line);
        } else {
            // Stop appending if we run out of space
            strcat(buffer, "... (more feedback truncated)\n");
            break; 
        }
    }

    // 4. Unlock the file ---
    unlock_record(feedback_fd, 0, 0);

    // 5. Send the result ---
    if (!found) {
        strcpy(res->message, "No feedback found.\n");
    } else {
        strcpy(res->message, buffer);
    }
    res->success = 1;
}
