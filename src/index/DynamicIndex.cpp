#include "DynamicIndex.h"

/**
 *  Begin Block part
 */
int Block::alpha_id = 1;

Block::Block(const char* _path, char* data, uint64_t _size): name( Block::alpha_id++ ){
    path = _path;
    init();
    size = _size;
    
    if( size > 0){
        buffer = new char[ MAX_DIGESTS * DIGEST_LENGTH];
        memcpy( buffer, data, _size * DIGEST_LENGTH);
        
        memcpy(id, data+(size-1)*DIGEST_LENGTH, DIGEST_LENGTH);
    }
}

void Block::init(){
    char _name[sizeof(uint64_t)+1];
    _name[sizeof(uint64_t)]=0;
    
    sprintf(_name, "%" PRIu64 "", name);     
    location = (fs::path(path) / fs::path(_name)).string();
}

Block::~Block(){
    if( is_loaded() )
        store();
    clean();
}

void Block::clean(){
    if( buffer != NULL ){
        free(buffer);
        buffer = NULL;
    }
    
    if( file != NULL ){
        fclose( file );
        file = NULL;
    }
}

bool Block::is_full(){ return size >= MAX_DIGESTS-1; }
bool Block::is_loaded(){ return buffer != NULL; }
char* Block::get_id(){ return id; }

Block* Block::split(){
    uint64_t m = size / 2;
    char* mid = buffer + m * DIGEST_LENGTH;
    Block* right = new Block(path, mid, size-m);
    size = m;
    
    if( m > 0)
        memcpy(id, buffer + (m-1) * DIGEST_LENGTH, DIGEST_LENGTH);
    else
        memset(id, 0, DIGEST_LENGTH);
        
    return right;
}

bool Block::load(){
    buffer = new char[ MAX_DIGESTS * DIGEST_LENGTH];
    file = fopen(location.c_str(), "rb"); 
    if( file == NULL)
        return false;
        
    int f = fread(buffer, 1, MAX_DIGESTS * DIGEST_LENGTH, file);
    if( f == -1 ){
        clean();
        return false;
    }

    char* end = buffer + uint64_s;
    size = f / DIGEST_LENGTH;

    fclose( file );
    file = NULL;
    
    return true;
}       
    
bool Block::store(){
    file = fopen(location.c_str(), "wb");

    if( file == NULL)
        return false;

    if( fwrite(buffer, 1, size * DIGEST_LENGTH, file) == -1){
        clean();
        perror("failed");
        return false;
    }
    fclose( file );
    file = NULL;
    return true;
}

int Block::get_pos_s(char* digest){
    int a = 0;
    int b = size-1;
    int m = 0;
    int flag = 0;
    
    
    while( a < b ){
        m = (a + b) / 2;
        flag = memcmp(digest, buffer + m * DIGEST_LENGTH, DIGEST_LENGTH);

        if( flag < 0 )
            b = m-1;
        else if(flag > 0 )
            a = m+1;
        else
            return m;
    }

    return a;

}

int Block::get_pos_i(char* digest, int pos_s){
    if( size == 0)
        return 0;
        
    int flag = memcmp(digest, buffer + pos_s * DIGEST_LENGTH, DIGEST_LENGTH);

    if( flag <= 0)
        return pos_s;
    else 
        return pos_s+1;
}

