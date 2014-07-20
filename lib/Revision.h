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

#include "Mutation.h"


using namespace std;


namespace Athena{
    namespace Mnemosyne{


        class Revision{


            protected :
                int n;                  ///N° of the current revision, -1 if this revision is a root
                uint64_t idBeginning;   ///Index of the first char of this revision in iStream
                uint64_t size;          ///Size of the data of the current revision
                uint32_t diff;          ///E[%*10000] diff with the origin, which is relative to the current rev


                Revision* parent;
                vector< Revision* > children;
                Revision* previous;     ///revision n-1, if exists
                Revision* next;         ///revision n+1, if exists
                Revision* last;         ///Last revision added ie nmax
                Revision* root;

                ifstream* iStream;      ///mutations's instructions
                ofstream* oStream;      ///mutations's instructions

                uint64_t relativeO = 0;     ///Relative origin in the current flux if we only load the needed chunks

            public :
                static const uint32_t REVISION_SIZE_TABLE = 20; /// Bits , origine(uint16_t) idBeginning(uint64_t) size(uint64_t) diff(uint16_t)

                Revision(){ root=this; n=-1;}
                Revision(int num) : n(num){ root = (n == -1 ) ? this : NULL; }
                Revision(int num, uint64_t id, uint64_t s, uint32_t d) : n(num), idBeginning(id), size(s), diff(d){ root = (n == -1 ) ? this : NULL; }
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
                uint64_t getRelativeO(){    return relativeO; }

                void setSize( uint64_t s ){         size=s; }
                void setParent( Revision* rev ){    parent=rev; }
                void setPrevious( Revision* rev ){  previous=rev; }
                void setNext( Revision* rev ){      next=rev; }
                void setLast( Revision* rev ){      last=rev; }
                void setIStream( ifstream* s ){      iStream=s; }
                void setOStream( ofstream* s ){      oStream=s; }
                void setRelativeO( uint64_t n ){    relativeO=n; }
                void setRoot( Revision* r ){        root = r; }

                void addChild( Revision* child ){ child->setRoot( root ); children.push_back(child); }
        };


        class RevisionHandler{
            public :
                RevisionHandler();
                ~RevisionHandler();

            ///Table function

                /**
                 * @brief Extraction function
                 * @param table         - cf.table structure
                 * @param it            - current position of the cursor of table
                 * @return
                 */
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

            ///Structure function
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

#endif // REVISION_CPP_INCLUDED
