#include "client/Client.h"
#include "sd/MapServer.h"
#include "index/BinIndex.h"

#include <iostream>
#include <thread>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

void fct(MapServer* mp){ mp->run(); }

int main(){
    NodeMap map(2);

    map.add_node(new Node(55, 1987, "127.0.1.1") );
    map.add_node(new Node(575, 1988, "127.0.1.1") );
    map.add_node(new Node(4, 1989, "127.0.1.1") );

    Client client("1926", &map);
    free( malloc( 1<< 24) );
    MapServer ms1("1987", &map, "/home/severus/test_tmp/1");
    MapServer ms2("1988", &map, "/home/severus/test_tmp/2");   
    MapServer ms3("1989", &map, "/home/severus/test_tmp/3");
    
    std::thread t1(fct, &ms1);
    std::thread t2(fct, &ms2);
    std::thread t3(fct, &ms3);
    
    if( client.bsave_bench("a1", "/home/severus/Downloads/test_1.pdf", fs::path("/home/severus/test_storage")) )
        cout<<"a1 - saved successfully"<<endl;
    else
        cout<<"a1 - saved error"<<endl;
        
    if( client.load_bench("a1", "/home/severus/Downloads/a1", fs::path("/home/severus/test_storage")) )
        cout<<"a1 - loaded successfully"<<endl;
    else
        cout<<"a1 - loaded error"<<endl;
    exit(0);
    t1.join();
    //t2.join();
    //t3.join();
}

//int main(){
    //BTree b("/home/severus/test_tmp");
    ////b.recover();
    
    //ChunkFactory c("chunks_factory_conf.data");
    //vector<Chunk*> chunks;
    //while( c.next("/home/severus/Downloads/test_1.pdf", chunks, 1000) ){}
    //printf("Nbr chunks %zu\n", chunks.size());
    ////Timer t;
    //for(int i=0; i<chunks.size(); i++){
        //b.add_digest(chunks[i]->ptr_digest());
    //}
    ////b.print();
    ////printf(" number of blocks : %zu %zu\n", b.number_blocks(), chunks.size());
    ////b.split(0.2);
    ////printf(" number of blocks : %zu %zu\n", b.number_blocks(), chunks.size());
    ////b.print();
    ////cout<<t.elapsed()<<endl;
    //for(int i=0; i<chunks.size(); i++){
        //if( b.exists_digest(chunks[i]->ptr_digest()) )
            //b.remove_digest(chunks[i]->ptr_digest());
        //delete chunks[i];
    //}
    ////b.print();
    ////cout<<t.elapsed()<<endl;
    
//}


//struct ChunkAsc{
    //bool operator() (Chunk* lhs, Chunk* rhs) const{
        //return memcmp(lhs->ptr_digest(), rhs->ptr_digest(), DIGEST_LENGTH )<0 ;
    //}
//};
//int main(){
    //BinTree b("/home/severus/test_tmp");
    ////b.recover();
    
    //ChunkFactory c("chunks_factory_conf.data");
    //vector<Chunk*> chunks;
    //while( c.next("/home/severus/Downloads/test_1.pdf", chunks, 1000) ){}
    //printf("Nbr chunks %zu\n", chunks.size());
    ////Timer t;
    //char *buffer = new char[ chunks.size() * DIGEST_LENGTH];
        //std::sort( chunks.begin(), chunks.end(), ChunkAsc() );

    //for(int i=0; i<chunks.size(); i++)
        //memcpy(buffer+i*DIGEST_LENGTH, chunks[i]->ptr_digest(), DIGEST_LENGTH);
    ////can not use block::add pour bin
    
    //BinBlock bin1( "", buffer, chunks.size());
    //BinBlock bin2( "", buffer, chunks.size());
    
    //bin1.set_id(chunks[chunks.size()-2]->ptr_digest());
    //bin2.set_id(chunks[chunks.size()-2]->ptr_digest());
    //bin2.set_id("aa");  
    //b.print();
    //b.add_bin( &bin1 );
    //b.add_bin( &bin2 );
    //b.print();
    //printf("end end\n");
    //free(buffer);
    ////merge foire lamentablement, puis on a un pb de destruction
//}
