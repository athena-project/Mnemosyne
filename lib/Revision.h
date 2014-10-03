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



struct TableElement{
		uint64_t idBeginning = 0 ;   ///Index of the first char of this revision in iStream
		uint64_t size = 0;          ///Size of the data of the current revision
		uint16_t diff = 0;
		uint16_t origin = 0;
};

class Revision{


	protected :
		uint16_t n = 0;                  ///N° of the current revision
		uint64_t idBeginning = 0 ;   ///Index of the first char of this revision in iStream
		uint64_t size = 0;          ///Size of the data of the current revision
		uint16_t diff = 0;          ///E[%*10000] diff with the origin, which is relative to the current rev


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
		Revision(uint16_t num);
		Revision(uint16_t num, uint64_t id, uint64_t s, uint16_t d);
		void initPtr();
		~Revision();

		uint16_t getN(){                 return n; }
		uint64_t getIdBeginning(){  return idBeginning; }
		uint64_t getSize(){         return size; }
		uint16_t getDiff(){         return diff; }


		/**
		 *  @return parents of the current revision order by n asc ie root first
		 */
		vector< Revision* > getParents();
		Revision* getRoot(){        return root; }
		Revision* getLast(){        return last; }
		Revision* getNext(){        return next; }
		Revision* getPrevious(){    return previous; }

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
		void setChildren( vector<Revision*> c ){ children=c; }


		void print(){
			cout<<"Revision : "<<endl;
			cout<<"N° "<<n<<endl;
			cout<<"IdBeginning  "<<idBeginning<<endl;
			cout<<"Size  "<<size<<endl;
			cout<<"=============="<<endl;
			if( next != NULL )
				next->print();

		}
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
		uint16_t extractSizeTable( ifstream* stream );

		/**
		 * @brief Return the table from the input file
		 * @param stream        - input file
		 * @return vector of char which is the table
		 */
		vector<TableElement> extractTable(ifstream* stream);

	///Structure functions

		/**
		 * @brief Get the children's ids
		 * @param origines      - key is the id of a revision and value is the origin one
		 * @param parent        - id of the wanted origin
		 * @return ids
		 */
		vector<int> extractChildren( vector< int >& origines, int parent );

		/**
		 * Build the revision tree
		 * @param table         - origine(uint16_t) idBeginning(uint64_t) size(uint64_t) diff(uin16_t)...
		 */
		Revision* buildStructure( vector<TableElement>& table );

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
		void writeTable( vector<TableElement>& table, ofstream* stream);

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
		 * @param origin    - origin of the new mutation
		 * @param newData
		 * @return the new Rev
		 */
		Revision* newRevision( Revision* origin, vector<char>& newData);
};


#endif // REVISION_CPP_INCLUDED
