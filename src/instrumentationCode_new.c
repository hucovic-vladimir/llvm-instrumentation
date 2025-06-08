/// TODO rename the file, add and update comments

#include <stdbool.h>
#include <unistd.h>
#include <alloca.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static FILE* out = NULL;
static char export_file_name[150] = {0};

/// @brief can be used in case some runtime initialization is needed before profiling starts 
/// @note the instrumentation pass inserts this call at the beginning of the main function
void __prof_init() {
	return;
}


void __export_array(const char* moduleName, unsigned long* arr, unsigned long len, bool lastModule);

void __export_modules() {
	return;
}

void __export_path_arrays() {
	return;
}

void __export_path_array(const char* moduleName, const char* funcName, unsigned long* arr, unsigned long len, bool lastModule);

static char format_module[] = 
"\
\t\t{\n\
\t\t\t\"name\":\"%s\",\n\
\t\t\t\"basicBlocks\": [\n\
";

static char format_block[] =
"\
\t\t\t\t{\n\
\t\t\t\t\t\"id\": %d,\n\
\t\t\t\t\t\"executionCount\": %lu\n\
\t\t\t\t}%s\
";

/// @brief exports the basic block execution counts to a json file
/// @param moduleName the original (source) name of the module the basic blocks belong to
/// @param arr the array containing the basic block execution counts
/// @param len the length of the array
/// @note only basic blocks with non-zero execution counts are exported
void __export_array(const char* moduleName, unsigned long* arr, unsigned long len, bool lastModule) {
    bool firstItemAdded = false;
    fprintf(out, format_module, moduleName);
    for (unsigned long i = 0; i < len; i++) {
        if (arr[i] > 0) {
            if (firstItemAdded) {
                fprintf(out, ",\n");
            }
            fprintf(out, format_block, i, arr[i], "");
            firstItemAdded = true;
        }
    }
    fprintf(out, "\n\t\t\t]\n\t\t}%s\n", lastModule ? "" : ",");
}

void __export_path_array(const char* moduleName, const char* funcName, unsigned long* arr, unsigned long len, bool lastModule) {
    fprintf(out, "\t\t{\n");
    fprintf(out, "\t\t\t\"mName\":\"%s\",\n", moduleName);
    fprintf(out, "\t\t\t\"functions\": [\n");
    fprintf(out, "\t\t\t\t{\n");
    fprintf(out, "\t\t\t\t\t\"fName\": \"%s\",\n", funcName);
    fprintf(out, "\t\t\t\t\t\"paths\": [\n");
    
    // Export all path values
    for (unsigned long i = 0; i < len; i++) {
        if (i > 0) {
            fprintf(out, ", ");
        }
        fprintf(out, "%lu", arr[i]);
    }
    fprintf(out, "\n\t\t\t\t\t]\n");
    fprintf(out, "\t\t\t\t}\n");
    fprintf(out, "\t\t\t]\n");
    fprintf(out, "\t\t}%s\n", lastModule ? "" : ",");
}

/// @brief exports the basic block execution counts to a file
/// @note the instrumentation pass inserts this call at the end of the main function and before each exit() call
/// @note calls to __export_array are inserted by the post-instrumentation pass which runs on this module
void __prof_export2() {
	unsigned long proc_id = getpid();
	snprintf(export_file_name, 150, "profile_data_pid_%lu.json", proc_id);
	out = fopen(export_file_name, "w");
	if(out == NULL) {
		fprintf(stderr, "Profiling error: Failed to open %s. Dumping to stderr\n", export_file_name);
		out = stderr;
	}
	fprintf(out, "{\n");
	fprintf(out, "\t\"modules\": [\n");
	__export_modules();
	fprintf(out, "\t]\n");
	fprintf(out, "}\n");
	return;
}

void __pathinst_export() {
	unsigned long proc_id = getpid();
	snprintf(export_file_name, 150, "paths_profile_pid_%lu.json", proc_id);
	out = fopen(export_file_name, "w");
	if(out == NULL) {
		fprintf(stderr, "Profiling error: Failed to open %s. Dumping to stderr\n", export_file_name);
		out = stderr;
	}
	fprintf(out, "{\n");
	fprintf(out, "\t\"modules\": [\n");
	__export_modules();
	fprintf(out, "\t]\n");
	fprintf(out, "}\n");
	return;
}


