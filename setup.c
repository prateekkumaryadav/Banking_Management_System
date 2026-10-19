#include "common.h"
#include <sys/stat.h> // For mkdir

// This is a one-time setup utility to initialize data files 
// required for smoothly processing for application's working
int main() {
    int user_fd, account_fd, loan_fd, feedback_fd;

    // Creating a 'data' directory if it doesn't exist in the locality
    mkdir("data", 0777);

    // Opening files, truncating them (O_TRUNC) to start fresh
    // By removing previous version's entries
    user_fd = open(USER_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    account_fd = open(ACCOUNT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    loan_fd = open(LOAN_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    feedback_fd = open(FEEDBACK_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    
    if (user_fd == -1 || account_fd == -1 || loan_fd == -1 || feedback_fd == -1) {
        perror("Error opening files for setup");
        exit(EXIT_FAILURE);
    }

    printf("Initializing data files...\n");

    // --- Creating Admin ---
    User admin;
    admin.userID = 1001;
    strcpy(admin.username, "admin");
    strcpy(admin.password, "admin123");
    admin.role = ADMIN;
    admin.isActive = 1;
    admin.sessionActive = 0;
    write(user_fd, &admin, sizeof(User));

    // --- Creating Manager ---
    User manager;
    manager.userID = 2001;
    strcpy(manager.username, "manager");
    strcpy(manager.password, "manager123");
    manager.role = MANAGER;
    manager.isActive = 1;
    manager.sessionActive = 0;
    write(user_fd, &manager, sizeof(User));

    // --- Creating Employees ---
    User employee1;
    employee1.userID = 3001;
    strcpy(employee1.username, "emp1");
    strcpy(employee1.password, "emp123");
    employee1.role = EMPLOYEE;
    employee1.isActive = 1;
    employee1.sessionActive = 0;
    write(user_fd, &employee1, sizeof(User));
    
    User employee2;
    employee2.userID = 3002;
    strcpy(employee2.username, "emp2");
    strcpy(employee2.password, "emp123");
    employee2.role = EMPLOYEE;
    employee2.isActive = 1;
    employee2.sessionActive = 0;
    write(user_fd, &employee2, sizeof(User));

    // --- Creating Customers ---
    User customer1;
    customer1.userID = 4001;
    strcpy(customer1.username, "cust1");
    strcpy(customer1.password, "cust123");
    customer1.role = CUSTOMER;
    customer1.isActive = 1;
    customer1.sessionActive = 0;
    write(user_fd, &customer1, sizeof(User));

    User customer2;
    customer2.userID = 4002;
    strcpy(customer2.username, "cust2");
    strcpy(customer2.password, "cust123");
    customer2.role = CUSTOMER;
    customer2.isActive = 1;
    customer2.sessionActive = 0;
    write(user_fd, &customer2, sizeof(User));
    
    User customer3;
    customer3.userID = 4003;
    strcpy(customer3.username, "cust3");
    strcpy(customer3.password, "cust123");
    customer3.role = CUSTOMER;
    customer3.isActive = 1;
    customer3.sessionActive = 0;
    write(user_fd, &customer3, sizeof(User));
    
     // --- Create Customer's Account ---
    Account acc1;
    acc1.accountID = 4001; // Matches customer's userID
    acc1.balance = 1000.00;
    write(account_fd, &acc1, sizeof(Account));

    Account acc2;
    acc2.accountID = 4002;
    acc2.balance = 500.00;
    write(account_fd, &acc2, sizeof(Account));
    
    Account acc3;
    acc3.accountID = 4003;
    acc3.balance = 1500.00;
    write(account_fd, &acc3, sizeof(Account));


    close(user_fd);
    close(account_fd);
    close(loan_fd);
    close(feedback_fd);

    printf("Data files initialized successfully.\n");
    printf("Users created:\n");
    printf("  - admin / admin123 (Admin)\n");
    printf("  - manager / manager123 (Manager)\n");
    printf("  - emp1 / emp123 (Employee)\n");
    printf("  - emp2 / emp123 (Employee)\n");
    printf("  - cust1 / cust123 (Customer, ID 4001, Bal: 1000.00)\n");
    printf("  - cust2 / cust123 (Customer, ID 4002, Bal: 500.00)\n");
    printf("  - cust3 / cust123 (Customer, ID 4003, Bal: 1500.00)\n");

    return 0;
}