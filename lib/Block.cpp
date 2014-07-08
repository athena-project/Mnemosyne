#include "Block.h"

namespace Athena{
    namespace Mnemosyne{

        /**
         *  ChunckManager
        **/
        BlockManager::BlockManager(){}
        BlockManager::~BlockManager(){}

        uint64_t BlockManager::insert( Block Block){
            mysqlpp::Query query = conn.query();
            query<<"INSERT INTO block";

            if (mysqlpp::SimpleResult res = query.execute())
                return (uint64_t)res.insert_id();
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

        //Return location of the dir
        string BlockHandler::extract( Block& block ){
            std::ostringstream blockId;
            blockId << block.getId();

            //Temporary directory creation
            string blockLocation = BlockHandler::DIR()+"/"+blockId.str();
            string tmpDir = BlockHandler::TMP_DIR()+"/"+blockId.str();
            boost::filesystem::create_directory( boost::filesystem::path( tmpDir.c_str() ) );
            boost::filesystem::copy_file( blockLocation, tmpDir );

            //Unxz archive
            string cmd = "tar -Jxvf "+tmpDir+".tar.xz ";
            system( cmd.c_str() );

            return tmpDir;
        }
        // location => of the tmpDir(cf.extract)
        void BlockHandler::make(Block& block, string location){
            std::ostringstream blockId;
            blockId << block.getId();
            string blockLocation    = BlockHandler::DIR()+"/"+blockId.str();

            string cmd = "tar -Jcvf "+blockLocation+".tar.xz "+location;
            system( cmd.c_str() );
        }

        void BlockHandler::makeBlocks(){
            ChunkManager* cManager = new ChunkManager();
            BlockManager* bManager = new BlockManager();

            uint64_t nbrChunks = cManager->count();
            uint64_t nbrBlocks = bManager->count();

            uint64_t nbrNeeded = nbrChunks/(Chunk::CHUNK_SIZE_MAX) - nbrBlocks;

            vector<Block> blocks;
            for( uint64_t i = 0; i<nbrNeeded; i++)
                blocks.push_back( Block() );

            bManager->insert( blocks );

            string tmpBlockLocation = BlockHandler::TMP_DIR()+"/";
            string blockLocation    = BlockHandler::DIR()+"/";
            for( uint64_t i = 0; i<nbrNeeded; i++){
                std::ostringstream tmpId;
                tmpId<< blocks[i].getId();
                boost::filesystem::create_directory( tmpBlockLocation );

                for(uint64_t j=(nbrBlocks+i)*(Chunk::CHUNK_SIZE_MAX); j<(nbrBlocks+i+1)*(Chunk::CHUNK_SIZE_MAX) ; j++){
                    std::ostringstream tmpId2;
                    tmpId2<<j;

                    string sourceChunk = ChunkHandler::TMP_DIR()+"/"+tmpId2.str();
                    string destChunk = tmpBlockLocation+"/"+tmpId2.str();
                    boost::filesystem::path pathSource = boost::filesystem::path(sourceChunk.c_str());
                    boost::filesystem::path pathDest = boost::filesystem::path(destChunk.c_str());

                    boost::filesystem::copy_file( pathSource, pathDest );
                    remove( sourceChunk.c_str() );
                }

                string cmd = "tar -Jcvf "+blockLocation+".tar.xz "+tmpBlockLocation;
                system( cmd.c_str() );
                boost::filesystem::remove_all( tmpBlockLocation );
            }

            //Free memory
            delete cManager;
            delete bManager;

        }
    }
}
