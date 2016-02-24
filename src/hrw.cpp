#ifndef MNEMOSYNE_HRW_H
#define MNEMOSYNE_HRW_H

#include <cstdint>
#include <vector>
#include <map>
#include "wrand.h"

using namespace std;

///must keep died nodes to preserve mapping at first, until all data replace
enum node_state{ ACTIVE, FULL, LOST };

class Node{
	protected:
		uint64_t id = 0;
		int port = 0;
		const char* host = "";
		node_state state = ACTIVE;
	
	public:
		Node( uint64_t _id ) : id(_id){}
		Node( uint64_t _id, int p, const char* h ) : id(_id), port(p), host(h){}
		~Node(){}
		
		uint64_t get_id(){ return id; }
		int get_port(){ return port; }
		const char* get_host(){ return host; }
		void set_state(node_state s){ state = s; }
		
		bool alive(){ return state != LOST; }
		bool full(){ return state == FULL; }
};

class NodeMap{
	protected:
		vector<Node*> nodes;	//non responsable des noeuds	
		map<uint64_t, Node*> map_nodes;
		
		int n = 1; //nombre de noeuds à choisir
	public:
		NodeMap(){}
		
		NodeMap(int _n) : n(_n){}
		
		void add_node(Node* node){ 
			nodes.push_back(node); 
			map_nodes[ node->get_id() ] = node;	
		}
		
		Node* get_node(uint64_t key){ return map_nodes[key]; }
		
		
		/////for lecture, pick the prime
		Node* rallocate(uint64_t key){
			Node* current = NULL;
			uint64_t weight = 0;
			uint64_t tmp = 0;
			
			vector<Node* >::iterator it = nodes.begin();
			for(; it != nodes.end() && !(current=*it)->alive(); it++){}
			if( it != nodes.end() )
				weight = wrand(key, current->get_id() );
			
			for(it++ ; it != nodes.end() ; it++){
				if( (*it)->alive() && 
					(tmp = wrand(key, (*it)->get_id() ) ) > weight){
					current = *it;
					weight = tmp;
				}
			}
			
			return current;
		}
		
		Node* rallocate(char* key, size_t s){//troncature si trop long
			char* end = key + (s>16 ? 16 : s);
			return rallocate( strtoull(key, &key, 16));
		}
		
		Node* find_prime(uint64_t key){
			Node* current = NULL;
			uint64_t weight = 0;
			uint64_t tmp = 0;
			
			vector<Node* >::iterator it = nodes.begin();
			while(it != nodes.end() && 
				(!(current=*it)->alive() || (*it)->full())){
				it++;
			}
			if( it == nodes.end() )
				return NULL;

			weight = wrand(key, current->get_id() );
			for(it++ ; it != nodes.end() ; it++){
				if( (*it)->alive() && !(*it)->full() &&
					(tmp = wrand(key, (*it)->get_id() ) ) > weight){
					current = *it;
					weight = tmp;
				}
			}
			
			return current;
		}
		
		///O(k*n) k is in practice < 5
		vector<Node*> wallocate(uint64_t key){
			vector<Node*> k_nodes=vector<Node*>(5);

			Node* prime = find_prime(key);
			if( prime == NULL )
				return k_nodes;
			uint64_t max = wrand( key, prime->get_id() );
			k_nodes[0] = prime;
			 
			 
			for(int k=1; k<n && (prime != NULL); k++){
				Node* current = NULL;
				uint64_t weight = 0;
				uint64_t tmp = 0;
				
				vector<Node* >::iterator it = nodes.begin();
				while(it != nodes.end() && 
					(!(current=*it)->alive() || (*it)->full() || 
					wrand(key, (*it)->get_id() ) >= max )){
					it++;
				}
				if( it == nodes.end() )
					return k_nodes;
		
				weight = wrand(key, current->get_id() );
				for(it++ ; it != nodes.end() ; it++){
					if( (*it)->alive() && !(*it)->full() &&
						(tmp = wrand(key, (*it)->get_id()) ) > weight && 
						weight < max){
						current = *it;
						weight = tmp;
					}
				}
				
				prime = current;
				k_nodes[k] = current;;
				max = weight;
			} 
				
			return k_nodes;
		}

		vector<Node*> wallocate(char* key, size_t s){//troncature si trop long
			char* end = key + (s>16 ? 16 : s);
			return wallocate( strtoull(key, &key, 16));
		}
		
};
#endif
