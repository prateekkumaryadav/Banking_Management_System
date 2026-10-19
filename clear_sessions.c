#include "common.h"

// Run this if users are stuck and cannot log in
// will clear out all active sessions

int main() {
    int user_fd;
    User user;
    int count = 0;

    user_fd = open(USER_FILE, O_RDWR);
    if (user_fd == -1) {
        perror("Error opening user file");
        printf("Make sure the data/users.dat file exists.\n");
        exit(EXIT_FAILURE);
    }

    printf("Clearing all active sessions...\n");

    // Read each user record to look for whether they are currently active or not
    long offset = 0;
    while (read(user_fd, &user, sizeof(User)) == sizeof(User)) {
        if (user.sessionActive == 1) {
            printf("  - Clearing session for user: %s (ID: %d, Role: %d)\n", user.username, user.userID, user.role);
            user.sessionActive = 0;
            
            // Seek back to the start of this record and write
            // go inside data/users.dat and check sessionActive for each user
            lseek(user_fd, offset, SEEK_SET);
            write(user_fd, &user, sizeof(User));
            
            // Move to next record position to look for next user
            lseek(user_fd, offset + sizeof(User), SEEK_SET);
            count++;
        }
        offset += sizeof(User);
    }

    close(user_fd);
    
    if (count == 0) printf("No active sessions found.\n");
    else printf("Successfully cleared %d active session(s).\n", count);
    
    return 0;
}