#ifndef FILEOPS_H
#define FILEOPS_H

// Create a new file with the given name
int create_file(const char *filename);

// Read contents of a file
int read_file(const char *filename);

// Append content to a file with proper locking
int append_to_file(const char *filename, const char *content);

// Delete a file
int delete_file(const char *filename);

#endif // FILEOPS_H