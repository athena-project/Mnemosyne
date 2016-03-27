#include "DynamicIndex.h"

/**
 *  Begin Block part
 */
int Block::alpha_id = 1;

Block::Block(const char* _path, const char* data, uint64_t _size): name( Block::alpha_id++ ){
    path = _path;
    init();
    size = _size;
    
    if( size > 0){
        buffer = new char[ MAX_DIGESTS * DIGEST_LENGTH];
        memcpy( buffer, data, _size * DIGEST_LENGTH);
        
        memcpy(id, data+(size-1)*DIGEST_LENGTH, DIGEST_LENGTH);
    }
}

Block::~Block(){
    if( is_loaded() && size > 0)
        store();
    else if( size == 0)
        unlink();  

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
bool Block::is_half(){ return size == MAX_DIGESTS/2; }
bool Block::is_empty(){ return size ==0; } 
bool Block::is_underloaded(){ return size < MAX_DIGESTS/2 -1; } 
bool Block::is_loaded(){ return buffer != NULL; }
const char* Block::get_id(){ return id; }
const char* Block::get_buffer(){ return buffer; }
uint64_t Block::get_size(){ return size; }

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

bool Block::load(bool degraded_mod){
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

    if( degraded_mod && size > 0)
        memcpy(id, + buffer + (size-1) * DIGEST_LENGTH, DIGEST_LENGTH);
    
    
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

int Block::get_pos_s(const char* digest){
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

int Block::get_pos_i(const char* digest, int pos_s){
    if( size == 0)
        return 0;
        
    int flag = memcmp(digest, buffer + pos_s * DIGEST_LENGTH, DIGEST_LENGTH);

    if( flag <= 0)
        return pos_s;
    else 
        return pos_s+1;
}

const char* Block::pick_smallest(){
    return buffer;
}

const char* Block::pick_greatest(){
    return buffer + (size-1) * DIGEST_LENGTH;
}

bool Block::add(const char* digest){
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

bool Block::remove(const char* digest){
    int p =  get_pos_s( digest );
    int pos = p * DIGEST_LENGTH;

    if( size == 0 || memcmp(digest, buffer + pos, DIGEST_LENGTH) != 0 ) //not in Block
        return false;
        
    memmove( buffer + pos, buffer + pos + DIGEST_LENGTH, (size-p-1) * DIGEST_LENGTH);
    size--;

    if( size != 0 && pos == 0 ) 
        memcpy(id, buffer, DIGEST_LENGTH); 
    
    return true;
}

bool Block::exists(const char* digest){
    int pos = get_pos_s( digest ) * DIGEST_LENGTH;
    if( size > 0 && memcmp(digest, buffer + pos, DIGEST_LENGTH) == 0 )
        return true;
    return false;
}

void Block::merge(Block* right){
    memcpy(id, right->get_id(), DIGEST_LENGTH);
    memcpy( buffer + size * DIGEST_LENGTH, right->get_buffer(), right->get_size() * DIGEST_LENGTH);
    size += right->get_size(); // garanted by caller that size<MAX_DIGESTS
}

void Block::unlink(){
    std::remove( location.c_str() );
}

void Block::print(int step){
    if( step ==0)
    printf("\n\n*************************************************************\n");
    for(int i=0; i<step; i++)
        printf("|   ");
    printf("Block %08X name=%d size=%d id:%s\n", reinterpret_cast<intptr_t>(this), name, size, id); 
    if( step ==0)
    printf("\n\n*************************************************************\n");
}


/**
 *  Begin LRU part
 */
 
LRU::~LRU(){}

void LRU::add(Block* item){
    unordered_map<Block*, list<Block*>::iterator >::iterator it = items_map.find( item ); 
    if( it != items_map.end() )
        items.erase( it->second );
        
    list< Block*>::iterator it22 = items.begin();

    items.push_front( item );
    items_map[item] = items.begin();
    size++;

    if( size >= max_size ){
        Block* tmp = items.back();
        items_map.erase( tmp );
        tmp->store();
        tmp->clean();
        
        items.pop_back();
        size--;
    }
    
    if( !item->is_loaded() )
        item->load();
}

void LRU::remove(Block* item){
    unordered_map<Block*, decltype(items.begin())>::iterator it = items_map.find( item ); 

    if( it != items_map.end() ){
        Block* tmp = *(it->second);
        tmp->store();
        tmp->clean();
        
        items_map.erase( it );
        items.erase( it->second );
    }
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

void BNode::clear(){
    for(size_t i=0; i<size_c ; i++)
        children[i] = NULL;
        
    for(size_t i=0; i<size_b ; i++)
        blocks[i] = NULL;
        
    size_c = size_b = 0;
}

bool BNode::is_full(){ 
    return (size_b>= 2*d) || (size_c>= 2*d); 
}

bool BNode::is_half(){ return size_c == d || size_b == d; }
bool BNode::is_empty(){ return (size_c<d && size_b<d); }
bool BNode::is_leaf(){ return leaf; }

const char* BNode::get_id(){ return id; }
size_t BNode::get_size_b(){ return size_b; }
size_t BNode::get_size_c(){ return size_c; }
Block** BNode::get_blocks(){ return blocks; }
BNode** BNode::get_children(){ return children; }

int BNode::get_child_pos_s(const char* digest){
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

int BNode::get_child_pos_i(const char* digest, int pos_s){
    if( size_c == 0 )
        return 0;
    
    int a = pos_s;
    int flag = memcmp(digest, children[a]->get_id(), DIGEST_LENGTH);
    
    if( flag <= 0)
        return a;
    else 
        return a+1;       
}

int BNode::get_block_pos_s(const char* digest){
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

int BNode::get_block_pos_i(const char* digest, int pos_s){
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

bool BNode::add_digest(const char* digest, LRU* cache){
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

            if( size_b != 0 && pos_b!=2*d ){
                memmove(&blocks[pos_b+2], &blocks[pos_b+1], (size_b-pos_b-1) * sizeof(Block*)); //+1 tjs garantie par la gestion de is_full
            }
            blocks[pos_b+1] = right;
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

            if( size_c != 0 && pos_c!=2*d)
                memmove(&children[pos_c+2], &children[pos_c+1], (size_c-pos_c-1) * sizeof(BNode*)); //+1 tjs garantie par la gestion de is_full

            children[pos_c+1] = right;
            size_c++;
        }
        return true;
    }
}

void BNode::remove_block(int pos, LRU* cache){
    cache->remove( blocks[pos] );
    delete blocks[pos];
    
    if( pos != size_b-1 )
        memmove( &blocks[pos], &blocks[pos+1], sizeof(Block*) * (size_b-pos-1) );
    else if( size_b > 1 )
        memcpy( id, blocks[pos-1]->get_id(), DIGEST_LENGTH); 
    size_b--;
}

void BNode::remove_child(int pos){
    delete children[pos];
    
    if( pos != size_c-1 )
        memmove( &children[pos], &children[pos+1], sizeof(BNode*) * (size_c-pos-1) );
    else if( size_c > 1 )
        memcpy( id, children[pos-1]->get_id(), DIGEST_LENGTH); 
    size_c--;
    
}

Block* BNode::lost_smallest_block(){
    Block* tmp = blocks[0];
    size_b--;//size_b >d ie size_b-1>1
    
    memmove( &blocks[0], &blocks[1], sizeof(Block*) * size_b); 
    return tmp;
}

Block* BNode::lost_greatest_block(){
    Block* tmp = blocks[size_b-1];
    size_b--;//size_b >d ie size_b-1>1

    memcpy( id, blocks[size_b]->get_id(), DIGEST_LENGTH);
    return tmp;
}

BNode* BNode::lost_smallest_child(){
    BNode* tmp = children[0];
    size_c--;//size_c>d ie size_c-1>1

    memmove( &children[0], &children[1], sizeof(BNode*) * size_c);
    return tmp;
}

BNode* BNode::lost_greatest_child(){
    BNode* tmp = children[size_c-1];
    size_c--;//size_b >d ie size_b-1>1

    memcpy( id, children[size_c]->get_id(), DIGEST_LENGTH);
    return tmp;
}

void BNode::win_block(Block* right){
    blocks[size_b] = right;
    size_b++;
    memcpy(id, right->get_id(), DIGEST_LENGTH);
}

void BNode::win_child(BNode* right){
    children[size_c] = right;
    size_c++;
    memcpy(id, right->get_id(), DIGEST_LENGTH);
}

void BNode::rebalance(){
    for(size_t i=0; i<size_c; i++)
        rebalance( children[ i ], i);
}

void BNode::rebalance(BNode* current_c, size_t pos_c){
    if( !current_c->is_empty())
        return;

    if( pos_c < size_c-1 && !children[pos_c+1]->is_half() ){ //ie children[pos_c+1].size >= d+1
        BNode* sibling = children[pos_c+1];
        
        if( current_c->is_leaf() ){
            Block* tmp = sibling->lost_smallest_block();
            current_c ->win_block( tmp );//no rebalance needed because sibling.size >= d+1
        }else{
            BNode* tmp = sibling->lost_smallest_child();
            current_c->win_child( tmp );
        }
    }else if( pos_c > 0 && !children[pos_c-1]->is_half() ){ //ie children[pos_b-1].size >= d+1
        BNode* sibling = children[pos_c-1];
        if( current_c->is_leaf() ){
            Block* tmp = sibling->lost_greatest_block();
            current_c ->win_block( tmp );
        }else{
            BNode* tmp = sibling->lost_greatest_child();
            current_c->win_child( tmp );
        }
    }else{//merge
        if( pos_c < size_c-1 ){ //right fusion
            current_c->merge( children[pos_c+1] );
            children[pos_c+1]->clear();
            remove_child( pos_c+1 );
        }else if( pos_c > 0){ //left fusion
            children[pos_c-1]->merge( current_c );
            current_c->clear();
            remove_child( pos_c );
        }
    }
}

bool BNode::remove_digest(const char* digest, LRU* cache){
    if( size_b + size_c == 0 )
        return false;

    bool flag = false;
    if( leaf ){
        int pos_b = get_block_pos_s( digest );
        Block* current_b = blocks[ pos_b ];
        
        cache->add( current_b );
        flag = current_b->remove(digest);

        if( current_b->is_empty())
            remove_block( pos_b, cache );
        else if( current_b->is_underloaded()){
            if( pos_b < size_b-1 && !blocks[pos_b+1]->is_half() ){ //ie blocks[pos_b+1].size >= d+1
                Block* sibling = blocks[pos_b+1];
                cache->add( sibling );

                const char* tmp = sibling->pick_smallest();
                current_b->add( tmp );
                sibling->remove( tmp ); //no rebalance needed because sibling.size >= d+1
            }else if( pos_b > 0 && !blocks[pos_b-1]->is_half() ){ //ie blocks[pos_b-1].size >= d+1
                Block* sibling = blocks[pos_b-1];
                cache->add( sibling );

                const char* tmp = sibling->pick_greatest();
                current_b->add( tmp );
                sibling->remove( tmp ); //no rebalance needed because sibling.size >= d+1
            }else{//merge
                if( pos_b < size_b-1 ){ //right fusion
                    cache->add( blocks[pos_b+1] );
                    current_b->merge( blocks[pos_b+1] );
                    remove_block( pos_b+1, cache );
                }else{ //left fusion pos_b>0 because d>=2
                    cache->add( blocks[pos_b-1] );
                    blocks[pos_b-1]->merge( current_b );
                    remove_block( pos_b, cache );
                }
            }
        }

        if( memcmp( id, digest, DIGEST_LENGTH) ==0 && size_b>0)
            memcpy( id, (blocks[size_b-1])->get_id(), DIGEST_LENGTH);
        
    }else{
        int pos_c = get_child_pos_s( digest );
        BNode* current_c = children[ pos_c ];

        bool flag = current_c->remove_digest(digest, cache);

        rebalance( current_c, pos_c );
        if( memcmp( id, digest, DIGEST_LENGTH) ==0 ) 
            memcpy( id, (children[size_c-1])->get_id(), DIGEST_LENGTH); //size_c>=1 because d>1
    }
    
    return flag;
}

bool BNode::exists_digest(const char* digest, LRU* cache){
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

void BNode::merge( BNode* right ){
    memcpy(id, right->get_id(), DIGEST_LENGTH);

    if( leaf ) //  garanted by caller that size_b <= d same for right->get_size_b()
        memcpy( &blocks[size_b], &(right->get_blocks())[0], right->get_size_b() * sizeof(Block*));
    else //  garanted by caller that size_c <= d same for right->get_size_c()
        memcpy( &children[size_c], &(right->get_children())[0], right->get_size_c() * sizeof(BNode*));

    size_b += right->get_size_b(); // garanted by caller that size_b<2*d+1
    size_c += right->get_size_c(); // garanted by caller that size_b<2*d+1
}

//BNode* BNode::split_in_2(float ratio){
    //int pos = 0;
    //BNode* right = NULL;
    
    //if( size_b > 0){
        //pos = (int)ratio * size_b;
        //Block* current_b=NULL;
        
        //for(int i=0; i<pos; i++){
            //current_b = lost_greatest_block();
            //right->win_block(current_b);
        //}
    //}else{
        //pos = (int)ratio * size_c;
        //BNode* current_c=NULL;
        
        //for(int i=0; i<pos; i++){
            //current_c = lost_greatest_child();
            //right->win_child(current_c);
            //rebalance( current_c, pos );
        //}
        
        //right->rebalance();
    //}
    
    //return right;
//}

void BNode::print(int step){
if( step ==0)
    printf("\n\n====================================================================\n");
    
    for(int i=0; i<step; i++)
        printf("|   ");
    printf("BNode %08X size_b=%d size_c=%d leaf:%s id:%s\n", reinterpret_cast<intptr_t>(this), size_b, size_c, leaf ? "true" : "false", id); 
    
    for(int i=0; i<step; i++)
        printf("|   ");
    printf("Children\n");
    for(int i=0; i<size_c; i++)
        children[i]->print(step+1);
    for(int i=0; i<step; i++)
        printf("|   ");
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

bool BTree::add_digest(const char* digest){
    bool flag = root->add_digest( digest, cache );
    
    if( root->is_full() ){
        BNode* right = root->split();
        BNode* left = root;
        root = new BNode(path.c_str(), left, right);
    }

    return flag;
}

bool BTree::exists_digest(const char* digest){
    return root->exists_digest( digest, cache );
}

bool  BTree::remove_digest(const char* digest){
   bool flag = root->remove_digest( digest, cache );
   root->rebalance();

   return flag;
}

//BTree* BTree::split_in_2(float ratio){
    //BNode* right_root = root->split_in_2( ratio );
    //BTree* right = new BTree( path, right_root );
    //return right;
//}

void BTree::print(){
    root->print();
}
