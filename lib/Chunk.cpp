#include "Chunk.h"


namespace Athena{
    namespace Mnemosyne{

        /**
         * Chunk
         */

        Chunk::Chunk(){}

        Chunk::Chunk( uint32_t size){
            this->id = -1;
            this->block_id = -1;
            this->size = size;
        }

        Chunk::Chunk( uint64_t id, uint64_t block_id){
            this->id = id;
            this->block_id = block_id;
            size=0;
        }

        Chunk::Chunk( uint64_t id, uint64_t block_id, uint32_t size) : Chunk(id, block_id){
            this->size = size;
        }



        /**
         *  ChunckManager
         */

        ChunkManager::ChunkManager(){}

        ChunkManager::~ChunkManager(){}

        uint64_t ChunkManager::insert( Chunk chunk){
            mysqlpp::Query query = conn.query();
            query<<"INSERT INTO chunk (block_id, size) VALUES ("<<chunk.getId();
            query<<","<<chunk.getBlock_id()<<","<<chunk.getSize()<< ");";

            if (mysqlpp::SimpleResult res = query.execute())
                return (uint64_t)res.insert_id();
            else{
                cerr << "Failed to get item list: " << query.error() << endl;
                throw "";
            }
        }

        vector<uint64_t> ChunkManager::insert( vector< Chunk > chunks ){
            vector<uint64_t> ids;

            for(vector<Chunk>::iterator it=chunks.begin(); it!=chunks.end(); it++){
                it->setId( insert( *it ) );
                ids.push_back( it->getId() );
            }

            return ids;
        }

        Chunk ChunkManager::get(uint64_t id){
            vector<Chunk> vect = get( "*", "id := "+id, "id", "");
            if( vect.size() == 1 )
                return vect[0];
            else
                throw "chunk not found";
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

        uint64_t ChunkManager::count( string where, string order, string limit ){
            vector<mysqlpp::Row> v;
            mysqlpp::Query query = conn.query();
            query << "SELECT COUNT(*) AS number FROM chunck "<< where <<" "<< order<< " "<<limit;
            mysqlpp::StoreQueryResult res = query.store();

            return res[0]["number"];

        }



        /**
          * ChunkHandler
          */
        ChunkHandler::ChunkHandler(){
            cManager = new ChunkManager();
        }

        ChunkHandler::~ChunkHandler(){
            delete cManager;

            for( int i=0; i<files.size(); i++)
                remove( files[i].c_str() );
        }

        string ChunkHandler::getFile( uint64_t id){
            BlockHandler bHandler;
            BlockManager bManager;

            Chunk currentChunk = cManager->get( id );
            std::ostringstream chunckId;
            chunckId<<id;

            Block currentBlock = bManager.get( currentChunk.getBlock_id() );

            string chunkLocation1 = bHandler.getChunk( currentBlock, id );
            string chunkLocation2 = ChunkHandler::TMP_DIR()+"/"+chunckId.str();

            fs::copy_file( chunkLocation1, chunkLocation2);
            files.push_back(chunkLocation2);
            return chunkLocation2;
        }

        void ChunkHandler::writeChunk(uint64_t id, ifstream& stream, uint64_t idBeginning, uint64_t size, string dir){
            std::ostringstream strId;
            strId<<id;

            string location = (dir == "" ) ? ChunkHandler::TMP_DIR()+"/"+strId.str() : dir+"/"+strId.str();
            ofstream oStream( location.c_str(), ios::trunc );

            stream.seekg( idBeginning );

            char tmpChar;
            uint64_t i=0;

            while( i<size && stream.get(tmpChar) )
                oStream<<tmpChar;
        }

        void ChunkHandler::updateData(Chunk c, ifstream& stream, uint64_t idBeginning, uint64_t size){
            BlockManager bManager;
            uint64_t tmpIdBlock = ceil( (float)c.getId() / (float)(Block::CHUNKS) );

            if( tmpIdBlock >= bManager.count() )
                writeChunk( c.getId(), stream, idBeginning, size);
            else{ //In a block
                BlockHandler bHandler;
                Block currentBlock = bManager.get( tmpIdBlock );
                string blockLocation = bHandler.extract( currentBlock );

                writeChunk( c.getId(), stream, idBeginning, size, blockLocation );
                bHandler.make( currentBlock, blockLocation);
            }
        }

        vector<Chunk> ChunkHandler::makeChunks( ifstream& stream, uint64_t idBeginning, uint64_t size ){
            ChunkManager cManager;
            uint64_t nbrNeeded = ceil( (float)size / (float)(Chunk::CHUNK_SIZE_MAX) );
            stream.seekg( idBeginning );

            vector<Chunk> chunks;
            for(uint64_t i=0; i<nbrNeeded; i++){
                if( i<nbrNeeded-1)
                    chunks.push_back( Chunk(Chunk::CHUNK_SIZE_MAX) );
                else
                    chunks.push_back( size-(nbrNeeded-1)*(Chunk::CHUNK_SIZE_MAX) );//Size less than max
            }

            vector<uint64_t> ids = cManager.insert( chunks );
            uint32_t chunkSize = Chunk::CHUNK_SIZE_MAX;
            for( uint64_t i=0; i<ids.size(); i++){
                chunks[i].setId( ids[i] );

                if( i=ids.size()-1 )
                    chunkSize = min( (uint64_t)Chunk::CHUNK_SIZE_MAX, size-(ids.size()-1)*(Chunk::CHUNK_SIZE_MAX) );

                writeChunk( ids[i], stream, idBeginning+i*(Chunk::CHUNK_SIZE_MAX), Chunk::CHUNK_SIZE_MAX);
            }

            return chunks;
        }


    }
}
