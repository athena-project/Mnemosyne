#include "IncrementalFile.h"



fs::path IncrementalFile::tmpfile(){
	ostringstream stime;
	stime<<time(NULL);
	fs::path tmp = location + fs::path("/") + fs::path( stime.str() );
	
	if( fs:exists(tmp) )
		return tmpfile();
	
	
	fstream tmpFs( tmp.str().c_str() ); ///Write the file
	if(!tmpFs)
		return tmpfile();
		
	fileGarbage.push_after( tmp );
	tmpFs.close();
	return tmp;
}


///Init functions

void IncrementalFile::lazyInit(){
	if( num_chunks==0 )
		return;
	
	///Where is the table
	Chunk last( num_chunks-1 );
	last.load(); 
	handler->setDescriptor( last.getDescriptor() );
	handler->extractNumber();
	
	int num_chunk_table = ceil( (double)handler->getTable_length() / (double)Chunk::CHUNK_SIZE_MAX );///last included
	if(num_chunk_table > 1 ){
		const char* tmpLocation = tmpfile().str().c_str()
		for( int i=num_chunks-num_chunk_table; i<num_chunk; i++)
			Chunk(i).loadInto( tmpLocation );
			
		last.loadInto( tmpLocation );
		handler->setDescriptor( fstream( tmpLocation ) );
	}
	
	handler->buildStructure();
	init_state = LAZY_INIT;
}

void IncrementalFile::init(){
	fs::path descriptor_path = tmpfile();
	fstream descriptor( lazy_descriptor_path.c_str() );
	
	for( int i=0; i<num_chunks; i++){
		Chunk tmpChunk( i, 0, dir ); ///We do not now the size
		tmpChunk.load();
		tmpChunk.read( descriptor, 0, chunks[i].readSize());
	}
	
	handler->setDescriptor( descriptor );
	handler->extractNumber();
	handler->buildStructure();
	init_state = GLOBAL_INIT;
}


/// Loadings functions

WorkingRev IncrementalFile::lazyLoading(int n){
	Revision* rev = handler->getTree()->get( n );
	Revision* lazy_tree = new Revision();///a tree with only parent
	vector< Revision* > parents = rev->getParents();
	vector<ChunkPiece> pieces;
	
	///create a new abstract revision tree that use only the needed chunk and rev
	for(int i=1; i<parents.size() ; i++){ ///rev included in parents
		lazy_tree->setNext( new Revision( parents[i]->getN() ) );
		lazy_tree->getNext()->setPrevious( lazy_tree );
		 	
		lazy_tree = lazy_tree.next();
		lazy_tree->setIdBeginning( parents[i]->getIdBeginning() );
		lazy_tree->setSize( parents[i]->getSize() );
		lazy_tree->setRoot( lazy_tree->getPrevious()->getRoot() );
			
		///Recalcul idBeginning
		lazy_tree->setIdBeginning( lazy_tree->getPrevious()->getIdBeginning()+lazy_tree->getPrevious()->getSize() );
	
		///Extracting chunks (needed, [begin,end[)
		id_beg = parents[i]->getIdBeginning() % Chunk::CHUNK_SIZE_MAX;
		id_end = (parents[i]->getIdBeginning() + parents[i]->getSize() ) % Chunk::CHUNK_SIZE_MAX;
		pos_beg= parents[i]->getIdBeginning() - Chunk::CHUNK_SIZE_MAX * id_beg;
		pos_end= parents[i]->getIdBeginning() + parents[i]->getSize() - Chunk::CHUNK_SIZE_MAX * id_end;
		
		if( id_beg == id_end ) ///Only one chunk
			pieces.push_back( ChunkPiece(id_beg, pos_beg, pos_end);
		else{
			pieces.push_back( ChunkPiece(id_beg, pos_beg, pos_beg+Chunk::CHUNK_SIZE_MAX);
			for(int j=id_beg+1; j<=id_end; i++)
				pieces.push_back( ChunkPiece(j, pieces[j-1]->getEnd(), pieces[j-1]->getEnd()+Chunk::CHUNK_SIZE_MAX);
			pieces.last()->setEnd( pos_end );
		}
	}
	
	///Hydrating last
	lazy_tree->setLast( lazy_tree );
	for(int i=0; i<parents.size()-1 ; i++){
		lazy_tree = lazy_tree->getPrevious();
		lazy_tree->setLast( lazy_tree->getNext()->getLast() );
	}
	
	///Now building the descriptor link to the lazy_tree
	fs::path lazy_descriptor_path = tmpfile();
	fstream lazy_descriptor( lazy_descriptor_path.c_str() );
	
	for( int i=0; i<pieces.size(); i++){
		Chunk tmpChunk( pieces[i], dir );
		tmpChunk.load();
		tmpChunk.read( lazy_descriptor, pieces[i]);
	}
	
	///Descriptor and tree hydrated
	WorkingRev data();
	RevisionHandler lazy_handler( lazy_descriptor, lazy_tree->getOrigin() ); 
	lazy_handler.applyLinear( data, parents.size());

	///Cleaning
	delete lazy_tree;
	
	return data;
}

WorkingRev IncrementalFile::loading(int n){
	WorkingRev data();
	handler->setTree( handler->getTree()->get( n ) );	
	handler->apply(data);
	
	return data;
}

WorkingRev IncrementalFile::get(int n){
	switch( init_state ){
		case GLOBAL_INIT :
			return loading(n);
		break;
		case LAZY_INIT :
			return lazyLoading(n);
		break:
		case NO_INIT:
			lazyInit();
			return lazyLoading(n);
		break;
	}
}


/// Update and add functions

void IncrementalFile::makeChunks(){
	if( init_state != GLOBAL_INIT )
		return;
	
	fstream descriptor = handler->getDescriptor();
	descriptor.seekg(0, descriptor.end);
	uint64_t descriptorSize=descriptor.tellg();
	descriptor.seekg(0, descriptor.beg);
	
	///-1 because last existing chunk must be overwritten
	if( num_chunks > 0)
		num_chunks--;
	uint32_t new_num_chunks = ceil( (double)descriptorSize / (double)Chunk::CHUNK_SIZE_MAX ) ;
	uint64_t lastPos = num_chunks * Chunk::CHUNK_SIZE_MAX;
	
	for(int i=num_chunks; i<new_num_chunks-1; i++){
		Chunk tmpChunk( i, Chunk::CHUNK_SIZE_MAX, dir);
		tmpChunk.write( descriptor, lastPos, Chunk::CHUNK_SIZE_MAX);
		tmpChunk.save();
		lastPos+=Chunk::CHUNK_SIZE_MAX;
	}
	
	Chunk tmpChunk( i, min(Chunk::CHUNK_SIZE_MAX, descriptorSize-lastPos), dir);
	tmpChunk.write( descriptor, lastPos, min(Chunk::CHUNK_SIZE_MAX, descriptorSize-lastPos) );
	tmpChunk.save();
	
	num_chunks = new_num_chunks;
}

void IncrementalFile::save(){
	makeChunks();
	descritor a ecrire
}

void IncrementalFile::newRevision(WorkingRev& data){
	if( init_state != GLOBAL_INIT )
		initHandler();
		
	handler->newRevision(data);
	num_revision ++;
}
