#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>

#include "fileops.h"
#include "dirops.h"
#include "logging.h"
#include "utils.h"

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 4
#define MAX_ARG_SIZE 256

// Global arguments array for signal handler access
static char *g_args[MAX_ARGS] = {NULL};

/**
 * Ensures all resources are freed at program exit
 */
void exit_handler(void) {
    // Free any allocated global arguments
    for (int i = 0; i < MAX_ARGS; i++) {
        if (g_args[i]) {
            free(g_args[i]);
            g_args[i] = NULL;
        }
    }
}

/**
 * Signal handler to ensure proper cleanup on exit
 */
void cleanup_handler(int signum) {
    // Free any allocated arguments
    for (int i = 0; i < MAX_ARGS; i++) {
        if (g_args[i]) {
            free(g_args[i]);
            g_args[i] = NULL;
        }
    }
    
    // Exit with the signal number
    _exit(signum);
}

/**
 * Display help information about available commands
 */
void display_help(void) {
    const char *help_text = 
        "Usage: fileManager <command> [arguments]\n\n"
        "Commands:\n\n"
        "  createDir \"folderName\" - Create a new directory\n\n"
        "  createFile \"fileName\" - Create a new file\n\n"
        "  listDir \"folderName\" - List all files in a directory\n\n"
        "  listFilesByExtension \"folderName\" \".txt\" - List files with specific extension\n\n"
        "  readFile \"fileName\" - Read a file's content\n\n"
        "  appendToFile \"fileName\" \"new content\" - Append content to a file\n\n"
        "  deleteFile \"fileName\" - Delete a file\n\n"
        "  deleteDir \"folderName\" - Delete an empty directory\n\n"
        "  showLogs - Display operation logs\n\n";
    
    write(STDOUT_FILENO, help_text, strlen(help_text));
}

/**
 * Free the memory allocated for arguments
 * 
 * @param args Array of argument pointers
 */
void free_args(char *args[MAX_ARGS]) {
    if (!args) return;
    
    for (int i = 0; i < MAX_ARGS; i++) {
        if (args[i]) {
            free(args[i]);
            args[i] = NULL;
        }
    }
}

/**
 * Sync global args array with local args array
 * Note: This makes g_args point to the same memory as args
 */
void sync_args(char *args[MAX_ARGS]) {
    for (int i = 0; i < MAX_ARGS; i++) {
        g_args[i] = args[i]; // No deep copy - just pointer assignment
    }
}

/**
 * Parse a command line into tokens
 * 
 * @param input Input string to parse
 * @param args Array to store parsed arguments
 * @return Number of arguments parsed, or -1 on error
 */
