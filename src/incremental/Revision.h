#ifndef REVISION_CPP_INCLUDED
#define REVISION_CPP_INCLUDED


#include <vector>
#include <list>
#include <string>
#include <fstream>
#include <stdint.h>
#include <algorithm>
#include <utility>

#include "Mutation.h"


using namespace std;

vector<uint64_t> methodes = { 1, 32, 64, 1024, 4096 }; /// block size
int METHODE_STD		= 0;
int METHODE_LINE	= -1;
double MAX_RATIO_ALLOWED = 0.85; //test needed

struct TableElement{
		uint64_t idBeginning    = 0;    ///Index of the first char of this revision in iStream
		uint64_t size           = 0;    ///Size of the data of the current revision
		uint16_t diff           = 0;
		uint16_t origin         = 0;
};


/**
 * Revision - is like a snapshot of a document a precise time
 */
class Revision{
	protected :
		uint16_t n              = 0;    ///N° of the current revision
		uint64_t idBeginning    = 0;    ///Index of the first char of this revision in iStream
		uint64_t size           = 0;    ///Size of the data of the current revision

		Revision* parent        = NULL;
		vector< Revision* > children;
		Revision* previous      = NULL; ///revision n-1, if exists
		Revision* next          = NULL; ///revision n+1, if exists
		Revision* last          = NULL; ///Last revision added ie nmax
		Revision* root          = NULL; ///@warning : to destroy a whole tree, the root must be deleted
 
		uint64_t relativeO      = 0;    ///Relative origin in the current flux if we only load the needed chunks

	public :
		static const uint32_t REVISION_SIZE_TABLE = 18; /// Bits , origine(uint16_t) idBeginning(uint64_t) size(uint64_t)

		Revision(){ 
			root=this; 
			last=this; 
		}
		
		Revision(uint16_t num) : Revision(), n(num){} 
		Revision(uint16_t num, uint64_t id, uint64_t s) : Revision(num), idBeginning(id), size(s){}
		
		~Revision(){
			for(int i=0; i<children.size(); i++)
				delete children[i];
		}
		
		uint16_t getN(){            return n; }
		uint64_t getIdBeginning(){  return idBeginning; }
		uint64_t getSize(){         return size; }
		/**
		 *  @return parents of the current revision order by n asc ie root first
		 */
		vector< Revision* > getParents(){
			if( parent == NULL || this == this->getRoot() )
				return vector< Revision* >();
			
			vector< Revision* > parents = parent->getParents();
			parents.push_back( this );
			return parents;
		}
		
		Revision* getRoot(){        return root; }
		Revision* getLast(){        return last; }
		Revision* getNext(){        return next; }
		Revision* getPrevious(){    return previous; }

		uint64_t getRelativeO(){    return relativeO; }

		void setSize( uint64_t s ){         size=s; }
		void setParent( Revision* rev ){    parent=rev; }
		void setPrevious( Revision* rev ){  previous=rev; }
		void setNext( Revision* rev ){      next=rev; }
		void setLast( Revision* rev ){      last=rev; }
		
		void setRelativeO( uint64_t n ){    relativeO=n; }
		void setRoot( Revision* r ){        root = r; }

		void addChild( Revision* child ){ 
			child->setRoot( root );
			children.push_back(child); 
		}
		void setChildren( vector<Revision*> c ){ children=c; }
		
		Revision* get( int num){
			if(n==num)
				return this
			else if(n<num)
				return next->get( num );
			else
				return previous->get( num );
		}
		
		/**
		 * Print the revision tree
		 */
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



/**
 * Represente the active copie of the data
 * store in active memory or not depending of the size of the data, for now we assume that file is lower than 1GB 
 */
class WorkingRev{
	protected:
		uint64_t size = 0;
		
		///Internal storage
		vector<char> v_data;
		//memory map file boost 
	public:
		WorkingRev( const char* location){
			fstream f( location );
			f.seekg( 0, f.end() );
			int size = f.tellg();
			char c;
			v_data = vector<char>( size );
			
			for( int i=0; i<size; i++){
				f.get( c ); 
				v_data[i] = c;
			}
		}
		
		~WorkingReb(){}
		
		char& operator[] (uint64_t i)  {  
			return v_data[i];
		} 
		
		void empty(){ return size == 0; }
		
		uint64_t size(){ return size; }
		
		void add(char c);
		
		void add(char* data, uint64_t size);
		
		/**
		 * erase[begin;begin+size[
		 */
		void erase(uint64_t begin, uint64_t size);
		
		void update( uint64_t begin, uint64_t size, char* data );
};

/**
 * Handler a descriptor file
 * Handle the convertion from a descriptor file (which contain all the tree) to a revision, and to a revision from a descriptor file
 */
class RevisionHandler{
	protected:
		fstream descriptor; 
		
		string location=""; ///descriptor location
		
		uint16_t number = 0; ///Number of revision
		uint32_t table_length =0; ///Lenght of table (bytes)
		vector<TableElement> table; ///Table of revisions
		Revision* tree = NULL;
		
