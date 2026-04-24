#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

// Function headers for helpers in txtest.c and txtiming_test.c
void write_all(int fd, const char *s);
void reset_file(const char *path, const char *contents);
void print_file(const char *tag, const char *path);
void child_print(const char *tag, const char *path);

#endif