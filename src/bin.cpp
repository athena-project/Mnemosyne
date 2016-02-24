#include <ups/upscaledb.h>
#include <openssl/sha.h>
#include <stdio.h> // for printf
#include <stdlib.h> // for exit
#include <string>

//#include "chunk.cpp" //pour le test uniquemnet

///ldconfig quand marche pas
///g++ -g -std=c++11 -o3 -g bin2.cpp -o bin2 -lssl -lcrypto -lupscaledb  -lboost_serialization
#define DBNAME_CUSTOMER 1

using namespace std;

 //this helper function will handle unexpected errors.
static void check_error(ups_status_t st, const char *description) {
	if (st != UPS_SUCCESS) {
		//printf("ERROR %s (%s), terminating\n", description, ups_strerror(st));
		exit(-1);
	}
}

class Bin{
	protected:
		int size=0; ///Nombre de hash
		int length=0;///Nombre de set
		
		string path; ///Where the data is save
		string id; ///hash id min_hash
			
		///Upscaledb
		ups_status_t st;
		ups_env_t *env;
		ups_db_t *db;
	public:
		Bin(string i, string p="") : path(p), id(i){
			if( path != "")
				st = ups_env_create (&env, (path+"/"+id).c_str(), 0, 0664, 0);
			else
				st = ups_env_create (&env, id.c_str(), 0, 0664, 0);
			//check_error(st, "ups_env_create");
			
			ups_parameter_t param[] = {
				{UPS_PARAM_KEY_TYPE, UPS_TYPE_BINARY},
				{UPS_PARAM_KEY_SIZE, 224}, ///sha224
				{UPS_PARAM_RECORD_SIZE, sizeof(bool)},
				{0, 0}
			};
			st = ups_env_create_db(env, &db, DBNAME_CUSTOMER, 0, &param[0]);
			//check_error(st, "ups_env_create_db");
		}
		
		~Bin(){
			st = ups_env_close (env, UPS_AUTO_CLEANUP);
			//check_error(st, "ups_env_close");
		}
		
		void insert(unsigned char* digest){
			ups_key_t key = ups_make_key((void *)digest, SHA224_DIGEST_LENGTH+1);
			ups_record_t record = ups_make_record((void *)true, sizeof(true));
			//check_error(st, "ups_make_key");
		}
		
		bool exists(unsigned char* digest){
			ups_key_t key = ups_make_key((void *)digest, SHA224_DIGEST_LENGTH+1);
			ups_record_t record;

			st = ups_db_find(db, NULL, &key, &record, 0);
			if(st == UPS_SUCCESS)
				return true;
			else if(st == UPS_KEY_NOT_FOUND)
				return false;
				
			//check_error(st, "ups_db_find");
		}	
};

//int main(){
	////new env 
	////ups_status_t st;
	////ups_env_t *env;
	////st = ups_env_create (&env, "test.db", 0, 0664, 0);
	////check_error(st, "ups_env_create");
	
	//////new db
	////ups_db_t *db;
	////ups_parameter_t param[] = {
		////{UPS_PARAM_KEY_TYPE, UPS_TYPE_BINARY},
		////{UPS_PARAM_KEY_SIZE, 224}, ///sha224
		////{UPS_PARAM_RECORD_SIZE, sizeof(bool)},
		////{0, 0}
	////};
	////st = ups_env_create_db(env, &db, DBNAME_CUSTOMER, 0, &param[0]);
	////check_error(st, "ups_env_create_db");
	
	////ChunkFactory cf;
	////ChunkFactory cf("krh.ser");
	////vector<Chunk> chunks = cf.split("zero.zero");
	////Bin b("gsoigufius.iosdsd");
	////for(vector<Chunk>::iterator it = chunks.begin(); it != chunks.end(); it++){
		//////insertion
		////b.insert( (unsigned char*) it->get_digest() );
		//////ups_key_t key = ups_make_key((void *)(it->get_digest()), 225);
		////////ups_key_t key = ups_make_key((void *)"b05c7bc56ac792dcdf9c4f108bfc2076bc62c12c8aaf1c9bfc9e9423", 225);
		//////ups_record_t record = ups_make_record((void *)true, sizeof(true)	);
		//////check_error(st, "ups_make_key");
		//////print_sha_sum( it->get_digest());
	//////}
	////cout<<"Number"<<chunks.size()<<endl;
	////closing 
	////st = ups_env_close (env, UPS_AUTO_CLEANUP);
	////check_error(st, "ups_env_close");
//}
int main(){}
