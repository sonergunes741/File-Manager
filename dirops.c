#define _GNU_SOURCE
#include "dirops.h"
#include "logging.h"
#include "utils.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/**
 * Create a new directory with the given name
 * 
 * @param dirname Name of the directory to create
 * @return 0 on success, -1 on failure
 */
int create_directory(const char *dirname) {
    // Check if directory already exists
    if (directory_exists(dirname)) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Directory \"%s\" already exists.\n", dirname);
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Create directory with read/write/execute permissions for owner, read/execute for others
    if (mkdir(dirname, 0755) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not create directory \"%s\": %s\n", 
                          dirname, strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Log the operation
    char log_msg[256];
    string_format(log_msg, sizeof(log_msg), "Directory \"%s\" created successfully.", dirname);
    log_operation(log_msg);
    
    // Print success message
    char success_msg[256];
    int len = string_format(success_msg, sizeof(success_msg), 
                      "Directory \"%s\" created successfully.\n", dirname);
    write(STDOUT_FILENO, success_msg, len);
    
    return 0;
}

/**
 * List contents of a directory
 * This operation is performed in a child process
 * 
 * @param dirname Name of the directory to list
 * @return 0 on success, -1 on failure
 */
int list_directory(const char *dirname) {
    // Check if directory exists
    if (!directory_exists(dirname)) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Directory \"%s\" not found.\n", dirname);
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Create a child process to perform the listing
    pid_t pid = fork();
    
    if (pid == -1) {
        // Fork failed
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not create process: %s\n", strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    } 
    else if (pid == 0) {
        // Child process
        DIR *dir;
        struct dirent *entry;
        
        // Open the directory
        dir = opendir(dirname);
        if (!dir) {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                              "Error: Could not open directory \"%s\": %s\n", 
                              dirname, strerror(errno));
            write(STDERR_FILENO, error_msg, len);
            _exit(EXIT_FAILURE);
        }
        
        // Print directory contents header
        char header[256];
        int header_len = string_format(header, sizeof(header), 
                                 "Contents of directory \"%s\":\n", dirname);
        write(STDOUT_FILENO, header, header_len);
        
        // Read directory entries
        int count = 0;
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".." entries
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            // Determine if entry is a directory
            char path[512];
            string_format(path, sizeof(path), "%s/%s", dirname, entry->d_name);
            
            struct stat st;
            if (stat(path, &st) == -1) {
                continue; // Skip if we can't get information
            }
            
            // Prepare entry string with directory indicator
            char entry_str[512];
            int entry_len;
            if (S_ISDIR(st.st_mode)) {
                entry_len = string_format(entry_str, sizeof(entry_str), "  [DIR] %s\n", entry->d_name);
            } else {
                entry_len = string_format(entry_str, sizeof(entry_str), "  %s\n", entry->d_name);
            }
            
            // Write entry to stdout
            write(STDOUT_FILENO, entry_str, entry_len);
            count++;
        }
        
        // Close directory
        closedir(dir);
        
        // If no entries found, print a message
        if (count == 0) {
            const char *empty_msg = "  (empty directory)\n";
            write(STDOUT_FILENO, empty_msg, strlen(empty_msg));
        }
        
        // Exit successfully
        _exit(EXIT_SUCCESS);
    } 
    else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            // Log the operation
            char log_msg[256];
            string_format(log_msg, sizeof(log_msg), "Listed contents of directory \"%s\".", dirname);
            log_operation(log_msg);
            
            return 0;
        } else {
            return -1;
        }
    }
}

/**
 * List files with a specific extension in a directory
 * This operation is performed in a child process
 * 
 * @param dirname Name of the directory to list
 * @param extension File extension to filter by (e.g., ".txt")
 * @return 0 on success, -1 on failure
 */
