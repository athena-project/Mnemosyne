/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * @autor Severus21
 */

#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED


#include <list>
#include <vector>
#include <fstream>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "Manager.h"
#include "Xz.h"

using namespace std;


namespace Athena{
    namespace Mnemosyne{

        class Chunk{
            protected :
                uint64_t id = 0;
                uint32_t size =0 ;
            public :
                static const uint32_t CHUNK_SIZE_MAX = 64*1024; //Bits

                Chunk();
                Chunk( uint32_t size );
                Chunk( uint64_t id, uint32_t size);

                uint64_t getId(){ return id; }
                uint32_t getSize(){ return size; }

                void setId( uint64_t i){ id=i; }
                void setSize( uint64_t i){ size=i; }
        };

        class ChunkManager : public Manager{
            public :
                ChunkManager();
                ~ChunkManager();

                uint64_t insert( Chunk chunk);
                vector<uint64_t> insert( vector< Chunk > chunks );


                Chunk get(uint64_t id);
                vector<Chunk> get( string fieldsNeeded, string where, string order, string limit);
                vector<Chunk> get(vector<uint64_t> ids); //Order by id

                void update( Chunk chunk);
                void update( vector< Chunk > chunks );


                uint64_t count( string where="", string order="", string limit="" );
        };

        class ChunkHandler : public Xz {
            protected :
                ChunkManager* cManager;

                vector< string > files; ///All the tmpFiles which are created by this instance

            public :
                ChunkHandler();
                ~ChunkHandler();

                static string DIR(){ return "/home/toor/Desktop/chunks"; } ///Path of the directory for chunk
                static string TMP_DIR(){ return "/home/toor/Desktop/chunks/tmp"; }

                /**
                 * @brief Create a tmp file from a chunk
                 * @param id            - sql id of the chunk
                 * @return tmp path
                 */
                string getFile( uint64_t id );

                /**
                 * @brief Mv tmp file to file(compressed)
                 * @param id            - sql id of the chunk
                 */
                void save( uint64_t id);


                /**
                 * @brief Write the data of the chunk
                 * @param id            - sql id of the chunk
                 * @param stream        - data
                 * @param idBeginning   - location in stream
                 * @param size          - size of the data
                 */
                void writeChunk(uint64_t id, ifstream& stream, uint64_t idBeginning, uint64_t size);

                /**
				 * @brief Update data of chunk
				 * @param c             - current chunk
				 * @param stream        - new data
				 * @param idBeginning   - location in stream
				 * @param size          - size of the data, which will be added to the chunk
				 * @param offset        - bytes not written again
				 */
                void updateData(Chunk c, ifstream& stream, uint64_t idBeginning, uint64_t size, uint64_t offset);

                /**
                 * @brief Make a vector chunks from a stream, this vect represents the stream data
                 * @param stream        - data stream
                 * @param idBeginning   - relativ origin of the stream
                 * @return vector of new chunks
                 */
                vector<Chunk> makeChunks(ifstream& stream, uint64_t idBeginning, uint64_t size);



        };
    }
}

#endif // CHUNK_H_INCLUDED
