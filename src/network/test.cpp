#include <iostream>
#include <cstdint>
#include <inttypes.h>

int main(){
	uint64_t g = 567;
	char buff[6];
	char* end = buff+sizeof(uint64_t);
	sprintf(buff, "%" PRIu64 "", g); 
	g =	strtoull(buff, &end, 0);
std::cout<<g<<std::endl;
		
}
