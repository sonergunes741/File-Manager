#ifndef LOGGING_H
#define LOGGING_H

// Initialize the logging system
int init_logging(void);

// Log an operation with timestamp
int log_operation(const char *message);

// Display all logs
int show_logs(void);

#endif // LOGGING_H