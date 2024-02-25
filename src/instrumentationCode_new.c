/// TODO rename the file

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>


/// @brief the number of basic blocks in the profiled program
/// @note the macro is defined in the build system
#define BBCOUNT_VALID BBCOUNT > 0
#if !defined(BBCOUNT) || !BBCOUNT_VALID
	#error "BBCOUNT not defined or invalid"
#endif

/// @brief the number of basic blocks in the profiled program
static unsigned long __basicblockCount = BBCOUNT;

/// @brief the array that holds the execution count of each basic block
/// The array is declared as volatile since a performance benefit has been observed on the ccsds project.
/// It is possible that the compiler generates instructions that completely bypass cache for volatile variables.
/// However, this should be double checked and the impact should be properly measured.
static volatile unsigned long __basicblocks[BBCOUNT];

/// @brief formats an unsigned long value as a string
/// @param value the value to be formatted
/// @return the formatted string
/// @note is used instead of printf for performance reasons, but the impact should be measured further
char* __format_ul(unsigned long value) {
    if (value == 0) {
        char* result = (char*)malloc(2); 
        if (result) {
            result[0] = '0';
            result[1] = '\0';
        }
        return result;
    }
    
    unsigned long temp = value;
    int digits = 0;
    while (temp > 0) {
        digits++;
        temp /= 10;
    }

    char* result = (char*)malloc(digits + 1); 
    if (!result) {
        return NULL; 
    }
    result[digits] = '\0'; 
    
    int i = digits - 1;
    while (value > 0) {
        result[i] = (value % 10) + '0'; 
        value /= 10; 
        i--; 
    }

    return result;
}

/// @brief can be used in case some runtime initialization is needed before profiling starts 
void __prof_init() {
	return;
}

/// @brief increments the execution counter of basic block with the specified id
/// @param basicblockId the id of the basic block
void __bb_enter(unsigned long basicblockId) {
	if(basicblockId < __basicblockCount) [[clang::likely]] {
		__basicblocks[basicblockId]++;	
	}
	else [[clang::unlikely]] {
		FILE* err = fopen("profiling_errors.log", "w");
		fprintf(err, "Basic block id %lu is out of range\n", basicblockId);
		fclose(err);
	}
}

/// @todo make the file name configurable
/// @brief exports the profiling data to a file
void __prof_export() {
	FILE* file = fopen("profile_data.txt", "w");
  for (unsigned long i = 0; i < __basicblockCount; i++) {
		// TODO test an unlikely hint here - assumimg that most basic blocks 
		// are never executed
		if(__basicblocks[i] > 0) {
			/// TODO remove the allocation and use stack
			char* bbId = __format_ul(i);
			char* bbCount = __format_ul(__basicblocks[i]);
			if (bbId && bbCount) [[clang::likely]] {
				fwrite(bbId, strlen(bbId), 1, file);
				fwrite(" : ", sizeof(char), 3, file);
				fwrite(bbCount, sizeof(char), strlen(bbCount), file);
				fwrite("\n", sizeof(char), 1, file);
				free(bbId);
				free(bbCount);
			}
			else {
				FILE* err = fopen("profiling_errors.log", "w");
				fprintf(err, "Failed to allocate memory for basic block id %lu\n", i);
				fclose(err);
			}
		}
  }
}
