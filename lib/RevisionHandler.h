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


#ifndef REVISIONHANDLER_H_INCLUDED
#define REVISIONHANDLER_H_INCLUDED


#include <vector>
#include <string>
#include <stdint.h>
#include <cmath>
#include <fstream>

#include "Revision.h"
#include "Mutation.h"
#include "Chunk.h"


using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class RevisionHandler{
            public :
                RevisionHandler();
                ~RevisionHandler();
                /**  Extraction function
                 *  @param it   -   current position of the cursor of table
                **/
                int getNumber( vector<char>& table, vector<char>::iterator& it  );
                uint16_t getOrigine( vector<char>& table, vector<char>::iterator& it );

                uint64_t getIdBeginning( vector<char>& table, vector<char>::iterator& it );
                uint64_t getSize( vector<char>& table, vector<char>::iterator& it );
                uint16_t getDiff( vector<char>& table, vector<char>::iterator& it );

                /**
                 * @return size of the table in bytes
                **/
                uint32_t extractSizeTable( ifstream& stream );


                vector<int> extractChildren( vector< int >& origines, int parent );
                void buildChildren( vector<int>& origines, vector< Revision* > revisions, Revision* current);

                vector<char> extractTable(ifstream& stream);
                /**
                 * Build the structur(ie no data)
                 * @param table     - number(uint16_t) origine(uint16_t) idBeginning(uint64_t) size(uint64_t) diff(float)...
                **/
                Revision* build( vector<bool>& table );





                void write( vector<char>& data, uint64_t pos, uint64_t length, ofstream& stream);
                void writeTable( vector<char>& table, ofstream& stream);
                void createdMutations( vector<char>& origine, vector<char>& data, ofstream& stream, uint64_t pos);

                /**
                 * Calcul the difference between  origin and data, if we keep the result in RAM
                 * @param origin     -
                 * @param data     -
                **/
                uint64_t diff( vector<char>& origine, vector<char>& data  );

                /**
                 *  Return a mutation, the stream cursor is at the begining of the body of the mutation the header
                 *  has been already read
                 *  @param stream - location of the mutations' instructions
                **/
                Mutation readMutation( ifstream& stream );

                void applyMutations( vector<char>& data, Revision* rev);
                vector< uint64_t > calculDifferences( Revision* rev,  vector<char>& data );
                Revision* bestOrigin( Revision* rev,  vector<char>& data );

                Revision* newRevision( Revision* currentRev, vector<char>& newData);

                Revision* buildStructure( vector<char>& table );

                void addTableElement( vector<char> table, uint64_t id, uint64_t size, uint16_t diff, uint16_t o);
        };
    }
}

#endif // REVISIONHANDLER_H_INCLUDED
