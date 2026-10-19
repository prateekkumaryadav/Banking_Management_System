#include "common.h"

// --- Global State for the Client ---
int server_socket;
int g_userID = -1; // Global to store our ID after login
UserRole g_role = 0;

// -------------------------------------------------------

// --- Function Prototypes ---
void show_customer_menu();
void show_employee_menu();
void show_manager_menu();
void show_admin_menu();

void handle_login();

// --- Client/Customer
void handle_get_balance();
void handle_deposit();
void handle_withdraw();
void handle_transfer();
void handle_apply_loan();
void handle_view_transactions();
void handle_add_feedback();

// --- Employee
void handle_add_customer();
void handle_modify_customer();

// --- Manager
void handle_toggle_account_status();
void handle_process_loan();
void handle_review_feedback();

// --- Admin
void handle_add_employee();
void handle_add_manager();
void handle_admin_modify_user();

// -------------------------------------------------------

// --- Main Client Function ---
int main() {
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Use "127.0.0.1" for localhost
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to bank server.\n");

    // --- 1. Login ---
    while(g_userID == -1) {
        handle_login();
    }

    // --- 2. Show Menu Based on Role ---
    switch (g_role) {
        case CUSTOMER:
            show_customer_menu();
            break;
        case EMPLOYEE:
            show_employee_menu();
            break;
        case MANAGER:
            show_manager_menu();
            break;
        case ADMIN:
            show_admin_menu();
            break;
        default:
            printf("Error: Unknown role.\n");
            break;
    }

    close(server_socket);
    return 0;
}

// -------------------------------------------------------

// --- Show menu functions for Respective users ---
void show_customer_menu() {
    int choice = 0;
    while (choice != 9) {
        printf("\n--- Customer Menu ---\n");
        printf("1. View Balance\n");
        printf("2. Deposit Money\n");
        printf("3. Withdraw Money\n");
        printf("4. Transfer Funds\n");
        printf("5. Apply for a Loan\n");
        printf("6. View Transaction History\n");
        printf("7. Add Feedback\n");
        printf("9. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                handle_get_balance();
                break;
            case 2:
                handle_deposit();
                break;
            case 3:
                handle_withdraw();
                break;
            case 4:
                handle_transfer();
                break;
            case 5:
                handle_apply_loan();
                break;
            case 6:
                handle_view_transactions();
                break;
            case 7:
                handle_add_feedback();
                break;
            case 9:
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}

void show_employee_menu() {
    int choice = 0;
    while (choice != 9) {
        printf("\n--- Employee Menu ---\n");
        printf("1. Add New Customer\n");
        printf("2. Modify Customer Details\n");
        printf("9. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                handle_add_customer();
                break;
            case 2:
                handle_modify_customer();
                break;
            case 9:
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}

void show_manager_menu() {
    int choice = 0;
    while (choice != 9) {
        printf("\n--- Manager Menu ---\n");
        printf("1. Activate/Deactivate Customer Account\n");
        printf("2. Process Loan Applications\n");
        printf("3. Review Customer Feedback\n");
        printf("9. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                handle_toggle_account_status();
                break;
            case 2:
                handle_process_loan();
                break;
            case 3:
                handle_review_feedback();
                break;
            case 9:
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}

void show_admin_menu() {
    int choice = 0;
    while (choice != 9) {
        printf("\n--- Administrator Menu ---\n");
        printf("1. Add New Bank Employee\n");
        printf("2. Add New Bank Manager\n");
        printf("3. Modify User Details\n");
        printf("9. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                handle_add_employee();
                break;
            case 2:
                handle_add_manager();
                break;
            case 3:
                handle_admin_modify_user();
                break;
            case 9:
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
}

void handle_login() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "LOGIN");

    printf("\n--- Login ---\n");
    printf("Username: ");
    scanf("%s", req.username);
    printf("Password: ");
    scanf("%s", req.password);

    // Send request
    write(server_socket, &req, sizeof(ClientRequest));
    
    // Get response
    read(server_socket, &res, sizeof(ServerResponse));

    // Display Server message
    printf("Server: %s\n", res.message);

    if (res.success) {
        // Set global state
        g_userID = res.userID;
        g_role = res.role;
    }
}

// -------------------------------------------------------

// --- Client-Side Handlers ---
// 1. --- Handling Get Balance
void handle_get_balance() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "GET_BALANCE");
    req.userID = g_userID; // Send our UID for context

    write(server_socket, &req, sizeof(ClientRequest));
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) printf("Your current balance is: %.2f\n", res.balance);
    else printf("Server Error: %s\n", res.message);
}

// 2. --- Handling Deposit
void handle_deposit() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "DEPOSIT");
    req.userID = g_userID;

    printf("Enter amount to deposit: ");
    scanf("%lf", &req.amount);

    write(server_socket, &req, sizeof(ClientRequest));
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) printf("Deposit successful. New balance: %.2f\n", res.balance);
    else printf("Server Error: %s\n", res.message);
}

