#include "client/Client.h"
#include "sd/MapServer.h"

#include <iostream>
#include <thread>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

void fct(MapServer* mp){ mp->run(); }

int main(){
	NodeMap map(3);
	map.add_node(new Node(55, 1987, "127.0.1.1") );
	map.add_node(new Node(575, 1988, "127.0.1.1") );
	map.add_node(new Node(4, 1989, "127.0.1.1") );

	Client client("1956", &map);
	
	MapServer ms1("1987", &map);
	MapServer ms2("1988", &map);
	MapServer ms3("1989", &map);
	
	std::thread t1(fct, &ms1);
	std::thread t2(fct, &ms2);
	//std::thread t3(fct, &ms3);
	
	client.save("a1", "/home/severus/Downloads/cir_38300.pdf", fs::path("test_storage"));
	printf("\n\n end\n\n");
	t1.join();
	t2.join();
}
