#include "Revision.h"

/**
 *  WorkingRev
 */
	void WorkingRev::add(char c){
		v_data.push_back(c);
		size++;
	}
		
	void WorkingRev::add(char* data, uint64_t s){
		v_data.resize(s);
		for(uint64_t i=0 ; i<s ; i++)
			v_data[size+i] = data[i];
		
		size += s;
	}

	void WorkingRev::update( uint64_t begin, uint64_t s, char*data ){
		for(uint64_t i=0 ; i<s ; i++)
			v_data[begin+i] = data[i];
	}

	void WorkingRev::erase(uint64_t begin, uint64_t s){
		v_data.erase( v_data.begin() + begin, v_data.begin() + begin + size);
		size-=s;
	}


/**
 *  RevisionHandler
 */

///Structure functions
	void RevisionHandler::extractNumber(){
		descriptor.seekg(-2, descriptor.end );
		descriptor.read((char*)&number, 2);
		
		table_length = number * Revision::REVISION_SIZE_TABLE + 2;///+2 : number of table element uint16_t
	}

	void RevisionHandler::extractTable(){
		table = vector<TableElement> (number);
		
		int newOrigin = (-1) * table_length;
		descriptor.seekg(newOrigin , descriptor.end);

		for(uint16_t i=0 ; i<sizeTable ; i++ ){
			TableElement element;
			descriptor.read((char*)&element.idBeginning, 8);
			descriptor.read((char*)&element.size, 8);
			descriptor.read((char*)&element.origin, 2);
			table[i] = element ;
		}

		return table;
	}

	vector<int> RevisionHandler::extractChildren( vector< int >& origines, int parent ){
		vector<int> children;
		for(uint32_t i=0; i<origines.size(); i++)
			if( parent == origines[i] )
				children.push_back( i );
		return children;
	}

	void RevisionHandler::buildStructure(){
		tree = new Revision();
		if( number == 0 )
			return;

		Revision* current;
		vector< vector<Revision*> > structur =  vector< vector<Revision*> >( table.size() ); /// Must not be deleted, it's used in revision tree
		///Previous and root
		for( uint32_t i=0 ; i<table.size() ; i++){
			current = new Revision( i+1, table[i].idBeginning, table[i].size );
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
		for( uint32_t i=0 ; i<table.size() ; i++){
			current->setChildren( structur[i] );
			for( uint32_t j=0 ; j<structur[i].size() ; j++)
				structur[i][j]->setParent( current );

			current = current->getNext();
		}
	}


///Building functions
	void RevisionHandler::applyMutations( WorkingRev& workingRev ){
		if( rev == NULL || rev->getRoot() == rev )
			return;
		
		uint64_t i =0;
		descriptor.seekg( tree->getIdBeginning() - tree->getRelativeO(), descriptor.beg );
		while( i < tree->getSize() ){
			Mutation m;
			m.read( descriptor );
			m.apply(data, descriptor);
			i+=m.getSize() + Mutation::HEADER_SIZE;
		}
	}
	
	void RevisionHandler::applyLinear( WorkingRev& workingRev , int n_rev){
		Revision* last = tree;
		
		for(int i=0; i<n_rev; i++){
			applyMutations(workingRev);
			tree = tree->getNext();
		}
		
		tree = last;
	}
	
	void apply(WorkingRev& workingRev){
		vector<Revision*> parents = tree->getParents();
		
		for(int i=0; i<parents.size() ; i++){
			tree = parents[i];
			applyMutations(workingRev);
		}
	}

///Write functions
	void RevisionHandler::writeTable(){
		descriptor.seekp( 0, descriptor.end );
		for( uint64_t i=0; i<table.size(); i++){
			descriptor.write( (char*)&table[i].idBeginning, 8);
			descriptor.write( (char*)&table[i].size, 8);
			descriptor.write( (char*)&table[i].origin, 2);
		}

		///Add nbr of table element
		descriptor.write( (char*)&number, 2);
		descriptor.flush();
	}

///Diff functions
	vector< uint64_t > RevisionHandler::stripLine( WorkingRev& current ){ ///vector[last char of the line+1 ie begin of the new line]
		vector< uint64_t > lines;
		if(current.empty())
			return lines;

		lines.push_back(0);
		for(uint64_t i=0 ; i<d.size() ; i++){
			if( current[i]==10 )
                lines.push_back( i+1 );
		}
		if( lines.back() != current.size())///abstract line
			lines.push_back( current.size() );

		return lines;
	}
	
	uint64_t RevisionHandler::diffLine( WorkingRev& origine, WorkingRev& data){
		uint64_t diff = 0;

		vector< uint64_t > lineO = stripLine( origine );
		vector< uint64_t > lineD = stripLine( data );

		///Update first
		uint64_t num = min( lineO.size(), lineD.size() );

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
				diff += length + Mutation::HEADER_SIZE;

				length=0;
				flag=true;
			}
			lastFlag = flag;
		}
		if( !flag ) ///Save the last line
			diff += length + Mutation::HEADER_SIZE;

		///Delete
		if( lineD.size() < lineO.size() )
			diff += lineO.back() - lineD.back() + Mutation::HEADER_SIZE;


		///Insert
		if( lineO.size() < lineD.size() )
			diff += lineD.back() - lineO.back() + Mutation::HEADER_SIZE;

		return diff;
	}

	uint64_t RevisionHandler::diffBlock(  WorkingRev& origine, WorkingRev& data, int method){
		uint64_t blockSize = methodes[ method ];
		uint64_t diff = 0;

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
				diff += length + Mutation::HEADER_SIZE;
				length=0;
				flag=true;
			}
			lastFlag = flag;
		}
		if( !flag ) ///The last block
			diff += length + Mutation::HEADER_SIZE;

		///Delete
		if( origine.size()>data.size() )
			diff += origine.size() - data.size() + Mutation::HEADER_SIZE;
		
		///Insert
		if( origine.size()<data.size() )
			diff += data.size() - origine.size() + Mutation::HEADER_SIZE;
			
		return diff;
	}

    uint64_t RevisionHandler::diff( WorkingRev& origine, WorkingRev& data, int method){
        if(method == METHODE_LINE)
			return diffLine( origine, data);
		else
			return diffBlock( origine, data, method);
	}

	vector< vector<uint64_t> > RevisionHandler::calculDifferences( WorkingRev& data ){
		WorkingRev tmpData;
		vector< vector<uint64_t> > differences;
		vector< uint64_t > tmpDifferences =  vector< uint64_t >( methodes.size()+1 );
		Revision* tmp = (tree->getRoot()->getNext()); ///Diff with racine <=> rev origine

		while( tmp != NULL ){
			applyMutations( tmpData, tmp);

			tmpDifferences =  vector< uint64_t >( methodes.size()+1 );
			for(uint32_t i=0; i<methodes.size() ; i++)
				tmpDifferences[i] =  diff(tmpData, data, i);
			tmpDifferences[ methodes.size() ] =  diffLine(tmpData, data);
			differences.push_back( tmpDifferences );

			tmp = tmp->getNext();

		}
		return differences;
	}

	pair<Revision*, int>RevisionHandler::bestOrigin( WorkingRev& data ){
		if( tree->getRoot() == tree->getLast() )
			return pair<Revision*, int>(  tree, METHODE_STD );

		vector< vector< uint64_t> > differences = calculDifferences(data);

		///Selection best method by rev
		vector< uint64_t > linearDiff	= vector< uint64_t >( differences.size() );
		vector< uint64_t > linearMethod	= vector< uint64_t >( differences.size() );
		uint64_t tmpDiff = 0;
		int tmpMethod = 0;

		for( uint32_t i=0 ; i<differences.size() ; i++){
			tmpDiff 	= differences[i][0];
			tmpMethod	= 0;
			for( uint32_t j=0; j<=methodes.size() ; j++){
				if( tmpDiff > differences[i][j] ){
					tmpDiff 	= differences[i][j];
					tmpMethod	= (j==methodes.size()) ? METHODE_LINE : j;
				}
			}

			linearDiff[i]	= tmpDiff;
			linearMethod[i]	= tmpMethod;
		}

		///Selection best origin
		Revision* tmpRev = tree->getRoot()->getNext();
		tmpDiff 	= linearDiff[0];
		int j=0;
		for( uint32_t i=0; i<linearDiff.size(); i++){
			if( tmpDiff > linearDiff[i] ){
				tmpDiff = linearDiff[i];
				j=i;
			}
		}

		for( int k=0; k<j; k++)
			tmpRev = tmpRev->getNext();

		///If diff/size > offset : creation af a new branch
		double ratio = (double)linearDiff[j] /(double)data.size();
		if( ratio > MAX_RATIO_ALLOWED )
			pair<Revision*, int>( tmpRev->getOrigin(), METHODE_STD);
			 
		return pair<Revision*, int>(  tmpRev, linearMethod[j] );
	}

