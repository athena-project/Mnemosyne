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

#ifndef REVISION_CPP_INCLUDED
#define REVISION_CPP_INCLUDED


#include <vector>
#include <list>
#include <string>
#include <fstream>
#include <stdint.h>
#include <algorithm>

#include "Chunk.h"
#include "Mutation.h"


using namespace std;


namespace Athena{
    namespace Mnemosyne{


        class Revision{


            protected :
                int n;                  ///N° of the current revision, -1 if this revision is a root
                uint64_t idBeginning = 0 ;   ///Index of the first char of this revision in iStream
                uint64_t size = 0;          ///Size of the data of the current revision
                uint32_t diff = 0;          ///E[%*10000] diff with the origin, which is relative to the current rev


                Revision* parent;
                vector< Revision* > children;
                Revision* previous;     ///revision n-1, if exists
                Revision* next;         ///revision n+1, if exists
                Revision* last;         ///Last revision added ie nmax
                Revision* root;

                string iStreamLocation;
                ifstream* iStream;      ///mutations's instructions
                string oStreamLocation;
                ofstream* oStream;      ///mutations's instructions

                uint64_t relativeO = 0;     ///Relative origin in the current flux if we only load the needed chunks

            public :
                static const uint32_t REVISION_SIZE_TABLE = 20; /// Bits , origine(uint16_t) idBeginning(uint64_t) size(uint64_t) diff(uint16_t)

                Revision();
                Revision(int num);
                Revision(int num, uint64_t id, uint64_t s, uint32_t d);
                void initPtr();
                ~Revision();

                int getN(){                 return n; }
                uint64_t getIdBeginning(){  return idBeginning; }
                uint64_t getSize(){         return size; }
                uint32_t getDiff(){         return diff; }


                /**
                 *  @return parents of the current revision order by n asc ie root first
                 */
                list< Revision* > getParents();
                Revision* getRoot(){        return root; }
                Revision* getLast(){        return last; }
                Revision* getNext(){        return next; }

                ifstream* getIStream(){      return iStream; }
                ofstream* getOStream(){      return oStream; }
                string getIStreamLocation(){    return iStreamLocation; }
                string getOStreamLocation(){    return oStreamLocation; }
                uint64_t getRelativeO(){    return relativeO; }

                void setSize( uint64_t s ){         size=s; }
                void setParent( Revision* rev ){    parent=rev; }
                void setPrevious( Revision* rev ){  previous=rev; }
                void setNext( Revision* rev ){      next=rev; }
                void setLast( Revision* rev ){      last=rev; }
                void setIStream( string s ){
                    iStreamLocation = s;
                    delete iStream;
                    iStream=new ifstream( s.c_str() );
                }
                void setOStream( string s ){
                    oStreamLocation = s;
                    delete oStream;
                    oStream=new ofstream( s.c_str(), ios::app );
                }
                void setRelativeO( uint64_t n ){    relativeO=n; }
                void setRoot( Revision* r ){        root = r; }

                void addChild( Revision* child ){ child->setRoot( root ); children.push_back(child); }
        };


        class RevisionHandler{
            public :
                RevisionHandler();
                ~RevisionHandler();

            ///Table functions

                /**
                 * @brief Return the size of the table in Bytes
                 * @param stream        - input file
                 * @return size of the table in bytes
                 */
                uint32_t extractSizeTable( ifstream* stream );

                /**
                 * @brief Return the table from the input file
                 * @param stream        - input file
                 * @return vector of char which is the table
                 */
                vector<char> extractTable(ifstream* stream);

                /**
                 * @brief Return the origin of the current revision
                 * @param table         - cf.table structure
                 * @param it            - current position of the cursor of table
                 * @return origin
                 */
                uint16_t getOrigine( vector<char>& table, vector<char>::iterator& it );

                /**
                 * @brief Return the beginning of the current revision
                 * @param table         - cf.table structure
                 * @param it            - current position of the cursor of table
                 * @return beginning
                 */
                uint64_t getIdBeginning( vector<char>& table, vector<char>::iterator& it );

