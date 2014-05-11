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
        ChunkManager::ChunkManager() : Manager(){}

        void ChunkManager::insert( Chunk chunk){
            mysqlpp::Query query = conn.query();
            query<<"INSERT INTO chunk (block_id, size) VALUES ("<<chunk.getId();
            query<<","<<chunk.getBlock_id()<<","<<chunk.getSize()<< ");";
            query.execute();

            if (mysqlpp::StoreQueryResult res != query.store()) {
                cerr << "Failed to get item list: " << query.error() << endl;
                throw "";
            }
        }

        void ChunkManager::insert( vector< Chunk > chunks ){
            mysqlpp::Query query = conn.query();
            query<<"INSERT INTO chunk (block_id, size) VALUES ";

            for(vector<Chunk>::iterator it=chunks.begin(); it!=chunks.end(); it++){
                query << (it==chunks.begin()) ? "" : ",";
                query<<"("<<it->getId()<<","<<it->getBlock_id()<<","<<it->getSize()<<") ";
            }

            query.execute();

            if (mysqlpp::StoreQueryResult res != query.store()) {
                cerr << "Failed to get item list: " << query.error() << endl;
                throw "";
            }
        }


        vector<Chunk> ChunckManager::get( string fieldsNeeded, string where, string order, string limit){
            vector<mysqlpp::Row> v;
            vector< Chunk > chunks;
            mysqlpp::Query query = conn.query();
            query << "SELECT " << fieldsNeeded <<" FROM chunck "<< where <<" "<< order<< " "<<limit;
            query.storein(v);

            for (vector<mysqlpp::Row>::iterator it = v.begin(); it != v.end(); ++it)
                chunks.push_back( Chunk( (uint64_t)it->at("id"), (uint64_t)it->at("block_id"), (uint64_t)it->at("size") ) );

            return chunks;
        }

    }
}
