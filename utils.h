#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdarg.h>

// Get the current timestamp as a formatted string
// Format: [YYYY-MM-DD HH:MM:SS]
char *get_timestamp(char *buffer, size_t size);

// Check if a file exists
int file_exists(const char *filename);

// Check if a directory exists
int directory_exists(const char *dirname);

// Custom string formatting function to avoid standard input&output dependency
// Only supports a limited set of format specifiers: %s, %d
int string_format(char *buffer, size_t size, const char *format, ...);

// Safely write a message to standard output
void write_message(const char *message);

// Safely write an error message to standard error
void write_error(const char *message);

#endif // UTILS_H