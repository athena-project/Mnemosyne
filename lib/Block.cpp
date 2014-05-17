#include "Block.h"

namespace Athena{
    namespace Mnemosyne{

        /**
         *  ChunckManager
        **/
        BlockManager::BlockManager(){}
        BlockManager::~BlockManager(){}

        void BlockManager::insert( Block Block){
            mysqlpp::Query query = conn.query();
            query<<"INSERT INTO block";
            query.execute();
            if (mysqlpp::StoreQueryResult res = query.store())
                return;
            else{
                cerr << "Failed to get item list: " << query.error() << endl;
                throw "";
            }
        }

        vector<Block> BlockManager::get( string fieldsNeeded, string where, string order, string limit){
            vector< Block > Blocks;
            mysqlpp::Query query = conn.query();
            query << "SELECT " << fieldsNeeded <<" FROM block "<< where <<" "<< order<< " "<<limit;
            mysqlpp::StoreQueryResult res = query.store();

            for(size_t i = 0; i < res.num_rows(); ++i)
                Blocks.push_back( Block( (uint64_t)res[i]["id"] ) );

            return Blocks;
        }

        vector<Block> BlockManager::get(vector<uint64_t>& ids){
            string where = "id INTO (";

            for(vector<uint64_t>::iterator it = ids.begin(); it != ids.end(); it++){
                where += (it != ids.begin() ) ? "," : "";
                where += (*it);
            }
            return get( "*", where, "id", "");
        }

        Block BlockManager::get(uint64_t id){
            vector<Block> vect = get( "*", "id :="+id, "id", "");

            if(vect.size() == 1)
                return vect[0];
            else
                throw "Block not found";
        }
    }
}
