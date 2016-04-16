#import "BinBlock.h"

BinBlock::BinBlock(const char* _path, const char* data, uint64_t _size): name( Block::alpha_id++ ){
    path = _path;
    init();
    size = _size;
    
    if( size > 0){
        buffer = new char[ size * DIGEST_LENGTH];
        memcpy( buffer, data, _size * DIGEST_LENGTH);
        
        memcpy(id, data+(size-1)*DIGEST_LENGTH, DIGEST_LENGTH);
    }
}

bool BinBlock::add(const char** digests, size_t length){
    if( length < RATIO_INSERTION * length){
        for(int i=0; i<length; i++, digests+=DIGEST_LENGTH)
            if( !add( digests) )
                return false;
        return true;
    }
    
    int p = get_pos_s( digest );
    int pos = p * DIGEST_LENGTH;
    
    if( memcmp(digest, buffer + pos, DIGEST_LENGTH ) == 0 )
        return true;

    p = get_pos_i( digest, p );
    pos = p * DIGEST_LENGTH;
        
    if( size != 0 && p!=MAX_DIGESTS-1)
        memmove(buffer + pos + DIGEST_LENGTH, buffer + pos, (size-p) * DIGEST_LENGTH);

    memcpy(buffer + pos, digest, DIGEST_LENGTH);
    
    if( size == 0 || memcmp(digest, id, DIGEST_LENGTH) > 0 ) 
        memcpy(id, digest, DIGEST_LENGTH); 
    size++;

    return true;
}

