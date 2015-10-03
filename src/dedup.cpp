//////////// ####################### Thread #######################
#include <sstream>
#include <stdio.h>
#include <openssl/md5.h>
#include <boost/thread/thread.hpp>
#include <vector>
#include <fstream>
#include <deque>
#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

boost::property_tree::ptree pt;
boost::property_tree::ini_parser::read_ini("config.ini", pt);
std::cout << pt.get<std::string>("Section1.Value1") << std::endl;
std::cout << pt.get<std::string>("Section1.Value2") << std::endl;

using namespace std;


void _md5(const char* data, int bytes){
	unsigned char digest[16];

			MD5_CTX ctx;
			MD5_Init(&ctx);
			MD5_Update (&ctx, data, bytes);
			MD5_Final(digest, &ctx);

			char mdString[33];
			for (int i = 0; i < 16; i++)
				sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);


			cout<< string(mdString)<<endl;
}

class Chunk{
	protected:
		uint64_t beg   = 0;
		uint64_t bytes = 0;
		string md5;
	public :
		const uint64_t static BUFFER_SIZE = 65536;
//        const uint64_t static BUFFER_SIZE = 1024;

		Chunk( uint64_t b, uint64_t by, string m) : beg(b), bytes(by), md5(m){}
};

class Lock{
	boost::mutex mtx1;
	boost::mutex mtx2;

	public :
		Lock(){}

		void lock1(){ mtx1.lock(); }
		void lock2(){ mtx2.lock(); }
		void unlock1(){ mtx1.unlock(); }
		void unlock2(){ mtx2.unlock(); }
};

class Worker{
	protected:
		char* data = NULL;
		uint64_t bytes = 0;
		uint64_t beg    =0;
		ifstream* inFile;

		Lock* lock      =NULL;
		deque<Chunk>* chunks = NULL;
	public:
		Worker(){}

		Worker(ifstream* f, deque<Chunk>* c,Lock* l ) : inFile(f), chunks(c), lock(l) {}

		~Worker(){
			if( data != NULL)
				delete[] data;
		}

		bool init(){
			lock->lock1();
			data    = new char[ Chunk::BUFFER_SIZE ];
			beg     = inFile->tellg();
			inFile->read (data, Chunk::BUFFER_SIZE);
			bytes   = inFile->tellg() - beg;
			lock->unlock1();

			return (*inFile) ? true: false;
		}

		string md5(){
			unsigned char digest[16];
			MD5_CTX ctx;
			MD5_Init(&ctx);
			MD5_Update (&ctx, data, bytes);
			MD5_Final(digest, &ctx);

			char mdString[33];
			for (int i = 0; i < 16; i++)
				sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);


			return string(mdString);
		}

		void operator ()(){
			while( init() ){
				string mdString  = md5();
				lock->lock2();
				Chunk( beg, bytes, mdString );
				chunks->push_back( Chunk( beg, bytes, mdString ) );
				lock->unlock2();
				delete[] data;
			}

		}
};

int main(){
		std::ifstream inFile ("/home/severus/Desktop/r.html", std::ifstream::binary);

		int n = 3;
		deque<Chunk> chunks;
		Lock l;
		boost::thread_group group;
		for(uint32_t i=0; i<n ; i++){
			Worker w(&inFile, &chunks, &l);
			group.create_thread( w );
		}
		group.join_all();
		
		
		map<string, Chunk> cache;
		
		return 0;
}
