/// TODO rename the file

#include <unistd.h>
#include <alloca.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>

static FILE* out = NULL;
static char export_file_name[150] = {0};

/// @brief can be used in case some runtime initialization is needed before profiling starts 
/// @note the instrumentation pass inserts this call at the beginning of the main function
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
	fprintf(out, "-----------------\n");
}


/// @brief exports the basic block execution counts to a file
/// @note the instrumentation pass inserts this call at the end of the main function and before each exit() call
/// @note calls to __export_array are inserted by the post-instrumentation pass which runs on this module
void __prof_export2() {
	unsigned long proc_id = getpid();
	snprintf(export_file_name, 150, "profile_data_pid_%lu.txt", proc_id);
	out = fopen(export_file_name, "w");
	if(out == NULL) {
		fprintf(stderr, "Profiling error: Failed to open %s. Dumping to stderr\n", export_file_name);
		out = stderr;
	}
	return;
}
