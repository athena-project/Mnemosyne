#include "Chunk.h"

namespace Athena{
    namespace Mnemosyne{
        Chunk::Chunk(){}

        Chunk::Chunk( uint64_t id, uint64_t block_id) {
            this->id = id;
            this->block_id = block_id;
            size=0;
        }

        Chunk::Chunk( uint64_t id, uint64_t block_id, uint32_t size) : Chunk(id, block_id){
            this->size = size;
        }


        /**
         *  ChunckManager
        **/
        ChunkManager::ChunkManager(){}
        ChunkManager::~ChunkManager(){}


        void ChunkManager::insert( Chunk chunk){
            mysqlpp::Query query = conn.query();
            query<<"INSERT INTO chunk (block_id, size) VALUES ("<<chunk.getId();
            query<<","<<chunk.getBlock_id()<<","<<chunk.getSize()<< ");";
            query.execute();

            if (mysqlpp::StoreQueryResult res = query.store())
                return;
            else{
                cerr << "Failed to get item list: " << query.error() << endl;
                throw "";
            }
        }

        void ChunkManager::insert( vector< Chunk > chunks ){
            mysqlpp::Query query = conn.query();
            query<<"INSERT INTO chunk (block_id, size) VALUES ";

            for(vector<Chunk>::iterator it=chunks.begin(); it!=chunks.end(); it++){
                query << (it==chunks.begin()) ? "" : ",";
                query<<"("<<it->getBlock_id()<<","<<it->getSize()<<") ";
            }

            query.execute();

            if (mysqlpp::StoreQueryResult res = query.store())
                return;
            else{
                cerr << "Failed to get item list: " << query.error() << endl;
                throw "";
            }
        }


        vector<Chunk> ChunkManager::get( string fieldsNeeded, string where, string order, string limit){
            vector<mysqlpp::Row> v;
            vector< Chunk > chunks;
            mysqlpp::Query query = conn.query();
            query << "SELECT " << fieldsNeeded <<" FROM chunck "<< where <<" "<< order<< " "<<limit;
            mysqlpp::StoreQueryResult res = query.store();

            for(size_t i = 0; i < res.num_rows(); ++i)
                chunks.push_back( Chunk( (uint64_t)res[i]["id"], (uint64_t)res[i]["block_id"], (uint64_t)res[i]["size"] ) );

            return chunks;
        }

        vector<Chunk> ChunkManager::get(vector<uint64_t> ids){
            string where = "id INTO (";

            for(vector<uint64_t>::iterator it; it != ids.end(); it++){
                where += (it != ids.begin() ) ? "," : "";
                where += (*it);
            }
            return get( "*", where, "id", "");
        }
        Chunk ChunkManager::get(uint64_t id){
            vector<Chunk> vect = get( "*", "id := "+id, "id", "");
            if( vect.size() == 1 )
                return vect[0];
            else
                throw "chunk not found";
        }

        uint64_t ChunkManager::count( string where, string order, string limit ){
            vector<mysqlpp::Row> v;
            mysqlpp::Query query = conn.query();
            query << "SELECT COUNT(*) AS number FROM chunck "<< where <<" "<< order<< " "<<limit;
            mysqlpp::StoreQueryResult res = query.store();

            return res[0]["number"];

        }

        /**
          * ChunkHandler
        **/
        ChunkHandler::ChunkHandler(){}

        ChunkHandler::~ChunkHandler(){
            for( int i=0; i<files.size(); i++)
                remove( files[i].c_str() );
        }

        string ChunkHandler::getFile( uint64_t id){
            ChunkManager cManager;
            Chunk currentChunk = cManager.get( id );
            std::ostringstream chunckId;
            chunckId<<id;

            BlockManager bManager;
            BlockHandler bHandler;
            Block currentBlock = bManager.get( currentChunk.getBlock_id() );

            string chunkLocation1 = bHandler.getChunk( currentBlock, id );
            string chunkLocation2 = ChunkHandler::TMP_DIR()+"/"+chunckId.str();

            boost::filesystem::copy_file( chunkLocation1, chunkLocation2);
            files.push_back(chunkLocation2);
            return chunkLocation2;
        }

        //Not in a block yet
        void ChunkHandler::writeChunk(Chunk c, ifstream stream, uint64_t idBeginning, uint64_t size){
            std::ostringstream strId;
            strId<<c.getId();

            string location = ChunkHandler::TMP_DIR()+"/"+strId;
            ofstream oStream( location.c_str(), ios::trunc );

            stream.seekg( idBeginning );

            char tmpChar="";
            uint64_t i=0;

            while( i<size & stream.get(tmpChar) )
                oStream<<tmpChar;
        }

        void ChunkHandler::updateData(Chunk c, ifstream stream, uint64_t idBeginning, uint64_t size){
            BlockManager* bManager = new BlocManager();
            uint64_t tmpIdBlock = ceil( (float)c.getId() / (float)(Block::CHUNKS) );
            if( tmpIdBlock>=bManager->count() )
                writeChunk( c, stream, idBeginning, size);
            else{ //In a block



            }

        }


    }
}
