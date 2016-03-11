#ifndef MNEMOSYNE_INDEX_DYNAMICINDEX_H
#define MNEMOSYNE_INDEX_DYNAMICINDEX_H

#include <boost/filesystem.hpp>
#include <unordered_map>
#include <list>
#include <cstddef>
#include <fcntl.h>    /* For O_RDWR */
#include <unistd.h>   /* For open(), creat() */
#include <inttypes.h>

#include "../utility/hash.cpp"

namespace fs = boost::filesystem;
using namespace std;
//~224Ko
#define MAX_DIGESTS 1024
#define uint64_s sizeof(uint64_t)
#define d 100 // à remplacer par template ??, strictement supérieur à 1
#define CACHE_SIZE 100 //en block

class Block{
    protected:
        static uint64_t _id;
        uint64_t name = 0;
    
        uint64_t size; //in digests
        const char* location;
        char id[DIGEST_LENGTH];
        
        const char* path; //couteux à conserver...
        
        int fd = -1;
        char* buffer = NULL;
    public:
        Block(const char* path) : name( _id++ ){
            init(path);
        }
        
        Block(const char* path, char* data, uint64_t _size){
            init(path);
            
            buffer = new char[ MAX_DIGESTS * DIGEST_LENGTH];
            memcpy( buffer, data, _size * DIGEST_LENGTH);
            size = _size;
        }
        
        void init(const char* path){
            char _name[sizeof(uint64_t)];
            sprintf(_name, "%" PRIu64 "", name);
            location = (fs::path(path) / fs::path(_name)).string().c_str();
        }
        
        ~Block(){
            if( buffer != NULL ){
                free( buffer );
                close( fd );
            }
        }
        
        void clean(){
            if( buffer != NULL ){
                free(buffer);
                buffer = NULL;
            }
            
            if( fd != -1 ){
                close( fd );
                fd = -1;
            }
        }
        
        bool is_full(){ return size >= MAX_DIGESTS-1; } ///-1 très important
        
        bool is_loaded(){ return buffer != NULL; }
        
        char* get_id(){ return id; }
        
        Block* split(){
            uint64_t m = size / 2;
            char* mid = buffer + m * DIGEST_LENGTH;
            Block* right = new Block(path, mid, size-m);
            size = m;
            
            return right;
        }
        
        bool load(){
            buffer = new char[ MAX_DIGESTS * DIGEST_LENGTH];
            fd = open(location, O_RDONLY); 
            if( fd == -1)
                return false;
                
            int f = read(fd, buffer, MAX_DIGESTS * DIGEST_LENGTH);
            if( f == -1 ){
                clean();
                return false;
            }
            
            char* end = buffer + uint64_s;
            size = f / DIGEST_LENGTH;
            
            close(fd);
            fd = -1;
            
            return true;
        }       
            
        bool store(){
            fd = open(location, O_WRONLY);
            if( fd == -1)
                return false;
                
            if( write(fd, buffer, size * DIGEST_LENGTH) == -1)
                return false;
            
            close(fd);
            fd = -1;
            return true;
        }
        
        ///assume tha block is load
        int get_pos(char* digest){
            int a = 0;
            int b = size-1;
            int m = (a + b) / 2;
            int flag = 0;
            
            
            while( a < b ){
                flag = memcmp(digest, buffer + m * DIGEST_LENGTH, DIGEST_LENGTH);
                
                if( flag < 0 ) //digest<buffer+a*...
                    b = m;
                else if(flag > 0 )
                    a = m;
                else
                    return m;
            }
            return a;           
        }
        
        bool add(char* digest){
            if( is_full() )
                return false;
                
            int p = get_pos( digest );
            int pos = p * DIGEST_LENGTH;
            if( memcmp(digest, buffer + pos, DIGEST_LENGTH ) == 0 )
                return true;
                
            memmove(buffer + pos + DIGEST_LENGTH, buffer + pos, (size-p-1) * DIGEST_LENGTH);
            memcpy(buffer + pos, digest, DIGEST_LENGTH);
            
            
            if( size == 0 || memcmp(digest, id, DIGEST_LENGTH) < 0 ) 
                memcpy(id, digest, DIGEST_LENGTH); 
            size++;

            return true;
        }
        
        bool exists(char* digest){
            int pos = get_pos( digest ) * DIGEST_LENGTH;
            if( memcmp(digest, buffer + pos, DIGEST_LENGTH) == 0 )
                return true;
            return false;
        }
};

class LRU{
    protected:
        list< Block*> items; 
        unordered_map<Block*, decltype(items.begin())> items_map;
        
        size_t size = 0;
        size_t offset = 0; //debut du cache, permet d'ameliorer les inserts en o(1)
        size_t max_size = 0;
        
    public:
        LRU( size_t m) : max_size( m ){}
       
        void add(Block* item){
            items.push_front( item );
            
            unordered_map<Block*, decltype(items.begin())>::iterator it = items_map.find( item ); 
            if( it != items_map.end() ){
                items.erase( it->second );
                it->second = items.begin();
            }else if( size >= max_size ){
                items_map.erase( items.back() );
                items.pop_back();
            }
                            
            if( !item->is_loaded() )
                item->load();
        }
};

