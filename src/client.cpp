#include "client/Client.h"
#include "sd/MapServer.h"

#include <iostream>
#include <thread>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

void fct(MapServer* mp){ mp->run(); }

int main(){
    NodeMap map(2);
    //map.add_node(new Node(55, 19107, "127.0.1.1") );
    //map.add_node(new Node(575, 19108, "127.0.1.1") );
    //map.add_node(new Node(4, 1989, "127.0.1.1") );

    Client client("1926", &map);
    
    //MapServer ms1("19107", &map);
    //MapServer ms2("19108", &map);   
    //MapServer ms3("1989", &map);
    
    //std::thread t1(fct, &ms1);
    //std::thread t2(fct, &ms2);
    //std::thread t3(fct, &ms3);
    
    //client.save("a1", "/home/severus/Downloads/cir_38300.pdf", fs::path("/home/severus/test_storage"));
    client.load("a1", "/home/severus/Downloads/restaure_cir_38300.pdf", fs::path("/home/severus/test_storage"));
    //t1.join();
    //t2.join();
}
