#define _GNU_SOURCE
#include "fileops.h"
#include "logging.h"
#include "utils.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/**
 * Create a new file with the given name
 * 
 * @param filename Name of the file to create
 * @return 0 on success, -1 on failure
 */
int create_file(const char *filename) {
    // Check if file already exists
    if (file_exists(filename)) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: File \"%s\" already exists.\n", filename);
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Create the file with read/write permissions for owner, read for group and others
    int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not create file \"%s\": %s\n", 
                          filename, strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Get current timestamp
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    
    // Write timestamp to the file
    if (write(fd, timestamp, strlen(timestamp)) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not write to file \"%s\": %s\n", 
                          filename, strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        close(fd);
        return -1;
    }
    
    // Close the file
    if (close(fd) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not close file \"%s\": %s\n", 
                          filename, strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Log the operation
    char log_msg[256];
    string_format(log_msg, sizeof(log_msg), "File \"%s\" created successfully.", filename);
    log_operation(log_msg);
    
    // Print success message
    char success_msg[256];
    int len = string_format(success_msg, sizeof(success_msg), 
                      "File \"%s\" created successfully.\n", filename);
    write(STDOUT_FILENO, success_msg, len);
    
    return 0;
}

/**
 * Read contents of a file
 * 
 * @param filename Name of the file to read
 * @return 0 on success, -1 on failure
 */
int read_file(const char *filename) {
    // Check if file exists
    if (!file_exists(filename)) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: File \"%s\" not found.\n", filename);
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Open the file for reading
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not open file \"%s\": %s\n", 
                          filename, strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not get file size for \"%s\": %s\n", 
                          filename, strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        close(fd);
        return -1;
    }
    
    // Allocate buffer for file contents
    char *buffer = (char *)malloc(st.st_size + 1);
    if (!buffer) {
        char error_msg[] = "Error: Memory allocation failed\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        close(fd);
        return -1;
    }
    
    // Read file contents
    ssize_t bytes_read = read(fd, buffer, st.st_size);
    if (bytes_read == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not read file \"%s\": %s\n", 
                          filename, strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        free(buffer);
        close(fd);
        return -1;
    }
    
    // Null-terminate the buffer
    buffer[bytes_read] = '\0';
    
    // Close the file
    if (close(fd) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not close file \"%s\": %s\n", 
                          filename, strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        free(buffer);
        return -1;
    }
    
    // Log the operation
    char log_msg[256];
    string_format(log_msg, sizeof(log_msg), "File \"%s\" read successfully.", filename);
    log_operation(log_msg);
    
    // Print file contents
    char header[256];
    int len = string_format(header, sizeof(header), "Contents of \"%s\":\n", filename);
    write(STDOUT_FILENO, header, len);
    write(STDOUT_FILENO, buffer, bytes_read);
    
    // Add a newline if the file doesn't end with one
    if (bytes_read > 0 && buffer[bytes_read - 1] != '\n') {
        write(STDOUT_FILENO, "\n", 1);
    }
    
    // Free allocated memory
    free(buffer);
    
    return 0;
}

/**
 * Append content to a file
 * 
 * @param filename The name of the file
 * @param content The content to append
 * @return 0 on success, -1 on error
 */
int append_to_file(const char *filename, const char *content) {
    // Check if the file exists
    if (!file_exists(filename)) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                         "Error: File \"%s\" does not exist\n", filename);
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Fork a child process to perform the operation
    pid_t pid = fork();
    
    if (pid == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                         "Error: Could not fork process: %s\n", strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    else if (pid == 0) {
        // Child process
        
        // Open file for writing (append mode)
        int fd = open(filename, O_RDWR | O_APPEND);
        if (fd == -1) {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                             "Error: Could not open file \"%s\": %s\n", 
                             filename, strerror(errno));
            write(STDERR_FILENO, error_msg, len);
            _exit(EXIT_FAILURE);
        }
        
        // Try to acquire an exclusive lock (non-blocking)
        if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
            // If we couldn't get the lock because it's already locked
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                char error_msg[256];
                int len = string_format(error_msg, sizeof(error_msg), 
                                 "Error: Cannot write to \"%s\". File is locked or read-only.\n", 
                                 filename);
                write(STDERR_FILENO, error_msg, len);
                close(fd);
                _exit(EXIT_FAILURE);
            } else {
                // Some other error occurred
                char error_msg[256];
                int len = string_format(error_msg, sizeof(error_msg), 
                                 "Error: Could not lock file \"%s\": %s\n", 
                                 filename, strerror(errno));
                write(STDERR_FILENO, error_msg, len);
                close(fd);
                _exit(EXIT_FAILURE);
            }
        }
        
        // Check if file is empty or if the last character is a newline
        struct stat st;
        fstat(fd, &st);
        
        // If file is not empty, check if we need to add a newline
        if (st.st_size > 0) {
            char last_char;
            if (lseek(fd, -1, SEEK_END) >= 0) {
                if (read(fd, &last_char, 1) == 1) {
                    // If the last character is not a newline, add one
                    if (last_char != '\n') {
                        const char newline = '\n';
                        write(fd, &newline, 1);
                    }
                }
            }
            // Move back to end of file
            lseek(fd, 0, SEEK_END);
        }
        
        // Write content to file
        size_t content_len = strlen(content);
        ssize_t bytes_written = write(fd, content, content_len);
        
        // Release the lock
        flock(fd, LOCK_UN);
        
        // Close the file
        close(fd);
        
        // Check if write was successful
        if (bytes_written != (ssize_t)content_len) {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                             "Error: Could not write to file \"%s\": %s\n", 
                             filename, strerror(errno));
            write(STDERR_FILENO, error_msg, len);
            _exit(EXIT_FAILURE);
        }
        
        // Log the operation
        char log_msg[256];
        string_format(log_msg, sizeof(log_msg), "Content appended to file \"%s\".", filename);
        log_operation(log_msg);
        
        // Print success message
        char success_msg[256];
        int len = string_format(success_msg, sizeof(success_msg), 
                          "Content appended to file \"%s\" successfully.\n", filename);
        write(STDOUT_FILENO, success_msg, len);
        
        _exit(EXIT_SUCCESS);
    }
    else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            return (WEXITSTATUS(status) == EXIT_SUCCESS) ? 0 : -1;
        } else {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                             "Error: Child process terminated abnormally\n");
            write(STDERR_FILENO, error_msg, len);
            return -1;
        }
    }
}
/**
 * Delete a file
 * 
 * @param filename Name of the file to delete
 * @return 0 on success, -1 on failure
 */
int delete_file(const char *filename) {
    // Check if file exists
    if (!file_exists(filename)) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: File \"%s\" not found.\n", filename);
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
        if (unlink(filename) == -1) {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                              "Error: Could not delete file \"%s\": %s\n", 
                              filename, strerror(errno));
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
            string_format(log_msg, sizeof(log_msg), "File \"%s\" deleted successfully.", filename);
            log_operation(log_msg);
            
            // Print success message
            char success_msg[256];
            int len = string_format(success_msg, sizeof(success_msg), 
                              "File \"%s\" deleted successfully.\n", filename);
            write(STDOUT_FILENO, success_msg, len);
            
            return 0;
        } else {
            return -1;
        }
    }
}