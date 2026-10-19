-----

# CSE 513-A/Systems Software - Banking Management System

This is a multi-threaded, client-server banking system written entirely in C. It uses socket programming, POSIX threads, and `fcntl()` file locking to simulate the core functionalities of a bank while handling concurrent operations safely.

The server manages all data logic, and clients connect via a terminal interface to perform actions based on their role (Customer, Employee, Manager, or Admin).

## 🔧 Prerequisites

Before you begin, ensure you have the following installed on your system:

  * A Unix-like environment (Linux, macOS, or WSL on Windows).
  * `gcc` (the C compiler).
  * `make` (the build automation tool).

## ⚙️ Building the Project

1.  Open your terminal and navigate to the project directory.
2.  Run the `make` command:
    ```bash
    make
    // pateek@Prateek-Kumar Mini-Project % make
    ```
    
3.  This will compile and link all the necessary files. You will see three new executable files in your directory:
      * `server`
      * `client`
      * `setup`

To remove all compiled files and clean the `data` directory, you can run:

```bash
make clean
// pateek@Prateek-Kumar Mini-Project % make clean
```

## ▶️ Running the Application

You must follow these steps in the correct order. This requires at least **two separate terminal windows**.

### Step 1: Initialize the Database (One-Time Setup)

First, run the `setup` utility. This creates the `data/` directory and populates the `.dat` files with default users and accounts.

```bash
./setup
// pateek@Prateek-Kumar Mini-Project % ./setup
```

You only need to do this once.

### Step 2: Start the Server

In your **first terminal**, start the server. It will wait for incoming connections.

```bash
./server
// prateek@Prateek-Kumar Mini-Project % ./server
```

You should see a message: `Server listening on port 8080...`
This terminal must remain open as long as you want the bank to be running.

### Step 3: Run the Client

In a **second terminal**, start the client application.

```bash
./client
// prateek@Prateek-Kumar Mini-Project % ./client
```

The client will connect to the server, and you will be prompted to log in.

### Step 4: Test Concurrency

To test the session management and file locking, you can open a **third (or fourth) terminal** and run `./client` in each one. Try to log in as the same user from two clients, or try to deposit and withdraw from the same account at the same time.

## 🔑 Default Login Credentials

The `setup` utility creates the following users:

| Role | Username | Password |
| :--- | :--- | :--- |
| 🧑‍💼 **Administrator** | `admin` | `admin123` |
| 👔 **Manager** | `manager` | `manager123` |
| 👨‍💻 **Employee** | `emp` | `emp123` |
| 🙍 **Customer 1** | `cust1` | `cust123` |
| 🙍 **Customer 2** | `cust2` | `cust123` |

## 📁 Project File Structure

```
.
├── server             # (Executable) The server
├── client             # (Executable) The client
├── setup              # (Executable) The data initializer
│
├── server.c           # Server source code
├── client.c           # Client source code
├── setup.c            # Setup utility source code
├── common.h           # Shared structs, headers, and constants
├── Makefile           # Build script
├── clear_sessions     # Clear active sessions
│
└── data/              # Directory for all data files
    ├── users.dat
    ├── accounts.dat
    ├── transactions.dat
    ├── loans.dat
    └── feedback.dat
```