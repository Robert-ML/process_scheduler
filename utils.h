#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define handle_error(...)				\
	do { fprintf(stderr,				\
	##__VA_ARGS__); exit(EXIT_FAILURE); } while (0)

#endif /* UTILS_H */