// 3. --- Handling withdrawal
void handle_withdraw() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "WITHDRAW");
    req.userID = g_userID;

    printf("Enter amount to withdraw: ");
    scanf("%lf", &req.amount);

    write(server_socket, &req, sizeof(ClientRequest));
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) printf("Withdrawal successful. New balance: %.2f\n", res.balance); 
    else printf("Server Error: %s\n", res.message);
}

// 4. --- Handling Funds Transfer
void handle_transfer() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "TRANSFER");
    req.userID = g_userID; // passing our UID as we are the sender

    printf("Enter target account ID: ");
    scanf("%d", &req.targetAccountID);

    printf("Enter amount to transfer: ");
    scanf("%lf", &req.amount);

    // Send the request
    write(server_socket, &req, sizeof(ClientRequest));
    
    // Read the response
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) {
        printf("Transfer successful.\n");
        printf("Your new balance is: %.2f\n", res.balance);
    } else printf("Server Error: %s\n", res.message);
}

// 5. --- Handling Loan Apply
void handle_apply_loan() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "APPLY_LOAN");
    req.userID = g_userID;

    printf("Enter amount for loan: ");
    scanf("%lf", &req.amount);

    write(server_socket, &req, sizeof(ClientRequest));
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) printf("Success: %s\n", res.message);
    else printf("Server Error: %s\n", res.message);
}

// 6. --- Handling Mini statement
void handle_view_transactions() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "GET_TRANSACTIONS");
    req.userID = g_userID;

    write(server_socket, &req, sizeof(ClientRequest));
    read(server_socket, &res, sizeof(ServerResponse));

    printf("\n--- Your Last 5 Transactions ---\n");
    // Server return entire list in a pre-formated message
    printf("%s", res.message);
    printf("----------------------------------\n");
}

// 7. --- Handling Give Feedback
void handle_add_feedback() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "ADD_FEEDBACK");
    req.userID = g_userID;

    printf("Enter your feedback (max %d chars):\n", MAX_MSG_LEN - 1);
    
    // Clear the input buffer from the menu's scanf
    int c;
    while ((c = getchar()) != '\n' && c != EOF); 

    // Read the feedback line
    fgets(req.feedbackMessage, MAX_MSG_LEN, stdin);
    
    // Remove trailing newline from fgets
    req.feedbackMessage[strcspn(req.feedbackMessage, "\n")] = 0;

    write(server_socket, &req, sizeof(ClientRequest));
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) printf("Success: %s\n", res.message);
    else printf("Server Error: %s\n", res.message);
}

// -------------------------------------------------------

// --- Employee-Side Handlers ---
// 1. --- Handling Add Customer
void handle_add_customer() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "ADD_CUSTOMER");
    
    // The employee's userID is sent for logging/permission check
    req.userID = g_userID; 

    printf("Enter new customer's username: ");
    scanf("%s", req.username);
    printf("Enter new customer's temporary password: ");
    scanf("%s", req.password);

    // Send the request
    write(server_socket, &req, sizeof(ClientRequest));
    
    // Read the response
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) {  // The server sends back the new user ID in the 'userID' field
        printf("Customer added successfully.\n");
        printf("New Customer ID: %d\n", res.userID);
    } else printf("Server Error: %s\n", res.message);
}

// 2. --- Handling Modify Customer details
void handle_modify_customer() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "MODIFY_CUSTOMER");
    
    // The employee's userID for auth
    req.userID = g_userID; 

    printf("Enter Customer ID to modify: ");
    scanf("%d", &req.targetAccountID); // Re-using this field for targetUserID

    printf("Enter new username (or '.' to skip): ");
    scanf("%s", req.username); // We will send the new username here

    printf("Enter new password (or '.' to skip): ");
    scanf("%s", req.password); // We will send the new password here

    // Send the request
    write(server_socket, &req, sizeof(ClientRequest));
    
    // Read the response
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) printf("Customer details modified successfully.\n");
    else printf("Server Error: %s\n", res.message);
}

// -------------------------------------------------------

