#include "Revision.h"


/**
 *  Revision
 */
	Revision::Revision(){
		root=this;
		initPtr();
	}

	Revision::Revision(uint16_t num) : n(num){
		initPtr();
	}

	Revision::Revision(uint16_t num, uint64_t id, uint64_t s, uint16_t d) : n(num), idBeginning(id), size(s), diff(d){
		initPtr();
	}

	void Revision::initPtr(){
		parent = NULL;
		previous = NULL;
		next = NULL;
		last = NULL;

		oStream=NULL;
		iStream=NULL;
	}

	Revision::~Revision(){
		for(int i=0; i<children.size(); i++)
			delete children[i];
	}

	vector< Revision* > Revision::getParents(){
		if( parent != NULL && this != this->getRoot() ){
			vector< Revision* > parents = parent->getParents();
			parents.push_back( this );
			return parents;
		}else
			return vector< Revision* >();
	}

/**
 *  Revision Handler
 */

	RevisionHandler::RevisionHandler(){}

	RevisionHandler::~RevisionHandler(){}

	uint16_t RevisionHandler::extractSizeTable( ifstream* stream ){
		if( stream == NULL )
			return 0;

		uint16_t number = 0;
		stream->seekg(-2, stream->end );
		stream->read((char*)&number, 2);

		return number;
	}

	vector<TableElement> RevisionHandler::extractTable(ifstream* stream){
		if( stream == NULL )
			return vector<TableElement>();

		vector<TableElement> table;
		uint16_t sizeTable = extractSizeTable( stream );

		int newOrigin = (-1)* ( sizeTable * Revision::REVISION_SIZE_TABLE + 2);//+2 : number of table element uint16_t
		stream->seekg(newOrigin , stream->end);

		for(uint16_t i=0 ; i<sizeTable ; i++ ){
			TableElement element;
			stream->read((char*)&element.idBeginning, 8);
			stream->read((char*)&element.size, 8);
			stream->read((char*)&element.diff, 2);
			stream->read((char*)&element.origin, 2);
			table.push_back( element );

			cout<<"extract table :: id "<<element.idBeginning<<endl;
			cout<<"extract table :: size "<<element.size<<endl;
			cout<<"extract table :: diff "<<element.diff<<endl;
			cout<<"extract table :: origin "<<element.origin<<endl;
		}

		return table;
	}

	vector<int> RevisionHandler::extractChildren( vector< int >& origines, int parent ){
		vector<int> children;
		for(int i=0; i<origines.size(); i++)
			if( parent == origines[i] )
				children.push_back( i );
		return children;
	}

	Revision* RevisionHandler::buildStructure( vector<TableElement>& table ){
		Revision* previous = new Revision();
		if( table.size() == 0 )
			return previous;

		Revision* current;
		vector<Revision*>* structur = new vector<Revision*>[ table.size() ]; /// Must not be deleted, it's used in revision tree
		///Previous and root
		for( int i=0 ; i<table.size() ; i++){
			current = new Revision( i+1, table[i].idBeginning, table[i].size, table[i].diff );
			current->setPrevious( previous );
			current->setRoot( previous->getRoot() );
			structur[ table[i].origin ].push_back( current );

			previous = current;
		}

		///Next and last
		previous = current->getPrevious();
		current->setLast( current );
		while( previous != NULL ){
			previous->setNext( current );
			previous->setLast( current->getLast() );

			previous = previous->getPrevious();
			current  = current->getPrevious();
		}

		///Children and parents
		current = current->getRoot();
		for( int i=0 ; i<table.size() ; i++){
			current->setChildren( structur[i] );
			for( int j=0 ; j<structur[i].size() ; j++)
				structur[i][j]->setParent( current );

			current = current->getNext();
		}

		return current->getRoot();
	}

	Mutation RevisionHandler::readMutation( ifstream& stream ){
		uint8_t type(0);
		uint64_t idBegining(0);
		uint64_t size(0);

		stream.read( (char *)&type, 1);
		stream.read( (char *)&idBegining, 8);
		stream.read( (char *)&size, 8);
		Mutation m( type, idBegining, size);
		return m;
	}

	void RevisionHandler::applyMutations( vector<char>& data, Revision* rev){
		if( rev == NULL || rev->getRoot() == rev )
			return;
		ifstream* stream = (rev->getIStream());
if (stream == NULL )
throw"";
		Mutation m;
		uint64_t i =0;
		stream->seekg( rev->getIdBeginning() - rev->getRelativeO(), stream->beg );

		while( i < rev->getSize() ){
			Mutation m=readMutation( *stream );
			m.apply(data, *stream);
			i+=m.getSize() + 17; //17 Bytes => header of a mutation
		}
	}

	void RevisionHandler::writeTable( vector<TableElement>& table, ofstream* stream){
		stream->seekp( 0, stream->end );
		for( uint64_t i=0; i<table.size(); i++){
			stream->write( (char*)&table[i].idBeginning, 8);
			stream->write( (char*)&table[i].size, 8);
			stream->write( (char*)&table[i].diff, 2);
			stream->write( (char*)&table[i].origin, 2);
		}

		///Add nbr of table element
		uint16_t s = table.size();
		stream->write( (char*)&s, 2);

		stream->flush();
	}

	void RevisionHandler::write( vector<char>& data, uint64_t pos, uint64_t length, ofstream* stream){
		for(uint64_t j=pos; j<length+pos; j++)
			(*stream)<<data[j];

		stream->flush();
	}

	uint64_t RevisionHandler::diff( vector<char>& origine, vector<char>& data ){
		uint64_t num= min( origine.size(), data.size() );
		uint64_t diff = max( origine.size(), data.size() ) - num;

		for(uint64_t i=0; i<num; i++){
			if( origine[i] != data[i] )
				diff++;
		}
		return diff;
	}

	vector< uint64_t > RevisionHandler::calculDifferences( Revision* rev,  vector<char>& data ){
		vector< char > tmpData;
		vector< uint64_t > differences;
		Revision* tmp = (rev->getRoot()->getNext()); //Diff with racine <=> rev origine

		while( tmp != NULL ){
			applyMutations( tmpData, tmp);
			differences.push_back( diff(tmpData, data) );
			tmp = tmp->getNext();
		}

		return differences;
	}

	Revision* RevisionHandler::bestOrigin( Revision* rev,  vector<char>& data ){
		vector< uint64_t > differences = calculDifferences( rev, data);
		uint64_t tmpDiff = 0;
		int j=0;
		cout << " diff      "<<differences.size()<<endl;


		if( differences.size() > 0)
			tmpDiff = data.size();
		else
			return rev;

		Revision* tmpRev = rev->getRoot();

		for( int i=0; i<differences.size(); i++){
			if( tmpDiff > differences[i] ){
				tmpDiff = differences[i];
				j++;
			}
		}

		for( int k=0; k<j; k++)
			tmpRev = tmpRev->getNext();

		return (tmpRev != NULL) ? tmpRev : rev;
	}

	void RevisionHandler::createMutations( vector<char>& origine, vector<char>& data, ofstream* stream, uint64_t pos){
		stream->seekp(pos, stream->end);

		uint8_t type = 0;
		uint64_t idBeginning = 0;
		uint64_t size = 0;

		//Update first
		uint64_t i(0);
		uint64_t n = min( origine.size(), data.size() );

		uint64_t length(0);
		while( i<n ){
			if( origine[i] !=  data[i] ){
				length++;
			}else if(length != 0){
				type = Mutation::UPDATE;
				idBeginning = i-length;
				size = length;

				stream->write( (char*)&type,1);
				stream->write( (char*)&idBeginning, 8);
				stream->write( (char*)&size, 8);
				write(data, i-length, length, stream);
				length=0;
			}
			i++;
		}

		//Delete
		if( origine.size()>data.size() ){
			type = Mutation::DELETE;
			idBeginning = data.size();
			size = origine.size() - data.size();

			stream->write( (char*)&type,1);
			stream->write( (char*)&idBeginning, 8);
			stream->write( (char*)&size, 8);
		}

		//Insert
		if( origine.size()<data.size() ){
			type = Mutation::INSERT ;
			idBeginning = origine.size();
			size = data.size() - origine.size();

			stream->write( (char*)&type,1);
			stream->write( (char*)&idBeginning, 8);
			stream->write( (char*)&size, 8);
			write(data, origine.size(), size, stream);
		}
		stream->flush();
	}

	Revision* RevisionHandler::newRevision( Revision* origin,  vector<char>&newData){
		uint32_t tableSize = extractSizeTable( origin->getIStream() );
		vector<TableElement> table = extractTable( origin->getIStream() );


		///Building of origin
		vector<char> tmpData;
		vector< Revision* > parents = origin->getParents();
		if( parents.size() > 0){
			for( int i=0; i<parents.size()-1 ; i++)
				applyMutations( tmpData, parents[i]);

			applyMutations( tmpData, origin); //Data is now hydrate
		}

		///Rev creation, size will be hydrate later
		Revision* rev = (origin->getLast() != NULL ) ? origin->getLast() : origin;
		Revision* newRev= new Revision( tableSize+1, rev->getIdBeginning()+rev->getSize(), 0, diff( tmpData, newData) );

		rev->addChild( newRev );
		newRev->setPrevious( rev );
		newRev->setParent( origin );
		newRev->setRoot( origin->getRoot() );
		newRev->setIStream( origin->getIStreamLocation() );
		newRev->setOStream( origin->getOStreamLocation() );
		newRev->setLast( newRev );

		createMutations( tmpData, newData, newRev->getOStream(), tableSize);

		///Size
		ofstream* oStream = origin->getOStream();
		oStream->seekp (0, oStream->end);
		uint64_t length = oStream->tellp();
		if( tableSize > 0 )
			newRev->setSize( length - rev->getIdBeginning()-rev->getSize()-tableSize*Revision::REVISION_SIZE_TABLE-2 );
		else
			newRev->setSize( length );

		///Maj of the table
		TableElement newElement;
		newElement.idBeginning = newRev->getIdBeginning();
		newElement.size = newRev->getSize();
		newElement.diff = newRev->getDiff();
		newElement.origin = origin->getN();

		table.push_back( newElement );

		writeTable( table, oStream );
		return newRev;
	}


