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

#ifndef BLOCK_H_INCLUDED
#define BLOCK_H_INCLUDED


#include <stdint.h>
#include <stdio.h> // remove file
#include <vector>

#include <mysql++/mysql++.h>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "Manager.h"
#include "Chunk.h"


using namespace std;


namespace Athena{
    namespace Mnemosyne{

        class Block{
            protected :
                uint64_t id;

            public :
                static const uint32_t CHUNKS = 2000; //2000 chuncks by block

                Block(){}
                Block( uint64_t id){ this->id = id; }

                uint64_t getId(){ return id; }
                void setId(uint64_t i){ id=i; }
        };

        class BlockManager : public Manager{
            protected :

            public :
                BlockManager();
                ~BlockManager();

                uint64_t insert( Block block );
                vector<uint64_t> insert( vector< Block > block );

                Block get( uint64_t id );
                vector<Block> get( string fieldsNeeded, string where, string order, string limit );
                vector<Block> get( vector<uint64_t>& ids ); //Order by id

                uint64_t count( string where="", string order="", string limit="" );
        };

        class BlockHandler{
            protected :
                vector<string> files;
                vector<string> directories;
            public :
                BlockHandler();
                ~BlockHandler();

                static string DIR(){ return ""; } ///Path of the tmp directory for block
                static string TMP_DIR(){ return ""; } ///Path of the directory for blovk

                /**
                 * @brief Extracts a chunk from a block
                 * @param block         - current block
                 * @param idChunk       - sql id of the chunk
                 * @return tmp location of the chunk
                 */
                string getChunk( Block& block, uint64_t idChunk );

                /**
                 * @brief Extracts chunks from a block
                 * @param block         -
                 * @return location of tmp directory
                 */
                string extract( Block& block );

                /**
                 * @brief Creates a block(now it's a compressed file) on DIR
                 * @param block         -
                 * @param location      - path of the tmpDir which hosts all the chunks of this block
                 */
                void make(Block& block, string location);

                /**
                 * @brief Creates all the blocks depending of the  number of chunks
                 */
                void makeBlocks();
        };
    }
}

#endif // BLOCK_H_INCLUDED
