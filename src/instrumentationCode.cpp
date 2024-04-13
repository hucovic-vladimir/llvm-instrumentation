#include <unordered_map>
#include <iostream>


static std::unordered_map<std::string, unsigned long> basicBlockExecutionsCounts;

extern "C" void __bb_enter(char* bbName) {
		std::string bbNameStr(bbName);
		if (basicBlockExecutionsCounts.find(bbNameStr) == basicBlockExecutionsCounts.end()) {
				basicBlockExecutionsCounts[bbNameStr] = 1;
		} else {
				basicBlockExecutionsCounts[bbNameStr]++;
		}
}

extern "C" void __prof_export() {
	for (auto it = basicBlockExecutionsCounts.begin(); it != basicBlockExecutionsCounts.end(); ++it) {
		std::cout << it->first << "," << it->second << std::endl;
	}
}
