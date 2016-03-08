#ifndef MNEMOSYNE_UTILITY_FILESYSTEM
#define MNEMOSYNE_UTILITY_FILESYSTEM

#include <fstream>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>

inline uint64_t size_of_file(std::istream& is){
    uint64_t last = is.tellg();
    uint64_t size = 0;
    
    is.seekg (0, is.end);
    size = is.tellg();
    is.seekg(last);
    
    return size;
}

inline uint64_t size_of_file(unsigned int fd){
    uint64_t tmp = lseek(fd, 0, SEEK_CUR);
    uint64_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, tmp, SEEK_SET);
    return size;
}

inline uint64_t size_of_file(const char* location){
    int fd = open(location, O_RDONLY);
    uint64_t tmp = lseek(fd, 0, SEEK_CUR);
    uint64_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, tmp, SEEK_SET);
    close(fd);
    return size;
}

#endif