                /**
                 * @brief Return the size of the current revision
                 * @param table         - cf.table structure
                 * @param it            - current position of the cursor of table
                 * @return size
                 */
                uint64_t getSize( vector<char>& table, vector<char>::iterator& it );

                /**
                 * @brief Return the diff of the current revision
                 * @param table         - cf.table structure
                 * @param it            - current position of the cursor of table
                 * @return diif
                 */
                uint16_t getDiff( vector<char>& table, vector<char>::iterator& it );

            ///Structure functions

                /**
                 * @brief Get the children's ids
                 * @param origines      - key is the id of a revision and value is the origin one
                 * @param parent        - id of the wanted origin
                 * @return ids
                 */
                vector<int> extractChildren( vector< int >& origines, int parent );

                /**
                 * @brief Get the children's ids
                 * @param origines      - key is the id of a revision and value is the origin one
                 * @param revision      - all the revisions already hydrated, order by id
                 * @param current       - the current revision
                 * @return make the link between the rev and itsw children
                 */
                void buildChildren( vector<int>& origines, vector< Revision* > revisions, Revision* current);

                /**
                 * Build the revision tree
                 * @param table         - number(uint16_t) origine(uint16_t) idBeginning(uint64_t) size(uint64_t) diff(float)...
                 */
                Revision* buildStructure( vector<char>& table );

            ///Building functions

                /**
                 *  @brief Return a mutation, the stream cursor is at the begining of the body of the mutation,
                 *  the header has been already read
                 *  @param stream       - location of the mutations' instructions
                 */
                Mutation readMutation( ifstream& stream );

                /**
                 * @brief Apply a revision to data
                 * @param data          -
                 * @param rev           -
                 */
                void applyMutations( vector<char>& data, Revision* rev);

            ///Write functions

                /**
                 * @brief Write the table a the end of a file
                 * @param table         - table's data
                 * @param stream        - file
                 */
                void writeTable( vector<char>& table, ofstream* stream);

                /**
                 * @brief Write a revision in a file
                 * @param data          - revision data
                 * @param pos           - beginning in the flux
                 * @param size          - size of the data
                 * @param stream        - file
                 */
                void write( vector<char>& data, uint64_t pos, uint64_t length, ofstream* stream);

            ///Create new rev functions

                /**
                 * @brief Add a revision in table
                 * @param cf.table structure
                 */
                void addTableElement( vector<char> table, uint64_t id, uint64_t size, uint16_t diff, uint16_t o);

                /**
                 * Calcul the difference between  origin and data
                 * @param origin        - origin's data
                 * @param data          - current's data
                 * @return E[%*10000] diff with the origin, which is relative to the current rev
                **/
                uint64_t diff( vector<char>& origine, vector<char>& data  );

                /**
                 * Calcul the differences between data and all the revision in the tree
                 * @param rev           - revision tree
                 * @param data          -
                 * @return vector of diff order by id
                 */
                vector< uint64_t > calculDifferences( Revision* rev,  vector<char>& data );

                /**
                 * @brief Return the best origin for the current data
                 * @param rev           - revision tree
                 * @param data          -
                 * @return  Revision which is the best origin
                 */
                Revision* bestOrigin( Revision* rev,  vector<char>& data );

                /**
                 * @brief Create the mutations needed to build data from origin
                 * @param origin        -
                 * @param data          -
                 * @param stream        - file where mutations are written
                 * @param pos           - id of the beginning in the flux
                 */
                void createMutations( vector<char>& origine, vector<char>& data, ofstream* stream, uint64_t pos);

                /**
                 * @brief Make a new revision from newData
                 * @param currentRev    - last revision of the tree
                 * @param newData
                 * @return the new Rev
                 */
                Revision* newRevision( Revision* currentRev, vector<char>& newData);









        };

    }
}

#endif // REVISION_CPP_INCLUDED
