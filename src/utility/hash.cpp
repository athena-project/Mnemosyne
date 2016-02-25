#ifndef MNEMOSYNE_UTILITY_HASH
#define MNEMOSYNE_UTILITY_HASH

#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>

#include <openssl/sha.h>

#define FILE_BUFFER 65536

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
    
    
    for(int i=0; i <SHA224_DIGEST_LENGTH; i++) {
		sprintf(buffer+i, "%02x", digest[i]);
	}
    
    return true;
}

inline void print_sha_sum(const unsigned char* md) {
    for(int i=0; i <SHA224_DIGEST_LENGTH; i++) {
        printf("%02x",md[i]);
    }
    printf("\n");
}


inline void digest_to_char(char* buffer, const unsigned char* digest){	
	for(int i=0; i <SHA224_DIGEST_LENGTH; i++)
		sprintf(buffer+i, "%02x", digest[i]);
}

inline std::string digest_to_string(const unsigned char* digest){
	char buffer[SHA224_DIGEST_LENGTH+1];
	buffer[SHA224_DIGEST_LENGTH] = 0;
	
	for(int i=0; i <SHA224_DIGEST_LENGTH; i++)
		sprintf(buffer+i, "%02x", digest[i]);

	return std::string( buffer );
}

///on considere feja onversion avec %x0 etc
inline std::string digest_to_string(const char* digest){
	char buffer[SHA224_DIGEST_LENGTH+1];
	buffer[SHA224_DIGEST_LENGTH] = 0;
	
	memcpy(buffer, digest, SHA224_DIGEST_LENGTH);

	return std::string( buffer );
}

#endif