bool Block::add(char* digest){
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

bool Block::exists(char* digest){
    int pos = get_pos_s( digest ) * DIGEST_LENGTH;
    if( memcmp(digest, buffer + pos, DIGEST_LENGTH) == 0 )
        return true;
    return false;
}

void Block::print(int step){
    if( step ==0)
    printf("\n\n*************************************************************\n");
    for(int i=0; i<step; i++)
        printf("    ");
    printf("Block %08X name=%d size=%d id:%s\n", reinterpret_cast<intptr_t>(this), name, size, id); 
    if( step ==0)
    printf("\n\n*************************************************************\n");
}


/**
 *  Begin LRU part
 */
 
LRU::~LRU(){}

void LRU::add(Block* item){
    items.push_front( item );

    unordered_map<Block*, decltype(items.begin())>::iterator it = items_map.find( item ); 
    if( it != items_map.end() ){
        items.erase( it->second );
        it->second = items.begin();
    }else if( size >= max_size ){
        items_map.erase( items.back() );
        items.back()->store();
        items.back()->clean();
        
        items.pop_back();
    }
    
    items_map[item] = items.begin();
    if( !item->is_loaded() )
        item->load();
}


/**
 *  Begin BNode part
 */
 
BNode::BNode(const char* _path, BNode* left, BNode* right){
    children[0] = left;
    children[1] = right;
    leaf = false;
    path = _path;
    
    size_c += 2;
}

BNode::BNode(const char* _path, BNode** data, uint64_t size){
    memcpy( &children[0], &data[0], size * sizeof(BNode*));
    
    size_c = size;
    leaf = false;
    path = _path;
    
    if( size > 0)
        memcpy(id, children[size_c-1]->get_id(), DIGEST_LENGTH);
}

BNode::BNode(const char* _path, Block** data, uint64_t size){
    memcpy( &blocks[0], &data[0], size * sizeof(Block*));
        
    size_b = size;
    path = _path;

    if( size > 0)
        memcpy(id, (blocks[size_b-1])->get_id(), DIGEST_LENGTH);
}

BNode::~BNode(){
    for(int i=0; i<size_c ; i++)
        delete children[ i ];
    
    for(int i=0; i<size_b ; i++)
        delete blocks[ i ];
    
    size_c = size_b = 0;
}

bool BNode::is_full(){ 
    return (size_b>= 2*d - 1) || (size_c>= 2*d - 1); }

char* BNode::get_id(){ return id; }

int BNode::get_child_pos_s(char* digest){
    int a = 0;
    int b = size_c-1;
    int m = 0;
    int flag = 0;
    
    
    while( a < b ){
        m = (a + b) / 2;
        flag = memcmp(digest, children[m]->get_id(), DIGEST_LENGTH );
        
        if( flag < 0 )
            b = m-1;
        else if(flag > 0 )
            a = m+1;
        else
            return m;
    }
    
    return a;     
}

int BNode::get_child_pos_i(char* digest, int pos_s){
    if( size_c == 0 )
        return 0;
    
    int a = pos_s;
    int flag = memcmp(digest, children[a]->get_id(), DIGEST_LENGTH);
    
    if( flag <= 0)
        return a;
    else 
        return a+1;       
}

int BNode::get_block_pos_s(char* digest){
    int a = 0;
    int b = size_b-1;
    int m = 0;
    int flag = 0;
    
    
    while( a < b ){
        m = (a + b) / 2;

        flag = memcmp(digest, blocks[m]->get_id(), DIGEST_LENGTH );
        
        if( flag < 0 )
            b = m-1;
        else if(flag > 0 )
            a = m+1;
        else
            return m;
    }
    
    return a;
}

int BNode::get_block_pos_i(char* digest, int pos_s){
    if( size_b == 0)
        return 0;
        
    int a = pos_s;
    int flag = memcmp(digest, blocks[a]->get_id(), DIGEST_LENGTH);
    
    if( flag <= 0)
        return a;
    else 
        return a+1;
}

BNode* BNode::split(){
    if( !leaf ){
        uint64_t m = size_c / 2;
        BNode* right = new BNode(path, &children[m], size_c-m);
        size_c = m;
        
        
        if( size_c > 0)
            memcpy(id, (children[size_c-1])->get_id(), DIGEST_LENGTH);
        else
            memset(id, 0, DIGEST_LENGTH);
            
        return right;
    }else{
        uint64_t m = size_b / 2;
        BNode* right = new BNode(path, &blocks[m], size_b-m);
        size_b = m;
        
        if( size_b > 0)
            memcpy(id, (blocks[size_b-1])->get_id(), DIGEST_LENGTH);
        else
            memset(id, 0, DIGEST_LENGTH);
            
        return right;
    }
}

bool BNode::add_digest(char* digest, LRU* cache){
    if( (size_b+size_c) == 0 ||  memcmp( digest, id, DIGEST_LENGTH) > 0 )
        memcpy( id, digest, DIGEST_LENGTH);
    
    if( leaf ){
        int pos_b = 0;
        Block* current_b = NULL;
        if( size_b > 0 ){
            pos_b = get_block_pos_s( digest );
            
            current_b = blocks[ pos_b ];
        }else{
            current_b = new Block( path );
            blocks[ pos_b ] = current_b;
            size_b++;
        }
        
        cache->add( current_b );

        if( !current_b->add( digest ) )
            return false;

        if( current_b->is_full() ){
            Block* right = current_b->split();
            
            cache->add( right );
            
            pos_b = get_block_pos_i(digest, pos_b);
            if( size_b != 0 && pos_b!=2*d-1)
                memmove(&blocks[pos_b+1], &blocks[pos_b], (size_b-pos_b) * sizeof(Block*)); //+1 tjs garantie par la gestion de is_full

            blocks[pos_b] = right;
            size_b++;
        }
        
        return true;
    }else{

        int pos_c = get_child_pos_s( digest );
        
        BNode* current_c = children[ pos_c ];
        if( !current_c->add_digest( digest, cache) )
            return false;

        if( current_c->is_full()){
            BNode* right = current_c->split();

            pos_c = get_child_pos_i(digest, pos_c);
            if( size_c != 0 && pos_c!=2*d-1)
                memmove(&children[pos_c+1], &children[pos_c], (size_c-pos_c) * sizeof(BNode*)); //+1 tjs garantie par la gestion de is_full

            children[pos_c] = right;
            size_c++;
        }
        return true;
    }
}

bool BNode::exists_digest(char* digest, LRU* cache){
    if( !leaf ){
        if( size_c == 0 )
            return 0;
        
        return children[ get_child_pos_s(digest) ]->exists_digest( digest, cache);
    }else{
        if( size_b == 0 )
            return false;
            
        Block* current = blocks[ get_block_pos_s(digest) ];
        cache->add( current );
        
        return current->exists(digest);
    }
}

void BNode::print(int step){
if( step ==0)
    printf("\n\n====================================================================\n");

    for(int i=0; i<step; i++)
        printf("    ");
    printf("BNode %08X size_b=%d size_c=%d leaf:%s id:%s\n", reinterpret_cast<intptr_t>(this), size_b, size_c, leaf ? "true" : "false", id); 
    for(int i=0; i<step; i++)
        printf("    ");
    printf("Children\n");
    for(int i=0; i<size_c; i++)
        children[i]->print(step+1);
    
    printf("Blocks\n");
    for(int i=0; i<size_b; i++)
        blocks[i]->print(step+1);
        if( step ==0)
    printf("====================================================================\n\n");
}


/**
 *  Begin BTree part
 */
 
BTree::~BTree(){
    if( root != NULL )
        delete root;
    if( cache != NULL )
        delete cache;
}

bool BTree::add_digest(char* digest){
    bool flag = root->add_digest( digest, cache );
    
    if( root->is_full() ){
        BNode* right = root->split();
        BNode* left = root;
        root = new BNode(path.c_str(), left, right);
    }
    
    return flag;
}

bool BTree::exists_digest(char* digest){
    return root->exists_digest( digest, cache );
}

void BTree::print(){
    root->print();
}
