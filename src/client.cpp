#include "client/Client.h"
#include "sd/MapServer.h"
#include "index/DynamicIndex.h"

#include <iostream>
#include <thread>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

void fct(MapServer* mp){ mp->run(); }

//int main(){
    //NodeMap map(1);
    //map.add_node(new Node(55, 19837, "127.0.1.1") );
    //map.add_node(new Node(575, 1988, "127.0.1.1") );
    //map.add_node(new Node(4, 1989, "127.0.1.1") );

    //Client client("1926", &map);
    //free( malloc( 1<< 24) );
    //MapServer ms1("19837", &map, "/home/severus/test_tmp");
    ////MapServer ms2("1988", &map, "/home/severus/test_tmp");   
    ////MapServer ms3("1989", &map, "/home/severus/test_tmp");
    
    //std::thread t1(fct, &ms1);
    ////std::thread t2(fct, &ms2);
    ////std::thread t3(fct, &ms3);
    
    //if( client.save_bench("a1", "/home/severus/Downloads/test_1.pdf", fs::path("/home/severus/test_storage")) )
        //cout<<"a1 - saved successfully"<<endl;
    //else
        //cout<<"a1 - saved error"<<endl;
        
    //if( client.load_bench("a1", "/home/severus/Downloads/a1", fs::path("/home/severus/test_storage")) )
        //cout<<"a1 - loaded successfully"<<endl;
    //else
        //cout<<"a1 - loaded error"<<endl;
    //t1.join();
    ////t2.join();
    ////t3.join();
//}

int main(){
    BTree b("/home/severus/test_tmp");

    //b.add_digest("8b14eca05af22f19c4afffc0531a72bf2d11b63d15417ca14a475354");
    
    //b.add_digest("8b14eca05af22f19c4afaac0531a72bf2d11b63d17db7ca14a475354");
    
    //b.add_digest("8b14eca05af22f19c4afaac0531a73bf2d11b63d17db7ca14a475354");
    
    //b.add_digest("8b14eca05af21f19c4afffc0531a72bf2d11b63d15417ca14a475354");
    
    //b.add_digest("8b14ea5b5af22f19c4afaac0531a72bf2d11b63d17db7ca14a475354");
    
    //b.add_digest("8b14eca05af22f19c4aff6c0531a73bf2d11b63d17db7ca14a475354");
    
    //b.add_digest("9b14eca05af22f19c4afffc0531a72bf2d11b63d15417ca14a475354");
    
    //b.add_digest("0c14eca05af22f19c4afaac0531a72bf2d11b63d17db7ca14a475354");
    
    //b.add_digest("0ca4eca05af22f19c4afaac0531a73bf2d11b63d17db7ca14a475354");
    
    //b.add_digest("0cb4eca05af21f19c4afffc0531a72bf2d11b63d15417ca14a475354");
    
    //b.add_digest("0cc4ea5b5af22f19c4afaac0531a72bf2d11b63d17db7ca14a475354");
    
    //b.add_digest("ocd4eca05af22f19c4aff6c0531a73bf2d11b63d17db7ca14a475354");
    
    //b.remove_digest("8b14eca05af22f19c4afffc0531a72bf2d11b63d15417ca14a475354");
    //b.remove_digest("8b14eca05af22f19c4afaac0531a72bf2d11b63d17db7ca14a475354");
    //b.remove_digest("8b14eca05af22f19c4afaac0531a73bf2d11b63d17db7ca14a475354");
    //b.remove_digest("8b14eca05af21f19c4afffc0531a72bf2d11b63d15417ca14a475354");
    //b.remove_digest("8b14ea5b5af22f19c4afaac0531a72bf2d11b63d17db7ca14a475354");
    //b.remove_digest("8b14eca05af22f19c4aff6c0531a73bf2d11b63d17db7ca14a475354");
    //b.remove_digest("9b14eca05af22f19c4afffc0531a72bf2d11b63d15417ca14a475354");
    //b.remove_digest("0c14eca05af22f19c4afaac0531a72bf2d11b63d17db7ca14a475354");
    //b.remove_digest("8b14eca05af21f19c4afffc0531a72bf2d11b63d15417ca14a475354");
    //b.remove_digest("0cb4eca05af21f19c4afffc0531a72bf2d11b63d15417ca14a475354");
    //b.remove_digest("0cc4ea5b5af22f19c4afaac0531a72bf2d11b63d17db7ca14a475354");
    //b.remove_digest("ocd4eca05af22f19c4aff6c0531a73bf2d11b63d17db7ca14a475354");
    
    
    
    
    ChunkFactory c("chunks_factory_conf.data");
    vector<Chunk*> chunks;
    c.split("/home/severus/Downloads/test_1.avi", chunks);
    Timer t;
    for(int i=0; i<chunks.size(); i++){
        b.add_digest(chunks[i]->ptr_digest());
        //delete chunks[i];
    }
    
    for(int i=0; i<chunks.size(); i++){
        b.remove_digest(chunks[i]->ptr_digest());
        delete chunks[i];
    }
    cout<<t.elapsed()<<endl;
}
