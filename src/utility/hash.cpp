#ifndef MNEMOSYNE_UTILITY_HASH
#define MNEMOSYNE_UTILITY_HASH

#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>

#include <openssl/sha.h>

#define FILE_BUFFER 65536
///unsigned char* to char*
#define DIGEST_LENGTH (SHA224_DIGEST_LENGTH * 2)



inline void print_sha_sum(const unsigned char* md) {
    for(int i=0; i <SHA224_DIGEST_LENGTH; i++) {
        printf("%02x",md[i]);
    }
    printf("\n");
}

inline void digest_to_char(char* buffer, const unsigned char* digest){  
    for(int i=0; i <SHA224_DIGEST_LENGTH; i++)
        sprintf(buffer+2*i, "%02x", digest[i]);
}   

///on considere feja onversion avec %x0 etc
inline std::string digest_to_string(const char* digest){
    return std::string( digest, DIGEST_LENGTH);
}

inline bool hashfile(const char* filename, char* buffer){
    FILE *inFile = fopen(filename, "rb");
    SHA256_CTX context;
    int bytes;
    unsigned char data[FILE_BUFFER];
    unsigned char digest[SHA224_DIGEST_LENGTH];

    if (inFile == NULL) {
        printf("%s can't be opened.\n", filename);
        return false;
    }

    SHA224_Init (&context);
    while ((bytes = fread (data, 1, FILE_BUFFER, inFile)) != 0)
        SHA224_Update (&context, data, bytes);
    SHA224_Final (digest,&context);
    
    fclose (inFile);    
    digest_to_char(buffer, digest);
    
    return true;
}

#endif
