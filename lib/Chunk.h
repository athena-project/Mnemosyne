#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include <list>
#include <vector>
#include <stdint.h>
#include <fstream>

#include <boost/filesystem.hpp>

#include "Manager.h"
#include "Block.h"

using namespace std;

namespace Athena{
    namespace Mnemosyne{

        sql_create_3(SqlChunk, 1, 3,
            mysqlpp::sql_bigint, id,
            mysqlpp::sql_bigint, block_id,
            mysqlpp::sql_int, size
        )

        class Chunk{
            protected :
                uint64_t id;
                uint64_t block_id;
                uint32_t size;
            public :
                static const uint32_t CHUNK_SIZE_MAX = 64*1024; //octect

                Chunk();
                Chunk( uint32_t size );
                Chunk( uint64_t id, uint64_t block_id);
                Chunk( uint64_t id, uint64_t block_id, uint32_t size);

                uint64_t getId(){ return id; }
                uint64_t getBlock_id(){ return block_id; }
                uint32_t getSize(){ return size; }
                SqlChunk* getSqlChunk();

                void setId( uint64_t i){ id=i; }
                void setBlock_id( uint64_t i){ block_id=i; }
                void setSize( uint64_t i){ size=i; }
        };

        class ChunkManager : public Manager{
            public :
                ChunkManager();
                ~ChunkManager();

                uint64_t insert( Chunk chunk);
                vector<uint64_t> insert( vector< Chunk > chunks );
                vector<Chunk> get( string fieldsNeeded, string where, string order, string limit);

                uint64_t count( string where="", string order="", string limit="" );
                /**
                 * Order by id
                **/
                vector<Chunk> get(vector<uint64_t> ids);
                Chunk get(uint64_t id);

        };

        class ChunkHandler {
            protected :
                vector< string > files; ///All the tmpFiles which was created by this instance
            public :
                ChunkHandler();
                ~ChunkHandler();

                static string TMP_DIR(){ return ""; }
// cree un tmp file et renvoit le chemin
                string getFile( uint64_t id );

                /**
				 * Update data of chunk
				 * @param c             - current chunk
				 */
                void updateData(Chunk c, ifstream& stream, uint64_t idBeginning, uint64_t size);

                /**
                 * Make a vector chunks from a stream, this vect represents the stream data
                 * @param stream        - data stream
                 * @param idBeginning   - relativ origin of the stream
                 * @return vector of new chunks
                 */
                vector<Chunk> makeChunks(ifstream& stream, uint64_t idBeginning, uint64_t size);

                void writeChunk(uint64_t id, ifstream& stream, uint64_t idBeginning, uint64_t size, string dir="");


        };
    }
}

#endif // CHUNK_H_INCLUDED
