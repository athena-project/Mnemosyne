#include "BinIndex.h"


///Begin BinBlock
BinBlock::BinBlock(const char* _path, const char* data, uint64_t _size){
    name =  BinBlock::alpha_id++;
    path = _path;
    init();
    size = _size;
    
    if( size > 0){
        buffer = new char[ size * DIGEST_LENGTH];
        memcpy( buffer, data, size * DIGEST_LENGTH);
        
        memcpy(id, data+(size-1)*DIGEST_LENGTH, DIGEST_LENGTH);
    }
}

bool BinBlock::load(bool degraded_mod){
    file = fopen(location.c_str(), "rb"); 
    if( file == NULL)
        return false;
        
    size_t filesize = size_of_file( file);
    size = filesize / DIGEST_LENGTH;
    buffer = new char[ size * DIGEST_LENGTH];   
     
    int f = fread(buffer, 1, size * DIGEST_LENGTH, file);
    if( f == -1 ){
        clean();
        return false;
    }

    char* end = buffer + uint64_s;
    
    if( degraded_mod && size > 0)
        memcpy(id, buffer + (size-1) * DIGEST_LENGTH, DIGEST_LENGTH);

    fclose( file );
    file = NULL;
    
    return true;
}       
  
void BinBlock::merge(BinBlock* right){
    const char* right_buffer = right->get_buffer();
    ///delete duplicated digets
    for( size_t k = right->get_size()-1; k >= 0 ; k--){
        if( exists( right_buffer + k * DIGEST_LENGTH ) )
            remove( right_buffer + k * DIGEST_LENGTH );
    }
    
    if( size == 0 || memcmp(right->get_id(), id, DIGEST_LENGTH) > 0 ) 
        memcpy(id, right->get_id(), DIGEST_LENGTH); 

    bool flag = right->get_size() + size > log( size ) * right->get_size();
    size_t end_left = size-1;
    size_t begin_end = size + right->get_size()-1;
    buffer = static_cast<char*>(realloc( static_cast<void*>(buffer), size + right->get_size()));
    size = size + right->get_size();
    
    
    for( size_t k = right->get_size()-1; k >= 0 ; k--){
        size_t pos = end_left;
        if( flag )//insertion
            pos = get_pos_i(right_buffer + k * DIGEST_LENGTH, get_pos_s(right_buffer + k * DIGEST_LENGTH));
        else{//fusion
            while( memcmp( buffer + pos * DIGEST_LENGTH, right_buffer+ k * DIGEST_LENGTH, DIGEST_LENGTH) > 0)
                pos--;
        }
            
        size_t number = end_left - pos +1;        
        memmove(buffer + (begin_end - number) * DIGEST_LENGTH, buffer + pos * DIGEST_LENGTH, number * DIGEST_LENGTH);
        end_left -= number;
        begin_end -= number;
        
        memcpy(buffer+pos, right_buffer + k * DIGEST_LENGTH, DIGEST_LENGTH);
    }
}


/// Begin BinNode
bool BinNode::add_bin(BinBlock* bin, const char* digest, LRU* cache){
    if( (size_b+size_c) == 0 ||  memcmp( digest, id, DIGEST_LENGTH) > 0 )
        memcpy( id, digest, DIGEST_LENGTH);
    
    if( leaf ){
        int pos_b = get_block_pos_s( digest );            
        
        if( size_b > 0 && memcmp(blocks[pos_b]->get_id(), digest, DIGEST_LENGTH) == 0){
            cache->add( blocks[pos_b] );
            cache->add( bin );
            
            blocks[ pos_b ]->merge( bin );
        }else{
            if( size_b != 0)
                memmove(&blocks[pos_b+1], &blocks[pos_b], (size_b-pos_b) * sizeof(Block*)); 
                
            blocks[pos_b] = new BinBlock( path, bin->get_buffer(), bin->get_size());
            size_b++;
        }
        
        return true;
    }else{
        int pos_c = get_child_pos_s( digest );

        BinNode* current_c = children[ pos_c ];
        if( !current_c->add_bin( bin, digest, cache) )
            return false;
            
        if( current_c->is_full()){
                BinNode* right = static_cast<BinNode*>(current_c->split());

            if( size_c != 0 && pos_c!=2*d)
                memmove(&children[pos_c+2], &children[pos_c+1], (size_c-pos_c-1) * sizeof(BNode*)); //+1 tjs garantie par la gestion de is_full

            children[pos_c+1] = right;
            size_c++;
        }
        return true;
    }
}

BinBlock* BinNode::get_bin(const char* digest, LRU* cache){
    if( !leaf ){
        if( size_c == 0 )
            return NULL;

        return children[ get_child_pos_s(digest) ]->get_bin( digest, cache);
    }else{
        if( size_b == 0 )
            return NULL;

        BinBlock* current = blocks[ get_block_pos_s(digest) ];
        cache->add( current );
        
        return current;
    }
}

/// Begin BinTree
bool BinTree::add_bin( BinBlock* bin){
    bool flag = static_cast<BinNode*>(root)->add_bin( bin, bin->get_id(), cache);
    
    if( root->is_full() ){
        BinNode* right = static_cast<BinNode*>(root->split());
        BinNode* left = static_cast<BinNode*>(root);
        root = new BinNode(path.c_str(), left, right);
    }

    return flag;
}

BinBlock* BinTree::get_bin( const char* digest){
    return static_cast<BinNode*>(root)->get_bin(digest, cache);
}
