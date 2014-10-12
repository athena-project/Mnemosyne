#include "Revision.h"


	vector<uint64_t> methodes = { 1, 32 }; /// block size
	int METHODE_STD		= 0;
	int METHODE_LINE	= 10;


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
        if( oStream != NULL)
            delete oStream;
        if( iStream != NULL)
            delete iStream;
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
		vector< vector<Revision*> > structur =  vector< vector<Revision*> >( table.size() ); /// Must not be deleted, it's used in revision tree
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

		Mutation m;
		uint64_t i =0;
		stream->seekg( rev->getIdBeginning() - rev->getRelativeO(), stream->beg );
		while( i < rev->getSize() ){
			Mutation m=readMutation( *stream );
			m.apply(data, *stream);
			i+=m.getSize() + Mutation::HEADER_SIZE;
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

	vector< uint64_t > RevisionHandler::stripLine( vector<char>& d ){ ///vector[last char of the line+1 ie begin of the new line]
		vector< uint64_t > lines;
		uint64_t size = 0;
		if(d.empty())
			return lines;

		lines.push_back(0);
		for(uint64_t i=0 ; i<d.size() ; i++){
			if( d[i]==10 )
                lines.push_back( i+1 );
		}
		if( lines.back() != d.size())///abstract line
			lines.push_back( d.size() );

		return lines;
	}

	uint64_t RevisionHandler::diffLine( vector<char>& origine, vector<char>& data){
		vector< uint64_t > lineO = stripLine( origine );
		vector< uint64_t > lineD = stripLine( data );

		uint64_t num = min( lineO.size(), lineD.size() );
		uint64_t diff = 0;

		uint64_t lastBegO(0), lastBegD(0);
		uint64_t j(0), m(0);
		bool flag			= true;
		for(uint64_t i=0 ; i<num ; i++){
			flag = ( lineO[i]- lastBegO == lineD[i]- lastBegD );//Size comparison

			j=0;
			m=min( lineO[i]- lastBegO, lineD[i]- lastBegD);
			while(flag && j<m){
				flag =( origine[ lastBegO + j ] == data[ lastBegD + j ] );
				j++;
			}

			diff += (flag) ? 0 : lineD[i] - lastBegD;
			lastBegO	= lineO[i];
			lastBegD	= lineD[i];
		}


		if( lineO.size() > lineD.size() )
			diff += Mutation::HEADER_SIZE;
		if( lineO.size() < lineD.size() )
			diff += Mutation::HEADER_SIZE + lineO.back()-lineD.back();
		return diff;

	}

	uint64_t RevisionHandler::diffBlock( vector<char>& origine, vector<char>& data, int method ){
		uint64_t num= min( origine.size(), data.size() );
		uint64_t diff = max( origine.size(), data.size() ) - num;

		uint64_t blockSize 	= methodes[ method ];
		uint64_t j			= 0;
		bool flag			= true;

		for(uint64_t i=0; i<num; i+=blockSize){
			flag 	= true; j		= 0;
			while( j<blockSize && flag){
				if( origine[ i+j ] != data[ i+j ] ){
					diff	+=	blockSize;
					flag 	=	false;
				}
				j++;
			}
		}
		return diff;
	}

    uint64_t RevisionHandler::diff( vector<char>& origine, vector<char>& data, int method){
        if(method == METHODE_LINE)
			diffLine( origine, data);
		else
			diffBlock( origine, data, method);
	}

	vector< vector<uint64_t> > RevisionHandler::calculDifferences( Revision* rev,  vector<char>& data ){
		vector< char > tmpData;
		vector< vector<uint64_t> > differences;
		vector< uint64_t > tmpDifferences =  vector< uint64_t >( methodes.size()+1 );
		Revision* tmp = (rev->getRoot()->getNext()); //Diff with racine <=> rev origine

		while( tmp != NULL ){
			applyMutations( tmpData, tmp);

			tmpDifferences =  vector< uint64_t >( methodes.size()+1 );
			for(int i=0; i<methodes.size() ; i++)
				tmpDifferences[i] =  diff(tmpData, data, i);
			tmpDifferences[ methodes.size() ] =  diffLine(tmpData, data);
			differences.push_back( tmpDifferences );

			tmp = tmp->getNext();

		}
		return differences;
	}

	pair<Revision*, int>RevisionHandler::bestOrigin( Revision* rev,  vector<char>& data ){
		if( NULL == rev->getLast() )
			return pair<Revision*, int>(  rev, METHODE_STD );

		vector< vector< uint64_t> > differences = calculDifferences( rev, data);

		///Selection best method by rev
		vector< uint64_t > linearDiff	= vector< uint64_t >( differences.size() );
		vector< uint64_t > linearMethod	= vector< uint64_t >( differences.size() );
		uint64_t tmpDiff = 0;
		int tmpMethod = 0;

		for( int i=0 ; i<differences.size() ; i++){
			tmpDiff 	= differences[i][0];
			tmpMethod	= 0;
			for( int j=0; j<=methodes.size() ; j++){
				if( tmpDiff > differences[i][j] ){
					tmpDiff 	= differences[i][j];
					tmpMethod	= (j==methodes.size()) ? METHODE_LINE : j;
				}
			}

			linearDiff[i]	= tmpDiff;
			linearMethod[i]	= tmpMethod;
		}

		///Selection best origin
		Revision* tmpRev = rev->getRoot()->getNext();
		tmpDiff 	= linearDiff[0];
		int j=0;
		for( int i=0; i<linearDiff.size(); i++){
			if( tmpDiff > linearDiff[i] ){
				tmpDiff = linearDiff[i];
				j=i;
			}
		}

		for( int k=0; k<j; k++)
			tmpRev = tmpRev->getNext();

		return pair<Revision*, int>(  tmpRev, linearMethod[j] );
	}

	void RevisionHandler::createBlockMutations( vector<char>& origine, vector<char>& data, ofstream* stream, uint64_t pos, int method){
		stream->seekp(pos, stream->end);
		uint64_t blockSize = methodes[ method ];

		uint8_t type = 0;
		uint64_t idBeginning = 0;
		uint64_t size = 0;

		///Update first
		uint64_t j(0);
		uint64_t n = min( origine.size(), data.size() );

		uint64_t length(0);
		bool flag(true), lastFlag(true);
		for(uint64_t i=0 ; i<n ; i+=blockSize){
			j		= 0;
			flag	= true;
			while(j<blockSize && i+j<n && flag){
				flag = ( origine[i+j] == data[i+j] );
				j++;
			}

			length	+=	(flag) ? 0 : blockSize;

			if( flag && !lastFlag ){
				type = Mutation::UPDATE;
				idBeginning = i-length;
				size = length;

				stream->write( (char*)&type,1);
				stream->write( (char*)&idBeginning, 8);
				stream->write( (char*)&size, 8);
				write(data, i-length, length, stream);

				length=0;
				flag=true;
			}
			lastFlag = flag;
		}
		if( !flag ){ ///Save the last block
			type = Mutation::UPDATE;
			idBeginning = n-length;
			size = length;

			stream->write( (char*)&type,1);
			stream->write( (char*)&idBeginning, 8);
			stream->write( (char*)&size, 8);
			write(data, n-length, length, stream);
		}

		///Delete
		if( origine.size()>data.size() ){
			type = Mutation::DELETE;
			idBeginning = data.size();
			size = origine.size() - data.size();

			stream->write( (char*)&type,1);
			stream->write( (char*)&idBeginning, 8);
			stream->write( (char*)&size, 8);
		}

		///Insert
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

	void RevisionHandler::createLineMutations( vector<char>& origine, vector<char>& data, ofstream* stream, uint64_t pos){
        stream->seekp(pos, stream->end);

		uint8_t type = 0;
		uint64_t idBeginning = 0;
		uint64_t size = 0;

		vector< uint64_t > lineO = stripLine( origine );
		vector< uint64_t > lineD = stripLine( data );

		///Update first
		uint64_t num = min( lineO.size(), lineD.size() );
		uint64_t diff = 0;

		uint64_t lastBegO(0), lastBegD(0);
		uint64_t j(0), m(0), length(0);
		bool flag(true), lastFlag(true);
		for(uint64_t i=0 ; i<num ; i++){
			flag = ( lineO[i]- lastBegO == lineD[i]- lastBegD );//Size comparison

			j=0;
			m=min( lineO[i]- lastBegO, lineD[i]- lastBegD);
			while(flag && j<m){
				flag =( origine[ lastBegO + j ] == data[ lastBegD + j ] );
				j++;
			}

			length 		+= (flag) ? 0 : lineD[i] - lastBegD;
			lastBegO	= lineO[i];
			lastBegD	= lineD[i];

			if( flag && !lastFlag ){
				type = Mutation::UPDATE;
				idBeginning = lastBegD-length;
				size = length;

				stream->write( (char*)&type,1);
				stream->write( (char*)&idBeginning, 8);
				stream->write( (char*)&size, 8);
				write(data, lastBegD-length, length, stream);

				length=0;
				flag=true;
			}
			lastFlag = flag;
		}
		if( !flag ){ ///Save the last line
			type = Mutation::UPDATE;
			idBeginning = lineD[num-1]-length;
			size = length;

			stream->write( (char*)&type,1);
			stream->write( (char*)&idBeginning, 8);
			stream->write( (char*)&size, 8);
			write(data, lineD[num-1]-length, length, stream);
		}

		///Delete
		if( lineD.size() < lineO.size() ){
			type = Mutation::DELETE;
			idBeginning = data.size();
			size = lineO.back() - lineD.back();

			stream->write( (char*)&type,1);
			stream->write( (char*)&idBeginning, 8);
			stream->write( (char*)&size, 8);
		}

		///Insert
		if( lineO.size() < lineD.size() ){
			type = Mutation::INSERT ;
			idBeginning = origine.size();
			size = lineD.back() - lineO.back();

			stream->write( (char*)&type,1);
			stream->write( (char*)&idBeginning, 8);
			stream->write( (char*)&size, 8);
			write(data, origine.size(), size, stream);
		}

		stream->flush();
	}

	void RevisionHandler::createMutations( vector<char>& origine, vector<char>& data, ofstream* stream, uint64_t pos, int method){
		if(method == METHODE_LINE)
			createLineMutations( origine, data, stream, pos);
		else
			createBlockMutations( origine, data, stream, pos, method);
	}

	Revision* RevisionHandler::newRevision( Revision* origin, int method, vector<char>&newData){
		uint32_t tableSize = extractSizeTable( origin->getIStream() );
		vector<TableElement> table = extractTable( origin->getIStream() );


		///Building of origin
		vector<char> tmpData;
		vector< Revision* > parents = origin->getParents();
        for( int i=0; i<parents.size() ; i++)
            applyMutations( tmpData, parents[i]);
        if( parents.empty() )//Data is now hydrate
            applyMutations( tmpData, origin);

		///Rev creation, size will be hydrate later
		Revision* rev = (origin->getLast() != NULL ) ? origin->getLast() : origin;
		cout<<diff( tmpData, newData, method) <<endl;
		Revision* newRev= new Revision( tableSize+1, rev->getIdBeginning()+rev->getSize(), 0, diff( tmpData, newData, method) );
		rev->addChild( newRev );
		newRev->setPrevious( rev );
		newRev->setParent( origin );
		newRev->setRoot( origin->getRoot() );
		newRev->setIStream( origin->getIStreamLocation() );
		newRev->setOStream( origin->getOStreamLocation() );
		newRev->setLast( newRev );
		createMutations( tmpData, newData, newRev->getOStream(), tableSize, method);

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