int parse_command(char *input, char *args[MAX_ARGS]) {
    int arg_count = 0;
    int in_quotes = 0;
    int i, j = 0;
    
    // Free any existing allocations in args
    free_args(args);
    
    // Allocate memory for the first argument
    args[0] = (char *)malloc(MAX_ARG_SIZE);
    if (!args[0]) {
        return -1; // Memory allocation failed
    }
    
    // Update global args for signal handler
    sync_args(args);
    
    // Parse input string
    for (i = 0; input[i] != '\0' && input[i] != '\n'; i++) {
        if (input[i] == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        
        if (input[i] == ' ' && !in_quotes) {
            if (j > 0) { // If we have collected characters
                args[arg_count][j] = '\0';
                arg_count++;
                
                if (arg_count >= MAX_ARGS) {
                    break; // Too many arguments
                }
                
                // Allocate memory for the next argument
                args[arg_count] = (char *)malloc(MAX_ARG_SIZE);
                if (!args[arg_count]) {
                    // Free previously allocated memory
                    free_args(args);
                    return -1; // Memory allocation failed
                }
                
                // Update global args for signal handler
                sync_args(args);
                
                j = 0;
            }
        } else {
            args[arg_count][j++] = input[i];
        }
    }
    
    // Set the last argument if there are characters
    if (j > 0) {
        args[arg_count][j] = '\0';
        arg_count++;
    } else if (arg_count > 0 && j == 0) {
        // Free the last allocated but unused memory
        free(args[arg_count]);
        args[arg_count] = NULL;
        g_args[arg_count] = NULL;
    }
    
    return arg_count;
}

/**
 * Execute a command with the given arguments
 * 
 * @param args Array of command arguments
 * @param arg_count Number of arguments
 * @return 0 on success, 1 to exit program, -1 on error
 */
int execute_command(char *args[], int arg_count) {
    if (arg_count == 0) {
        return 0;
    }
    
    // Check the command
    if (strcmp(args[0], "createDir") == 0) {
        if (arg_count != 2) {
            write(STDERR_FILENO, "Error: createDir requires one argument\n", 39);
            return -1;
        }
        return create_directory(args[1]);
    } 
    else if (strcmp(args[0], "createFile") == 0) {
        if (arg_count != 2) {
            write(STDERR_FILENO, "Error: createFile requires one argument\n", 40);
            return -1;
        }
        return create_file(args[1]);
    } 
    else if (strcmp(args[0], "listDir") == 0) {
        if (arg_count != 2) {
            write(STDERR_FILENO, "Error: listDir requires one argument\n", 37);
            return -1;
        }
        return list_directory(args[1]);
    } 
    else if (strcmp(args[0], "listFilesByExtension") == 0) {
        if (arg_count != 3) {
            write(STDERR_FILENO, "Error: listFilesByExtension requires two arguments\n", 51);
            return -1;
        }
        return list_files_by_extension(args[1], args[2]);
    } 
    else if (strcmp(args[0], "readFile") == 0) {
        if (arg_count != 2) {
            write(STDERR_FILENO, "Error: readFile requires one argument\n", 38);
            return -1;
        }
        return read_file(args[1]);
    } 
    else if (strcmp(args[0], "appendToFile") == 0) {
        if (arg_count != 3) {
            write(STDERR_FILENO, "Error: appendToFile requires two arguments\n", 43);
            return -1;
        }
        return append_to_file(args[1], args[2]);
    } 
    else if (strcmp(args[0], "deleteFile") == 0) {
        if (arg_count != 2) {
            write(STDERR_FILENO, "Error: deleteFile requires one argument\n", 40);
            return -1;
        }
        return delete_file(args[1]);
    } 
    else if (strcmp(args[0], "deleteDir") == 0) {
        if (arg_count != 2) {
            write(STDERR_FILENO, "Error: deleteDir requires one argument\n", 39);
            return -1;
        }
        return delete_directory(args[1]);
    } 
    else if (strcmp(args[0], "showLogs") == 0) {
        return show_logs();
    } 
    else if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
        return 1; // Special return value for exit command
    } 
    else if (strcmp(args[0], "help") == 0) {
        display_help();
        return 0;
    } 
    else {
        char error_msg[100];
        int len = string_format(error_msg, sizeof(error_msg), 
                               "Error: Unknown command '%s'\n", args[0]);
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
}

/**
 * Main function - program entry point
 */
int main(int argc, char *argv[]) {
    // Set up signal handlers for clean exit
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);
    
    // Register exit handler to clean up resources
    atexit(exit_handler);
    
    // Initialize logging
    init_logging();
    
    // If no arguments are provided, just show help and exit
    if (argc == 1) {
        display_help();
        return EXIT_SUCCESS;
    } 
    
    // Command-line mode
    char *args[MAX_ARGS] = {NULL}; // Initialize to NULL
    sync_args(args);  // Update global args
    
    int arg_count = argc - 1;
    
    // Check if we have too many arguments
    if (arg_count > MAX_ARGS) {
        write(STDERR_FILENO, "Error: Too many arguments\n", 26);
        return EXIT_FAILURE;
    }
    
    // Copy arguments from argv
    for (int i = 0; i < arg_count; i++) {
        args[i] = strdup(argv[i + 1]);
        if (!args[i]) {
            write(STDERR_FILENO, "Error: Memory allocation failed\n", 32);
            
            // Free already allocated memory
            free_args(args);
            sync_args(args);
            return EXIT_FAILURE;
        }
    }
    
    // Update global args for signal handler
    sync_args(args);
    
    // Execute the command
    int result = execute_command(args, arg_count);
    
    // Free allocated memory
    free_args(args);
    sync_args(args);
    
    return (result < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}