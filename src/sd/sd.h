//if all in memory
#include <map>

map<const unsigned char*, bool> chunks;

int main(){
	if(read){
		if( chunks.count(key) )
			send( true );
		else
			send(false);
	}else if(write){
		chunks[key] = true;
		send(true);
	}
}
