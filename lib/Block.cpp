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


        uint64_t BlockManager::count( string where, string order, string limit ){
            vector<mysqlpp::Row> v;
            mysqlpp::Query query = conn.query();
            query << "SELECT COUNT(*) AS number FROM block "<< where <<" "<< order<< " "<<limit;
            mysqlpp::StoreQueryResult res = query.store();

            return res[0]["number"];

        }

        /**
            Block Handler
        **/

        BlockHandler::BlockHandler(){}
        BlockHandler::~BlockHandler(){
            for( int i=0; i<files.size(); i++)
                remove( files[i].c_str() );
        }

        string BlockHandler::getChunk( Block& block, uint64_t idChunk ){
            std::ostringstream blockId;
            std::ostringstream chunckId;
            blockId << block.getId();
            chunckId << idChunk;


            string firstLocation = blockId.str()+"/"+chunckId.str() ; //Relative
            string newLocation   =  BlockHandler::TMP_DIR()+"/"+chunckId.str(); //Absolue
            string cmd = "tar -Jxvf "+BlockHandler::DIR()+"/"+blockId.str()+".tar.xz "+firstLocation;
            system( cmd.c_str() );

            boost::filesystem::copy_file( BlockHandler::DIR()+"/"+firstLocation, newLocation);
            remove( (BlockHandler::DIR()+"/"+firstLocation).c_str() );

            return newLocation;
        }

        void BlockHandler::makeBlocks(){
            ChunkManager cManager();
            BlockManager bManager();

            uint64_t nbrChunks = cManager.count();
            uint64_t nbrBlocks = bManager.count();

            uint64_t nbrNeeded = nbrChunks/(Chunk::CHUNK_SIZE_MAX) - nbrBlocks;

            vector<Block> blocks;
            for( uint64_t i = 0; i<nbrNeeded; i++)
                blocks.push_back( Block() );

            bManager.insert( blocks );

            string tmpBlockLocation = BlockHandler::TMP_DIR()+"/"+tmpId;
            string blockLocation    = BlockHandler::DIR()+"/"+tmpId;
            for( uint64_t i = 0; i<nbrNeeded; i++){
                std::ostringstream tmpId;
                tmpId<< blocks[i].getId();
                boost::filesystem::create_directory( tmpBlockLocation );

                for(uint64_t i=(nbrBlocks+i)*(Chunk::CHUNK_SIZE_MAX); <(nbrBlocks+i+1)*(Chunk::CHUNK_SIZE_MAX) ; i++){
                    std::ostringstream tmpId2;
                    tmpId2<<i;
                    if( boost::filesystem::copy_file( ChunkManager::TMP_DIR()+"/"+tmpId2.str(), tmpBlockLocation+"/"+tmpId2.str() ) )
                        remove( ChunkManager::TMP_DIR()+"/"+tmpId2.str() );
                }
                system( "tar -Jcvf "+blockLocation+".tar.xz "+tmpBlockLocation );
                boost::filesystem::remove_all( tmpBlockLocation );

            }


        }
    }
}
