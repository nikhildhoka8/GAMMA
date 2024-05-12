---

# GAMMA - Terminal Simulator

GAMMA is an advanced C++-based terminal simulator for Linux, designed to emulate complex shell functionalities directly from your terminal. This tool is especially useful for developers and system administrators who need to test or automate tasks in a Linux-like environment.

## Features

- **Command Execution**: Execute system and custom commands within the simulator with full support for environment variables.
- **Pipelining**: Chain multiple commands together using pipes (`|`), allowing the output of one command to serve as input to another.
- **Redirection**: Redirect input and output to and from files using `>`, `>>`, `<`.
- **Forking**: Utilizes process forking to handle command execution, ensuring isolation and security between processes.
- **Built-in Commands**: Supports a range of built-in commands like `cd`, `alias`, `set`, and more.
- **Command History**: Access and manage a history of executed commands, enhancing user experience in recalling past commands.
- **Environment Variable Management**: Set and unset environment variables that can influence the behavior of the simulator and commands.
- **Error Handling**: Robust error handling and reporting mechanism, with options to halt on error.
- **Wildcards and Globbing**: Supports wildcard characters like `*` and `?` for matching file names and patterns within command arguments.
- **Scripting and Batch Mode**: Execute scripts or enter batch mode for automated command processing.

## Concepts Demonstrated

- **Forking**: The simulator demonstrates the creation of new processes using `fork()`, which is fundamental for process management in Unix-like systems.
- **Command Pipelining**: Illustrates the use of Unix pipelines, allowing the output of one command to be used as input to another.
- **File Redirection**: Showcases redirection of input and output, fundamental for scripting and command automation.
- **Environment Handling**: Manages environment variables which are essential for the configuration of Unix-like operating systems.
- **Error Handling**: Implements robust error handling which is crucial for developing reliable software.

## Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/username/gamma.git
   ```
2. **Navigate to the gamma directory:**
   ```bash
   cd gamma
   ```

## Usage

### Compilation

To compile GAMMA, ensure you are in the project directory and then use the Makefile by running:

```bash
make
```

This command compiles the `main.cpp` file into an executable named `project4`, with appropriate compiler flags to support C++17 features and filesystem operations.

### Running the Simulator

After compilation, you can start the simulator by running:

```bash
make run
```

This command executes the `project4` binary, starting up the terminal simulator.

### Cleaning Up

To clean up the executable and other build files, run:

```bash
make clean
```

This command removes the `project4` executable, helping maintain a clean workspace.

## Documentation

For more detailed information about each command and its options, refer to the built-in help system by typing `help` followed by the command name within the simulator.

---