///Creation functions
	void RevisionHandler::createBlockMutations( WorkingRev& origine, WorkingRev& data, uint64_t pos, int method){
		descriptor.seekp(pos, descriptor.end);
		uint64_t blockSize = methodes[ method ];

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
				Mutation m(Mutation::UPDATE, i-length, length);
				m.write(data, descriptor);

				length=0;
				flag=true;
			}
			lastFlag = flag;
		}
		if( !flag ){ ///Save the last block
			Mutation m(Mutation::UPDATE, n-length, length);
			m.write(data, descriptor);
		}

		///Delete
		if( origine.size()>data.size() ){
			Mutation m(Mutation::DELETE, data.size(), origine.size() - data.size());
			m.writeHeader();
		}

		///Insert
		if( origine.size()<data.size() ){
			Mutation m(Mutation::INSERT, origine.size(), data.size() - origine.size());
			m.write(data, descriptor);
		}
	}

	void RevisionHandler::createLineMutations( WorkingRev& origine, WorkingRev& data, uint64_t pos){
        descriptor.seekp(pos, descriptor.end);

		vector< uint64_t > lineO = stripLine( origine );
		vector< uint64_t > lineD = stripLine( data );

		///Update first
		uint64_t num = min( lineO.size(), lineD.size() );

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
				Mutation m(Mutation::UPDATE, lastBegD-length, length);
				m.write(data, descriptor);

				length=0;
				flag=true;
			}
			lastFlag = flag;
		}
		if( !flag ){ ///Save the last line
			Mutation m(Mutation::UPDATE, lineD[num-1]-length, length);
			m.write(data, descriptor);
		}

		///Delete
		if( lineD.size() < lineO.size() ){
			Mutation m(Mutation::DELETE, data.size(), lineO.back() - lineD.back());
			m.writeHeader();
		}

		///Insert
		if( lineO.size() < lineD.size() ){
			Mutation m(Mutation::INSERT, origine.size(), lineD.back() - lineO.back());
			m.write(data, descriptor);
		}
	}

	void RevisionHandler::createMutations( WorkingRev& origine, WorkingRev& data, uint64_t pos, int method){
		if(method == METHODE_LINE)
			createLineMutations( origine, data, stream, pos);
		else
			createBlockMutations( origine, data, stream, pos, method);
	}

	void RevisionHandler::newRevision( WorkingRev& newData ){
		///Extract origin
		pair<Revision*, int> p = revHandler.bestOrigin( rev, data );
		Revision* origin = p.first;
		int method = p.second;
		
		///Build origin
		NewData tmpData;
		vector< Revision* > parents = origin->getParents();
        for( uint32_t i=0; i<parents.size()-1 ; i++) ///-1 we eliminate origin itself, will be build later
            applyMutations( tmpData, parents[i]);
        applyMutations( tmpData, origin);

		///size will be hydrate later
		Revision* parent = origin->getLast();
		Revision* newRev= new Revision( number+1, parent->getIdBeginning()+parent->getSize(), 0);
		
		parent->addChild( newRev );
		newRev->setPrevious( parent );
		newRev->setParent( parent );
		newRev->setRoot( parent->getRoot() );
		newRev->setLast( newRev );
		createMutations( tmpData, newData, table_length, method);

		///Size
		descriptor.seekp (0, descriptor.end);
		uint64_t length = descriptor.tellp();
		newRev->setSize( length - newRev->getIdBeginning() );

		///Maj of the table
		TableElement newElement;
		newElement.idBeginning = newRev->getIdBeginning();
		newElement.size = newRev->getSize();
		newElement.diff = newRev->getDiff();
		newElement.origin = origin->getN();
		
		///Maj Handler
		number++;
		table_length+=Mutation::HEADER_SIZE;
		table.push_back( newElement );

		writeTable();
	}
