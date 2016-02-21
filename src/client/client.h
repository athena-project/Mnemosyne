#include "network/TCPServer.h"
#include "hrw.cpp"
#include "Chunk.h"

#define little_cluster; //ie nombre node << nombre moyen de chunk par file

//objet qui file -> hash -> dedup+store ? -> true or false
class Client{//en fait faut 2 thread separé un qui ecoute le reseau(et qui envoie en asynchrone es requetes, un qui cacul les chunjs 
	protected:
		NodeMap nodes(3); //3-replication
		ChunkFactory cf("krh.ser");
		
	public:
		Client(char* port, char* _nodes){//_nodes json or ini
			//call parent constructeur
		} 
		
		uint64_t convert(char* key){
			strtoll(key, key+16, 16);
		}
		
		#ifndef little_cluster
		bool save(unsigned char* location){
			vector<Chunk> chunks = cf.split("zero.zero");
			//coute un peu de prétraitement mais pas de reseau
			uint64_t tmp_id;
			map<uint64_t, vector<Chunk&> > buffers;
			map<unsigned char*, bool> exists; ///true=> le fichier existe dans la base, faut le fichier n'existe pas
			
			
			for(int i=0; i<chunks.size(); i++){
				tmp_id = nodes.rallocate( convert(chunks[i].get_digest()) ).get_id();
				
				if( buffers.find(tmp_id) == buffers.end() )
					buffers[tmp_id]=list<Chunk&>();
				
				buffers[tmp_id].push_back( chunks[i] );
				exists[chunks[i].get_digest()] = false;
			}
			
			map<uint64_t, vector<Chunk&> >::iterator it = buffers.beg();
			for(; it != buffers.end(); it++){
				
				char* buffer = new char[ (it->second).size() * SHA224_DIGEST_LENGTH];
				for(int i=0,k=0; i<(it->second).size(); i++, k+=SHA224_DIGEST_LENGTH)
					memcpy(buffer+k, static_cast<char*>(it->second)[i].get_digest(),SHA224_DIGEST_LENGTH);
				send(buffer, nodes.get_nodes( it->first )
			}
			
			//for chink in chunkswallocate();
		}
		#else
		
		#endif
};
