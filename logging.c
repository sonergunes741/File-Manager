#define _GNU_SOURCE
#include "logging.h"
#include "utils.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define LOG_FILE "log.txt"

/**
 * Initialize the logging system
 * Creates the log file if it doesn't exist
 * 
 * @return 0 on success, -1 on failure
 */
int init_logging(void) {
    // Check if log file exists, if not, create it
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not initialize logging: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Close the file
    if (close(fd) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not close log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    return 0;
}

/**
 * Log an operation with timestamp
 * 
 * @param message Message to log
 * @return 0 on success, -1 on failure
 */
int log_operation(const char *message) {
    // Open log file in append mode
    int fd = open(LOG_FILE, O_WRONLY | O_APPEND);
    if (fd == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not open log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Acquire an exclusive lock
    if (flock(fd, LOCK_EX) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not lock log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        close(fd);
        return -1;
    }
    
    // Get timestamp
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    
    // Format the log entry: [timestamp] message
    char log_entry[512];
    int entry_len = string_format(log_entry, sizeof(log_entry), 
                            "%s %s\n", timestamp, message);
    
    // Write log entry to file
    if (write(fd, log_entry, entry_len) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not write to log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        
        // Release the lock
        flock(fd, LOCK_UN);
        close(fd);
        return -1;
    }
    
    // Release the lock
    if (flock(fd, LOCK_UN) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not unlock log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        close(fd);
        return -1;
    }
    
    // Close the file
    if (close(fd) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not close log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    return 0;
}

/**
 * Display all logs
 * 
 * @return 0 on success, -1 on failure
 */
int show_logs(void) {
    // Check if log file exists
    struct stat st;
    if (stat(LOG_FILE, &st) == -1) {
        if (errno == ENOENT) {
            const char *no_logs_msg = "No logs available.\n";
            write(STDOUT_FILENO, no_logs_msg, strlen(no_logs_msg));
            return 0;
        } else {
            char error_msg[256];
            int len = string_format(error_msg, sizeof(error_msg), 
                              "Error: Could not access log file: %s\n", 
                              strerror(errno));
            write(STDERR_FILENO, error_msg, len);
            return -1;
        }
    }
    
    // Open log file for reading
    int fd = open(LOG_FILE, O_RDONLY);
    if (fd == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not open log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        return -1;
    }
    
    // Acquire a shared lock for reading
    if (flock(fd, LOCK_SH) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not lock log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        close(fd);
        return -1;
    }
    
    // Get file size
    if (fstat(fd, &st) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not get log file size: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        
        // Release the lock
        flock(fd, LOCK_UN);
        close(fd);
        return -1;
    }
    
    // Check if log file is empty
    if (st.st_size == 0) {
        const char *empty_log_msg = "Log file is empty.\n";
        write(STDOUT_FILENO, empty_log_msg, strlen(empty_log_msg));
        
        // Release the lock
        flock(fd, LOCK_UN);
        close(fd);
        return 0;
    }
    
    // Allocate buffer for file contents
    char *buffer = (char *)malloc(st.st_size + 1);
    if (!buffer) {
        char error_msg[] = "Error: Memory allocation failed\n";
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        
        // Release the lock
        flock(fd, LOCK_UN);
        close(fd);
        return -1;
    }
    
    // Read file contents
    ssize_t bytes_read = read(fd, buffer, st.st_size);
    if (bytes_read == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not read log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        
        // Clean up
        free(buffer);
        flock(fd, LOCK_UN);
        close(fd);
        return -1;
    }
    
    // Null-terminate the buffer
    buffer[bytes_read] = '\0';
    
    // Release the lock
    if (flock(fd, LOCK_UN) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not unlock log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        
        // Clean up
        free(buffer);
        close(fd);
        return -1;
    }
    
    // Close the file
    if (close(fd) == -1) {
        char error_msg[256];
        int len = string_format(error_msg, sizeof(error_msg), 
                          "Error: Could not close log file: %s\n", 
                          strerror(errno));
        write(STDERR_FILENO, error_msg, len);
        free(buffer);
        return -1;
    }
    
    // Display logs header
    const char *logs_header = "Operation Logs:\n";
    write(STDOUT_FILENO, logs_header, strlen(logs_header));
    
    // Display log contents
    write(STDOUT_FILENO, buffer, bytes_read);
    
    // Add a newline if the log doesn't end with one
    if (bytes_read > 0 && buffer[bytes_read - 1] != '\n') {
        write(STDOUT_FILENO, "\n", 1);
    }
    
    // Free allocated memory
    free(buffer);
    
    return 0;
}