int list_files_by_extension(const char *dirname, const char *extension) {
    // Check if directory exists
    if (!directory_exists(dirname)) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Directory \"%s\" not found.\n", dirname);
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Create a child process to perform the listing
    pid_t pid = fork();
    
    if (pid == -1) {
        // Fork failed
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not create process: %s\n", strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    } 
    else if (pid == 0) {
        // Child process
        DIR *dir;
        struct dirent *entry;
        
        // Open the directory
        dir = opendir(dirname);
        if (!dir) {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                              "Error: Could not open directory \"%s\": %s\n", 
                              dirname, strerror(errno));
            write(STDERR_FILENO, error_msg, len);
            _exit(EXIT_FAILURE);
        }
        
        // Print directory contents header
        char header[256];
        int header_len = string_format(header, sizeof(header), 
                                 "Files with extension \"%s\" in directory \"%s\":\n", 
                                 extension, dirname);
        write(STDOUT_FILENO, header, header_len);
        
        // Read directory entries
        int count = 0;
        size_t ext_len = strlen(extension);
        
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".." entries
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            // Get filename length
            size_t name_len = strlen(entry->d_name);
            
            // Check if file has the specified extension
            if (name_len > ext_len && 
                strcmp(entry->d_name + name_len - ext_len, extension) == 0) {
                    
                // Prepare full path for stat
                char path[512];
                string_format(path, sizeof(path), "%s/%s", dirname, entry->d_name);
                
                struct stat st;
                if (stat(path, &st) == -1) {
                    continue; // Skip if we can't get information
                }
                
                // Only list regular files, not directories
                if (S_ISREG(st.st_mode)) {
                    // Prepare entry string
                    char entry_str[512];
                    int entry_len = string_format(entry_str, sizeof(entry_str), 
                                            "  %s\n", entry->d_name);
                    
                    // Write entry to stdout
                    write(STDOUT_FILENO, entry_str, entry_len);
                    count++;
                }
            }
        }
        
        // Close directory
        closedir(dir);
        
        // If no matching files found, print a message
        if (count == 0) {
            char no_files_msg[256];
            int msg_len = string_format(no_files_msg, sizeof(no_files_msg), 
                                  "No files with extension \"%s\" found in \"%s\".\n", 
                                  extension, dirname);
            write(STDOUT_FILENO, no_files_msg, msg_len);
        }
        
        // Exit successfully
        _exit(EXIT_SUCCESS);
    } 
    else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            // Log the operation
            char log_msg[256];
            string_format(log_msg, sizeof(log_msg), 
                    "Listed files with extension \"%s\" in directory \"%s\".", 
                    extension, dirname);
            log_operation(log_msg);
            
            return 0;
        } else {
            return -1;
        }
    }
}

/**
 * Delete an empty directory
 * 
 * @param dirname Name of the directory to delete
 * @return 0 on success, -1 on failure
 */
int delete_directory(const char *dirname) {
    // Check if directory exists
    if (!directory_exists(dirname)) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Directory \"%s\" not found.\n", dirname);
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Create a child process to perform the deletion
    pid_t pid = fork();
    
    if (pid == -1) {
        // Fork failed
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not create process: %s\n", strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    } 
    else if (pid == 0) {
        // Child process
        
        // Check if directory is empty
        DIR *dir = opendir(dirname);
        if (!dir) {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                              "Error: Could not open directory \"%s\": %s\n", 
                              dirname, strerror(errno));
            write(STDERR_FILENO, error_msg, len);
            _exit(EXIT_FAILURE);
        }
        
        struct dirent *entry;
        int is_empty = 1;
        
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".." entries
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                is_empty = 0;
                break;
            }
        }
        
        closedir(dir);
        
        if (!is_empty) {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                              "Error: Directory \"%s\" is not empty.\n", dirname);
            write(STDERR_FILENO, error_msg, len);
            _exit(EXIT_FAILURE);
        }
        
        // Delete the directory
        if (rmdir(dirname) == -1) {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                              "Error: Could not delete directory \"%s\": %s\n", 
                              dirname, strerror(errno));
            write(STDERR_FILENO, error_msg, len);
            _exit(EXIT_FAILURE);
        }
        
        // Exit successfully
        _exit(EXIT_SUCCESS);
    } 
    else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            // Log the operation
            char log_msg[256];
            string_format(log_msg, sizeof(log_msg), "Directory \"%s\" deleted successfully.", dirname);
            log_operation(log_msg);
            
            // Print success message
            char success_msg[256];
            int len = string_format(success_msg, sizeof(success_msg), 
                              "Directory \"%s\" deleted successfully.\n", dirname);
            write(STDOUT_FILENO, success_msg, len);
            
            return 0;
        } else {
            return -1;
        }
    }
}