///**
 //* dans un chaque block  : id min digest ie le premier
 //* 
 //*/
class BNode{
    protected:
        bool leaf = true;
        char id[DIGEST_LENGTH];//Min des ids de fils
        
        
        BNode* children[2*d]; //ordered by min id of each, peut être utilisé directement un tableau de BNode**
        size_t size_c = 0;
        
        Block* blocks[2*d]; // usefull for leaves only
        size_t size_b = 0;
    public:
        BNode(){}
        
        BNode(BNode* left, BNode* right){
            children[0] = left;
            children[1] = right;
            size_c += 2;
        }
        
        BNode(BNode* data[2*d], uint64_t beg, uint64_t size){
            memcmp( children, data + beg * sizeof(BNode*), size * sizeof(BNode*));
            size_c = size;
        }
        
        BNode(Block* data[2*d], uint64_t beg, uint64_t size){
            memcmp( blocks, data + beg * sizeof(Block*), size * sizeof(Block*));
            size_c = size;
        }
        
        ~BNode(){
            for(int i=0; i<size_c ; i++)
                delete children[ i ];
            
            for(int i=0; i<size_b ; i++)
                delete blocks[ i ];
            
            size_c = size_b = 0;
        }
       
        bool is_full(){ return (size_b>=d-1) || (size_c>=d-1); }
        
        char* get_id(){ return id; }
        
        int get_child_pos(char* digest){
            int a = 0;
            int b = size_c-1;
            int m = (a + b) / 2;
            int flag = 0;
            
            
            while( a < b ){
                flag = memcmp(digest, children[m]->get_id(), DIGEST_LENGTH );
                
                if( flag < 0 ) //digest<buffer+a*...
                    b = m;
                else if(flag > 0 )
                    a = m;
                else
                    return m;
            }
            return a;           
        }
        
        int get_block_pos(char* digest){
            int a = 0;
            int b = size_b-1;
            int m = (a + b) / 2;
            int flag = 0;
            
            
            while( a < b ){
                flag = memcmp(digest, blocks[m]->get_id(), DIGEST_LENGTH );
                
                if( flag < 0 ) //digest<buffer+a*...
                    b = m;
                else if(flag > 0 )
                    a = m;
                else
                    return m;
            }
            return a;           
        }
        
        BNode* split(){
            if( !leaf ){
                uint64_t m = size_c / 2;
                BNode* right = new BNode(children, m, size_c-m);
                size_c = m;
                
                return right;
            }else{
                uint64_t m = size_b / 2;
                BNode* right = new BNode(blocks, m, size_b-m);
                size_b = m;
                
                return right;
            }
        }
        
        bool add_digest(char* digest, LRU* cache){
            if( memcmp( digest, id, DIGEST_LENGTH) < 0 )
                memcpy( id, digest, DIGEST_LENGTH);
            
            if( !leaf ){
                int pos_c = get_child_pos( digest );
                BNode* current_c = children[ pos_c ];
                if( !current_c->add_digest( digest, cache) )
                    return false;
                
                if( current_c->is_full()){
                    BNode* right = current_c->split();
                    memmove(children + (pos_c+1) * sizeof( BNode* ), children + pos_c * sizeof( BNode* ), (size_c-pos_c-1) * sizeof( BNode* ));
                    children[pos_c+1] = right;
                    size_c++;
                }
                return true;
                
            }else{
                int pos_b = get_block_pos( digest );
                Block* current_b = blocks[ pos_b ];
               
                cache->add( current_b );
                
                if( !current_b->add( digest ) )
                    return false;
                   
                if( current_b->is_full() ){
                    Block* right = current_b->split();
                    
                    cache->add( right );
                    
                    memmove(blocks + (pos_b+1) * sizeof( Block* ), blocks + pos_b * sizeof( Block* ), (size_b-pos_b-1) * sizeof( Block* ));
                    blocks[pos_b+1] = right;
                    size_b++;
                }
                
                return true;
            }
        }
        
        bool exists_digest(char* digest, LRU* cache){
            if( !leaf )
                return children[ get_child_pos(digest) ]->exists_digest( digest, cache);
            else{
                Block* current = blocks[ get_block_pos(digest) ];
                cache->add( current );
                
                return current->exists(digest);
            }
        }
};  

class BTree{
    protected:
        LRU* cache = NULL;
        BNode* root = NULL;
        
    public:
        BTree(){
            root = new BNode();
            cache = new LRU( CACHE_SIZE );
        }
        
        ~BTree(){
            if( root != NULL )
                delete root;
            if( cache != NULL )
                delete cache;
        }
        
        bool add_digest(char* digest){
            bool flag = root->add_digest( digest, cache );
            if( root->is_full() ){
                BNode* right = root->split();
                BNode* left = root;
                root = new BNode(left, right);
            }
            
            return flag;
        }
};

#endif //MNEMOSYNE_INDEX_DYNAMICINDEX_H
