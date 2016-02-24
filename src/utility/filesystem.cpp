#ifndef MNEMOSYNE_UTILITY_FILESYSTEM
#define MNEMOSYNE_UTILITY_FILESYSTEM

#include <fstream>
#include <inttypes.h>

inline uint64_t size_of_file(std::istream& is){
	uint64_t last = is.tellg();
	uint64_t size = 0;
	
	is.seekg (0, is.end);
	size = is.tellg();
	is.seekg(last);
	
	return size;
}

#endif