	public :
		static const uint64_t BUFFER_LENGTH = 64; //depend on the average lenght of mutation
		
		RevisionHandler(){}
		
		RevisionHandler( const char* l ) : location(l){
			descritpor = fstream( l );
		}
		
		/**
		 * @brief Minimal construct -> read only Handler
		 */
		RevisionHandler(fstream& d, Revision* t) : descriptor(d), tree(t){}
		
		
		
		~RevisionHandler(){
			descriptor.close();
			delete tree->getRoot();
		}

		void setDescriptor( fstream d ){ descriptor = d; }

	///Structure functions

		/**
		 * @brief extract the number of revisions store in the descriptor
		 */
		void extractNumber();

		/**
		 * @brief extract the table of revisions from the input file
		 */
		void extractTable();

		/**
		 * @brief Get the children's ids
		 * @param origines      - key is the id of a revision and value is the origin one
		 * @param parent        - id of the wanted origin
		 * @return ids
		 */
		vector<int> extractChildren( vector< int >& origines, int parent );

		/**
		 * Build the revision tree from a linear table
		 * @param table         - origine(uint16_t) idBeginning(uint64_t) size(uint64_t) diff(uin16_t)...
		 */
		void buildStructure();

	///Building functions

		/**
		 * @brief Apply a tree to workingRev
		 * @param workingRev        -
		 */
		void applyMutations( WorkingRev& workingRev);
		
		/**
		 * @brief Apply a tree to data from current node to the nem count from the current number
		 * @param workingRev        -
		 * @param n_rev      		-
		 */
		void applyLinear( WorkingRev& workingRev , int n_rev);
		
		/**
		 * @brief Apply all parents of the tree and tree to workingRev
		 * @param workingRev         -
		 */
		void applyMutations( WorkingRev& workingRev);
		
	///Write functions

		/**
		 * @brief Write the table a the end of a file
		 * @param table         - table's data
		 * @param stream        - file
		 */
		void writeTable( vector<TableElement>& table, fstream* stream);

	///Diff functions
		/**
		 * Split a vector in line
		 * @param current		- data of the new rev
		 * @return vector of index of beginning line ( an abstract line beginnin at the end of the data)
		**/
		vector< uint64_t > stripLine( WorkingRev& current );

		/**
		 * Calcul the difference between  origin and data by line in bytes
		 * @param origin        - origin's data
		 * @param data          - current's data
		 * @return diff with the origin, which is relative to the current rev
		**/
		uint64_t diffLine( WorkingRev& origine, WorkingRev& data);


		/**
		 * Calcul the difference between  origin and data by block in bytes
		 * @param origin        - origin's data
		 * @param data          - current's data
		 * @param method		- block type
		 * @return diff with the origin, which is relative to the current rev
		**/
		uint64_t diffBlock( WorkingRev& origine, WorkingRev& data, int method=METHODE_STD );


		/**
		 * Calcul the difference between  origin and data, used as an interface for diffLine and diffBlock
		 * @param origin        - origin's data
		 * @param data          - current's data
		 * @param method		- block type (or line)
		 * @return diff with the origin, which is relative to the current rev
		**/
		uint64_t diff( WorkingRev& origine, WorkingRev& data, int method=METHODE_STD);

		/**
		 * Calcul the differences between data and all the revision in the tree for all the methods,
		 * so will build iterativly the rev, then delete it. Will calcul a vector of diff for each rev indeed will used all 
		 * the method know 
		 * @param data          -
		 * @return vector of diff order by id
		 */
		vector< vector<uint64_t> > calculDifferences( WorkingRev& data );

		/**
		 * @brief Return the best origin for the current data and the best method
		 * @param data          -
		 * @return  Revision which is the best origin, with the best method
		 */
		pair<Revision*, int> bestOrigin( WorkingRev& data );
	
	///Creation functions
	
		/**
		 * @brief Create the mutations needed to build data from origin
		 * @param origin        -
		 * @param data          -
		 * @param stream        - file where mutations are written
		 * @param pos           - id of the beginning in the flux
		 * @param method		- block type
		 */
		void createBlockMutations( WorkingRev& origine, WorkingRev& data, uint64_t pos, int method=METHODE_STD);

		/**
		 * @brief Create the mutations needed to build data from origin
		 * @param origin        -
		 * @param data          -
		 * @param stream        - file where mutations are written
		 * @param pos           - id of the beginning in the flux
		 * @param method		- block type
		 */
		void createLineMutations( WorkingRev& origine, WorkingRev& data, uint64_t pos);

		/**
		 * @brief Create the mutations needed to build data from origin, used as an interface
		 * @param origin        -
		 * @param data          -
		 * @param pos           - id of the beginning in the flux
		 * @param method		- block type
		 */
		void createMutations( WorkingRev& origine, WorkingRev& data,  uint64_t pos, int method=METHODE_STD);

		/**
		 * @brief Make a new revision from newData
		 * @param method    - block type (or line)
		 * @param newData
		 */
		void newRevision( WorkingRev& newData);
};


#endif // REVISION_CPP_INCLUDED
