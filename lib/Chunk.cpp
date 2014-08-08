#include "Chunk.h"


namespace Athena{
    namespace Mnemosyne{

        /**
         * Chunk
         */

        Chunk::Chunk(){}

        Chunk::Chunk( uint32_t size){
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
            query<<"INSERT INTO chunk (block_id, size) VALUES (";
            query<<chunk.getBlock_id()<<","<<chunk.getSize()<< ");";

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
            std::stringstream idStr;
            idStr<<id;

            vector<Chunk> vect = get( "*", "id = "+idStr.str(), "id", "");
            if( vect.size() == 1 )
                return vect[0];
            else
                throw runtime_error("chunk not found");
        }

        vector<Chunk> ChunkManager::get( string fieldsNeeded, string where, string order, string limit){
            vector<mysqlpp::Row> v;
            vector< Chunk > chunks;
            mysqlpp::Query query = conn.query();
            query << "SELECT " << fieldsNeeded <<" FROM chunk WHERE "<< where <<" ORDER BY "<< order<< " "<<limit;
            mysqlpp::StoreQueryResult res = query.store();

            ofstream tmp("/home/toor/Desktop/sql",ios::app);
            tmp<<query<<endl;
            for(size_t i = 0; i < res.num_rows(); ++i)
                chunks.push_back( Chunk( (uint64_t)res[i]["id"], (uint64_t)res[i]["block_id"], (uint64_t)res[i]["size"] ) );

            return chunks;
        }

        vector<Chunk> ChunkManager::get(vector<uint64_t> ids){
            string where = "id IN (";

            for(uint64_t i=0; i<ids.size(); i++){
                where += ( i != 0 ) ? "," : "";

                std::ostringstream id;
                id<<ids[i];
                where += id.str();
            }
            where += ")";
            return get( "*", where, "id", "");
        }

        uint64_t ChunkManager::count( string where, string order, string limit ){
            vector<mysqlpp::Row> v;
            mysqlpp::Query query = conn.query();
            query << "SELECT COUNT(*) AS number FROM chunck "<< where <<" "<< order<< " "<<limit;
            mysqlpp::StoreQueryResult res = query.store();

            return res[0]["number"];

        }

        void ChunkManager::update( Chunk chunk){
            mysqlpp::Query query = conn.query();
            query<<"UPDATE chunk SET block_id="<<chunk.getBlock_id();
            query<<", size="<<chunk.getSize();
            query<<" WHERE id="<< chunk.getId();

            if (mysqlpp::SimpleResult res = query.execute())
                return;
            else{
                cerr << "Failed to get item list: " << query.error() << endl;
                throw "";
            }
        }

        void ChunkManager::update( vector< Chunk > chunks ){
            for(vector<Chunk>::iterator it=chunks.begin(); it!=chunks.end(); it++)
                 update( *it );

            return;
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

            string chunkLocation2 = ChunkHandler::DIR()+"/"+chunckId.str();
            try{
                Block currentBlock = bManager.get( currentChunk.getBlock_id() );
                string chunkLocation1 = bHandler.getChunk( currentBlock, id );

                fs::copy_file( chunkLocation1, chunkLocation2);
                files.push_back(chunkLocation2);
                return chunkLocation2;
            }catch(runtime_error e){//if no block yet
                return chunkLocation2;
            }
        }

        void ChunkHandler::writeChunk(uint64_t id, ifstream& stream, uint64_t idBeginning, uint64_t size, string dir){
            std::ostringstream strId;
            strId<<id;

            string location = (dir == "" ) ? ChunkHandler::DIR()+"/"+strId.str() : dir+"/"+strId.str();
            ofstream oStream( location.c_str() );

            stream.seekg( idBeginning, stream.beg );

            char* buffer = new char[ size ];
            stream.read( buffer, size);
            oStream.write( buffer, size);
            delete[] buffer;

            oStream.flush();
            oStream.close();
        }

        void ChunkHandler::updateData(Chunk c, ifstream& stream, uint64_t idBeginning, uint64_t size, uint64_t offset){
            BlockManager bManager;
            BlockHandler bHandler;
            Block currentBlock;
            string blockLocation;
            uint64_t tmpIdBlock = ceil( (float)c.getId() / (float)(Block::CHUNKS) );
            bool flag = tmpIdBlock < bManager.count();
            idBeginning += offset; ///Table
            idBeginning -= c.getSize(); ///chunk must be overwrite

            std::ostringstream strId;
            strId<<c.getId();
            string location = ChunkHandler::DIR()+"/"+strId.str();

            if( flag ){ //In a block
                currentBlock = bManager.get( tmpIdBlock );
                blockLocation = bHandler.extract( currentBlock );
                location = bHandler.extract( currentBlock )+"/"+strId.str();
            }


            ofstream oStream( location.c_str());
            stream.seekg( idBeginning, stream.beg );


            ///Write first part of the chunk
            char* buffer = new char[ c.getSize()-offset ];
            stream.read( buffer, c.getSize()-offset);
            oStream.write( buffer, c.getSize()-offset);
            delete[] buffer;

            ///Skip offset's bytes
            stream.seekg( offset, stream.cur );

            ///Write last part of the chunk
            char* buffer2 = new char[ size ];
            stream.read( buffer2, size);
            oStream.write( buffer2, size);
            delete[] buffer2;

            oStream.flush();
            oStream.close();

            if( flag )
                bHandler.make( currentBlock, blockLocation);

            ///SQL UPDATE
            c.setSize( c.getSize()-offset+size );
            cManager->update( c );
        }

        vector<Chunk> ChunkHandler::makeChunks( ifstream& stream, uint64_t idBeginning, uint64_t size ){
            ChunkManager cManager;
            uint64_t nbrNeeded = ceil( (float)size / (float)(Chunk::CHUNK_SIZE_MAX) );
            stream.seekg( idBeginning );

            ///Chunks creation
            vector<Chunk> chunks;
            for(uint64_t i=0; i<nbrNeeded; i++){
                if( i<nbrNeeded-1)
                    chunks.push_back( Chunk(Chunk::CHUNK_SIZE_MAX) );
                else
                    chunks.push_back( Chunk( size-(nbrNeeded-1)*(Chunk::CHUNK_SIZE_MAX)) );//Size less than max
            }

            ///SQL insertion and writting on hard drive
            vector<uint64_t> ids = cManager.insert( chunks );
            uint32_t chunkSize = Chunk::CHUNK_SIZE_MAX;
            for( uint64_t i=0; i<ids.size(); i++){
                chunks[i].setId( ids[i] );

                if( i == ids.size()-1 )
                    chunkSize = min( (uint64_t)Chunk::CHUNK_SIZE_MAX, size-(ids.size()-1)*(Chunk::CHUNK_SIZE_MAX) );
                else
                    chunkSize = Chunk::CHUNK_SIZE_MAX;

                writeChunk( ids[i], stream, idBeginning+i*(Chunk::CHUNK_SIZE_MAX), chunkSize);
            }

            return chunks;
        }


    }
}
