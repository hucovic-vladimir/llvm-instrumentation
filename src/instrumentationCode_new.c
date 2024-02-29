/// TODO rename the file

#include <unistd.h>
#include <alloca.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>

static FILE* out = NULL;

/// @brief can be used in case some runtime initialization is needed before profiling starts 
void __prof_init() {
	return;
}

/// @brief exports the basic block execution counts to a file
/// @param moduleName the original (source) name of the module the basic blocks belong to
/// @param arr the array containing the basic block execution counts
/// @param len the length of the array
/// @note only basic blocks with non-zero execution counts are exported
void __export_array(const char* moduleName, unsigned long* arr, unsigned long len) {
	fprintf(out, "%s\n-----------------\n", moduleName);
	for (unsigned long i = 0; i < len; i++) { 
		if(arr[i] > 0) {
			fprintf(out, "%lu:%lu\n", i, arr[i]);
		}
	}
}

/// @brief exports the basic block execution counts to a file
/// @note calls to __export_array are inserted by the post-instrumentation pass which runs on this module
void __prof_export2() {
	out = fopen("profile_data.txt", "w");
	if(out == NULL) {
		fprintf(stderr, "Profiling error: Failed to open profile_data.txt. Dumping to stderr\n");
		out = stderr;
	}
	return;
}
