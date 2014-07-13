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

                uint64_t relativeO = 0;     ///Origine relative du flux ie debut de la rev
                utilité a cherchée

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


                /**
                 * @brief Extract all the diff from the current revision to the end, (ie used mostly with root)
                 * @param vect          - vector which will stores the diffs, order by asc
                 */
                void getAllDiffs( vector< uint32_t >& vect );



                pair<uint32_t, uint32_t> getExtremaDiff();

                /*
                 * Build the current revision(data) in the argument, if we keep the result in RAM
                 * @param data     -
                 */
                void hydrate( vector<bool>& data );

        };

    }
}

#endif // REVISION_CPP_INCLUDED
