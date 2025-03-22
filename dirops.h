#ifndef DIROPS_H
#define DIROPS_H

// Create a new directory with the given name
int create_directory(const char *dirname);

// List contents of a directory
int list_directory(const char *dirname);

// List files with a specific extension in a directory
int list_files_by_extension(const char *dirname, const char *extension);

// Delete an empty directory
int delete_directory(const char *dirname);

#endif // DIROPS_H