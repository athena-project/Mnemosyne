#include "client/Client.h"
#include "sd/MapServer.h"

#include <iostream>
#include <thread>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

void fct(MapServer* mp){ mp->run(); }

int main(){
    NodeMap map(3);
    map.add_node(new Node(55, 19817, "127.0.1.1") );
    map.add_node(new Node(575, 19818, "127.0.1.1") );
    map.add_node(new Node(4, 19819, "127.0.1.1") );

    Client client("1926", &map);
    free( malloc( 1<< 24) );
    MapServer ms1("19817", &map);
    MapServer ms2("19818", &map);   
    MapServer ms3("19819", &map);
    
    std::thread t1(fct, &ms1);
    std::thread t2(fct, &ms2);
    std::thread t3(fct, &ms3);
    
    if( client.save_bench("a1", "/home/severus/Downloads/test_1.mkv", fs::path("/home/severus/test_storage")) )
        cout<<"a1 - saved successfully"<<endl;
    else
        cout<<"a1 - saved error"<<endl;
        
    if( client.load_bench("a1", "/home/severus/Downloads/a1", fs::path("/home/severus/test_storage")) )
        cout<<"a1 - loaded successfully"<<endl;
    else
        cout<<"a1 - loaded error"<<endl;
    t1.join();
    t2.join();
    t3.join();
}