// --- Manager-Side Handlers ---
// 1. --- Handling Toggle Account Status
void handle_toggle_account_status() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "TOGGLE_STATUS");
    
    // The manager's userID for auth/logging
    req.userID = g_userID; 

    printf("Enter Customer ID to activate/deactivate: ");
    scanf("%d", &req.targetAccountID); // Re-using this field for targetUserID

    // Send the request
    write(server_socket, &req, sizeof(ClientRequest));
    
    // Read the response
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) printf("Success: %s\n", res.message);
    else printf("Server Error: %s\n", res.message);
}

// 2. --- Handling Process Loan
void handle_process_loan() {
    ClientRequest req;
    ServerResponse res;
    
    // 1. Listing out all the pending loan applications
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "LIST_PENDING_LOANS");
    req.userID = g_userID; // Manager's ID for auth

    write(server_socket, &req, sizeof(ClientRequest));
    read(server_socket, &res, sizeof(ServerResponse));

    printf("\n--- Pending Loan Applications ---\n");
    printf("%s", res.message); // Server will send a list or simply print "No loans"
    printf("---------------------------------\n");

    if (!res.success) return; // No loan applications to process

    // 2. Take manager's choice
    int loanID_to_process;
    int action;
    
    printf("Enter Loan ID to process: ");
    scanf("%d", &loanID_to_process);
    
    printf("Enter 1 to APPROVE or 0 to REJECT: ");
    scanf("%d", &action);

    if (action != 0 && action != 1) {
        printf("Invalid action. Aborting.\n");
        return;
    }

    // 3. Send the processing request
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "PROCESS_LOAN");
    req.userID = g_userID; // Manager's ID
    req.targetAccountID = loanID_to_process; // Re-using this field for loanID
    req.amount = (double)action; // Re-using this field for action (1.0 or 0.0)

    write(server_socket, &req, sizeof(ClientRequest));
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) printf("Success: %s\n", res.message);
    else printf("Server Error: %s\n", res.message);
}

// 3. --- Handling Review Feedback
void handle_review_feedback() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "REVIEW_FEEDBACK");
    req.userID = g_userID; // Manager's ID for auth

    write(server_socket, &req, sizeof(ClientRequest));
    read(server_socket, &res, sizeof(ServerResponse));

    printf("\n--- All Customer Feedback ---\n");
    // Server pre-formats the entire list into a message
    printf("%s", res.message);
    printf("------------------------------\n");
}

// -------------------------------------------------------

// --- Admin-Side Handlers ---
// 1. --- Handling Add Employee
void handle_add_employee() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "ADD_USER");
    
    req.userID = g_userID; // The admin's userID for auth

    printf("Enter new employee's username: ");
    scanf("%s", req.username);
    printf("Enter new employee's temporary password: ");
    scanf("%s", req.password);
    
    // Specifying the role to create
    req.newUserRole = EMPLOYEE;

    // Send the request
    write(server_socket, &req, sizeof(ClientRequest));
    
    // Read the response
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) {
        printf("Employee added successfully.\n");
        printf("New Employee ID: %d\n", res.userID);
    } else printf("Server Error: %s\n", res.message);
}

// 2. --- Handling Add Manager
void handle_add_manager() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "ADD_USER");
    
    req.userID = g_userID; // The admin's userID for auth

    printf("Enter new manager's username: ");
    scanf("%s", req.username);
    printf("Enter new manager's temporary password: ");
    scanf("%s", req.password);
    
    // This is the only difference from handle_add_employee()
    req.newUserRole = MANAGER;

    // Send the request
    write(server_socket, &req, sizeof(ClientRequest));
    
    // Read the response
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) {
        printf("Manager added successfully.\n");
        printf("New Manager ID: %d\n", res.userID);
    } else printf("Server Error: %s\n", res.message);
}

// 3. --- Handling Modify User
void handle_admin_modify_user() {
    ClientRequest req;
    ServerResponse res;
    
    memset(&req, 0, sizeof(ClientRequest));
    strcpy(req.command, "ADMIN_MOD_USER");
    
    req.userID = g_userID; // The admin's userID for auth

    printf("Enter User ID to modify: ");
    scanf("%d", &req.targetAccountID); // Re-using this field for targetUserID

    printf("Enter new username (or '.' to skip): ");
    scanf("%s", req.username); // We will send the new username here

    printf("Enter new password (or '.' to skip): ");
    scanf("%s", req.password); // We will send the new password here

    // Send the request
    write(server_socket, &req, sizeof(ClientRequest));
    
    // Read the response
    read(server_socket, &res, sizeof(ServerResponse));

    if (res.success) printf("User details modified successfully.\n");
    else printf("Server Error: %s\n", res.message);
}

// -------------------------------------------------------