#define _GNU_SOURCE
#include "utils.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

/**
 * Get the current timestamp as a formatted string
 * Format: [YYYY-MM-DD HH:MM:SS]
 * 
 * @param buffer Buffer to store the timestamp
 * @param size Size of the buffer
 * @return Pointer to the buffer on success, NULL on failure
 */
char *get_timestamp(char *buffer, size_t size) {
    if (!buffer || size < 22) { // [YYYY-MM-DD HH:MM:SS] requires at least 22 bytes
        return NULL;
    }
    
    time_t now;
    struct tm *tm_info;
    
    // Get current time
    time(&now);
    tm_info = localtime(&now);
    
    if (!tm_info) {
        return NULL;
    }
    
    // Format timestamp
    int len = strftime(buffer, size, "[%Y-%m-%d %H:%M:%S]", tm_info);
    if (len == 0) {
        return NULL;
    }
    
    return buffer;
}

/**
 * Check if a file exists
 * 
 * @param filename Name of the file to check
 * @return 1 if file exists, 0 otherwise
 */
int file_exists(const char *filename) {
    struct stat st;
    
    // Check if file exists and is a regular file
    if (stat(filename, &st) == 0 && S_ISREG(st.st_mode)) {
        return 1;
    }
    
    return 0;
}

/**
 * Check if a directory exists
 * 
 * @param dirname Name of the directory to check
 * @return 1 if directory exists, 0 otherwise
 */
int directory_exists(const char *dirname) {
    struct stat st;
    
    // Check if directory exists and is a directory
    if (stat(dirname, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;
    }
    
    return 0;
}

/**
 * Custom string formatting function to avoid standard input&output dependency
 * Only supports a limited set of format specifiers: %s, %d
 * 
 * @param buffer Output buffer
 * @param size Buffer size
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of characters written (excluding null terminator)
 */
int string_format(char *buffer, size_t size, const char *format, ...) {
    if (!buffer || !format || size == 0) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    
    size_t buffer_pos = 0;
    
    for (size_t i = 0; format[i] != '\0' && buffer_pos < size - 1; i++) {
        if (format[i] == '%' && format[i + 1] != '\0') {
            switch (format[i + 1]) {
                case 's': {
                    // String
                    i++;  // Skip the 's'
                    const char *str = va_arg(args, const char *);
                    if (str == NULL) {
                        str = "(null)";
                    }
                    
                    size_t j = 0;
                    while (str[j] != '\0' && buffer_pos < size - 1) {
                        buffer[buffer_pos++] = str[j++];
                    }
                    break;
                }
                case 'd': {
                    // Integer
                    i++;  // Skip the 'd'
                    int value = va_arg(args, int);
                    
                    // Handle negative numbers
                    if (value < 0) {
                        buffer[buffer_pos++] = '-';
                        if (buffer_pos >= size - 1) break;
                        value = -value;
                    }
                    
                    // Special case for zero
                    if (value == 0) {
                        buffer[buffer_pos++] = '0';
                        break;
                    }
                    
                    // Convert integer to string
                    char temp[20]; // Enough for 64-bit integers
                    int temp_pos = 0;
                    
                    while (value > 0 && temp_pos < 20) {
                        temp[temp_pos++] = '0' + (value % 10);
                        value /= 10;
                    }
                    
                    // Reverse digits into buffer
                    while (temp_pos > 0 && buffer_pos < size - 1) {
                        buffer[buffer_pos++] = temp[--temp_pos];
                    }
                    break;
                }
                case '%': {
                    // Literal percent
                    i++;  // Skip the '%'
                    buffer[buffer_pos++] = '%';
                    break;
                }
                default: {
                    // Unsupported format specifier, just copy it
                    buffer[buffer_pos++] = format[i];
                    break;
                }
            }
        } else {
            buffer[buffer_pos++] = format[i];
        }
    }
    
    va_end(args);
    
    // Null-terminate the buffer
    buffer[buffer_pos] = '\0';
    
    return buffer_pos;
}

/**
 * Safely write a message to standard output
 * 
 * @param message Message to write
 */
void write_message(const char *message) {
    if (!message) {
        return;
    }
    
    // Get message length
    size_t len = strlen(message);
    
    // Write message to stdout
    ssize_t bytes_written = 0;
    size_t total_written = 0;
    
    while (total_written < len) {
        bytes_written = write(STDOUT_FILENO, 
                             message + total_written, 
                             len - total_written);
        
        if (bytes_written == -1) {
            // Error occurred
            if (errno == EINTR) {
                // Interrupted by signal, try again
                continue;
            } else {
                // Other error, abort
                break;
            }
        }
        
        total_written += bytes_written;
    }
}

/**
 * Safely write an error message to standard error
 * 
 * @param message Error message to write
 */
void write_error(const char *message) {
    if (!message) {
        return;
    }
    
    // Get message length
    size_t len = strlen(message);
    
    // Write message to stderr
    ssize_t bytes_written = 0;
    size_t total_written = 0;
    
    while (total_written < len) {
        bytes_written = write(STDERR_FILENO, 
                             message + total_written, 
                             len - total_written);
        
        if (bytes_written == -1) {
            // Error occurred
            if (errno == EINTR) {
                // Interrupted by signal, try again
                continue;
            } else {
                // Other error, abort
                break;
            }
        }
        
        total_written += bytes_written;
    }
}