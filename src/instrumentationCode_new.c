/// TODO rename the file

#include <unistd.h>
#include <alloca.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>

#define __MAX_DIGITS 21

/// @brief the number of basic blocks in the profiled program
/// @note the macro is defined in the build system
#define BBCOUNT_VALID BBCOUNT > 0
#if !defined(BBCOUNT) || !BBCOUNT_VALID
#error "BBCOUNT not defined or invalid"
#endif

/// @brief the number of basic blocks in the profiled program
unsigned long __basicblockCount = BBCOUNT;

/// @brief the array that holds the execution count of each basic block
/// The array is declared as volatile since a performance benefit has been observed on the ccsds project.
/// It is possible that the compiler generates instructions that completely bypass cache for volatile variables.
/// However, this should be double checked and the impact should be properly measured.
unsigned long __basicblocks[BBCOUNT];

/// @brief formats an unsigned long value as a string
/// @param value the value to be formatted
/// @param the address where result the result of the formatting is written
/// @note is used instead of printf for performance reasons, but the impact should be measured further
void __format_ul(unsigned long value, char* result) {
	if (value == 0) {
		result[0] = '0';
		result[1] = '\0';
		return;
	}

	unsigned long temp = value;
	int digits = 0;
	while (temp > 0) {
		digits++;
		temp /= 10;
	}

	result[digits] = '\0'; 

	int i = digits - 1;
	while (value > 0) {
		result[i] = (value % 10) + '0'; 
		value /= 10; 
		i--; 
	}
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
		/// unlikely assumes that most of the basic blocks are not executed
		if(__basicblocks[i] > 0) [[clang::unlikely]] {
			char bbId[__MAX_DIGITS] = {'\0'};
			char bbCount[__MAX_DIGITS] = {'\0'};
			char result[2 * __MAX_DIGITS + 2] = {'\0'};
			__format_ul(i, bbId);
			__format_ul(__basicblocks[i], bbCount);
			strcat(result, bbId);
			strcat(result, ":");
			strcat(result, bbCount);
			strcat(result, "\n");
			fwrite(result, sizeof(char), strlen(result), file);
			/* fwrite(bbId, strlen(bbId), 1, file); */
			/* fwrite(" : ", sizeof(char), 3, file); */
			/* fwrite(bbCount, sizeof(char), strlen(bbCount), file); */
			/* fwrite("\n", sizeof(char), 1, file); */
		}
	}
}
