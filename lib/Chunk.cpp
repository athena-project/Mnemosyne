#include "Chunk.h"

namespace Athena{
    namespace Mnemosyne{
        Chunk::Chunk(){}
        Chunk::Chunk( uint64_t id, uint64_t block_id){
            this->id = id;
            this->block_id = block_id;
        }



        /**
         *  ChunckManager
        **/

        /*vector<Chunk> ChunkManager::get( list<uint64_t> ids){
            QString str = "SELECT * FROM chunk WHERE id IN (";
            for(list<uint64_t>::iterator it=ids.begin(); it != ids.end(); it++)
                str +="?,";
            str[str.size()-1]=")";
            str+=")";

            QSqlQuery query;
            query.prepare(str);

            QVariantList ints;
            for(list<uint64_t>::iterator it=ids.begin(); it != ids.end(); it++)
                ints << (*it);
            q.addBindValue(ints);

            if (q.execBatch(){

            }else{
                std::cout << "Ça marche pas ! :(" << std::endl;
            }
        }*/
    }
}
