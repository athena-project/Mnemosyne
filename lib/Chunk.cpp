#include "Chunk.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>


/**
 * Chunk
 */

Chunk::Chunk(){}

Chunk::Chunk( uint32_t size){
	this->size = size;
}


Chunk::Chunk( uint64_t id, uint32_t size){
	this->id = id;
	this->size = size;
}



/**
 *  ChunckManager
 */

ChunkManager::ChunkManager(){
    cout<<"hello"<<endl;
}

ChunkManager::~ChunkManager(){
    conn.disconnect();
}

uint64_t ChunkManager::insert( Chunk chunk){
	mysqlpp::Query query = conn.query();
	query<<"INSERT INTO chunk (size) VALUES (" <<chunk.getSize()<< ")";

	if (mysqlpp::SimpleResult res = query.execute())
		return (uint64_t)res.insert_id();
	else{
		cerr << "Failed to get item list: " << query.error() << endl;
		throw "";
	}
}

vector<uint64_t> ChunkManager::insert( vector< Chunk > chunks ){
    if( chunks.empty() )
        return vector<uint64_t>();

    mysqlpp::Query query = conn.query();
    mysqlpp::Query id_query = conn.query();

	query<<"INSERT INTO chunk (size) VALUES ";

	for(vector<Chunk>::iterator it=chunks.begin(); it!=chunks.end(); it++){
        if( it != chunks.begin() )
            query<<", ";
        query<<"("<<(*it).getSize()<<")";
    }

	vector<uint64_t> ids( chunks.size(), 0);

	if (mysqlpp::SimpleResult res = query.execute()){

        id_query<<"SELECT  LAST_INSERT_ID() as id FROM chunk";
        mysqlpp::StoreQueryResult id_res = id_query.store();

        uint64_t  I = (uint64_t)id_res[0]["id"];

        for( uint64_t i =0; i<ids.size() ; i++)
            ids[ ids.size()-i-1 ] = I-i;

		return ids;
	}else{
		cerr << "Failed to get item list: " << query.error() << endl;
		throw "";
	}
}

Chunk ChunkManager::get(uint64_t id){
	std::stringstream idStr;
	idStr<<id;

	vector<Chunk> vect = get( "*", "id = "+idStr.str(), "id", "");
	if( vect.size() == 1 )
		return vect[0];
	else
		throw runtime_error("chunk not found");
}

vector<Chunk> ChunkManager::get( string fieldsNeeded, string where, string order, string limit){
	vector<mysqlpp::Row> v;
	vector< Chunk > chunks;
	mysqlpp::Query query = conn.query();
	query << "SELECT " << fieldsNeeded <<" FROM chunk WHERE "<< where <<" ORDER BY "<< order<< " "<<limit;
	mysqlpp::StoreQueryResult res = query.store();

	for(size_t i = 0; i < res.num_rows(); ++i)
		chunks.push_back( Chunk( (uint64_t)res[i]["id"], (uint64_t)res[i]["size"] ) );

	return chunks;
}

vector<Chunk> ChunkManager::get(vector<uint64_t> ids){
	string where = "id IN (";

	for(uint64_t i=0; i<ids.size(); i++){
		where += ( i != 0 ) ? "," : "";

		std::ostringstream id;
		id<<ids[i];
		where += id.str();
	}
	where += ")";
	return get( "*", where, "id", "");
}

uint64_t ChunkManager::count( string where, string order, string limit ){
	vector<mysqlpp::Row> v;
	mysqlpp::Query query = conn.query();
	query << "SELECT COUNT(*) AS number FROM chunck "<< where <<" "<< order<< " "<<limit;
	mysqlpp::StoreQueryResult res = query.store();

	return res[0]["number"];

}

void ChunkManager::update( Chunk chunk){
	mysqlpp::Query query = conn.query();
	query<<"UPDATE chunk SET "<<"size="<<chunk.getSize();
	query<<" WHERE id="<< chunk.getId();

	if (mysqlpp::SimpleResult res = query.execute())
		return;
	else{
		cerr << "Failed to get item list: " << query.error() << endl;
		throw "";
	}
}

void ChunkManager::update( vector< Chunk > chunks ){
    if( chunks.empty() )
        return ;

	mysqlpp::Query query = conn.query();
	query<<"INSERT INTO chunk (id, size) VALUES";

    for(vector<Chunk>::iterator it=chunks.begin(); it!=chunks.end(); it++){
        if( it != chunks.begin() )
            query<<", ";
        query<<"("<<(*it).getId()<<", "<<(*it).getSize()<<")";
    }

    query<<"ON DUPLICATE KEY UPDATE size=VALUES( size )";

	if (mysqlpp::SimpleResult res = query.execute())
		return;
	else{
		cerr << "Failed to get item list: " << query.error() << endl;
		throw "";
	}
}

/**
  * ChunkHandler
  */
ChunkHandler::~ChunkHandler(){
	for( uint32_t i=0; i<files.size(); i++)
		std::remove( files[i].c_str() );
}

string ChunkHandler::getFile( uint64_t id){
	std::ostringstream chunckId;
	chunckId<<id;

	string chunkLocation = ChunkHandler::DIR()+"/"+chunckId.str()+".xz";
	string chunkLocation2 = ChunkHandler::TMP_DIR()+"/"+chunckId.str();

	decompress( chunkLocation.c_str(), chunkLocation2.c_str());
	files.push_back(chunkLocation2);
	return chunkLocation2;
}

void ChunkHandler::save( uint64_t id){
	std::ostringstream chunckId;
	chunckId<<id;

	string chunkLocation2 = ChunkHandler::DIR()+"/"+chunckId.str()+".xz";
	string chunkLocation = ChunkHandler::TMP_DIR()+"/"+chunckId.str();

    files.push_back( chunkLocation );
    compress( chunkLocation.c_str(), chunkLocation2.c_str());

}

void ChunkHandler::writeChunk(uint64_t id, ifstream& stream, uint64_t idBeginning, uint64_t size){
	std::ostringstream strId;
	strId<<id;

	string location = ChunkHandler::TMP_DIR()+"/"+strId.str();
	ofstream oStream( location.c_str(), ios::out );

	stream.seekg( idBeginning, stream.beg );

	char* buffer = new char[ size ];
	stream.read( buffer, size);
	oStream.write( buffer, size);
	delete[] buffer;

	oStream.flush();
	oStream.close();

	save( id );
}

void ChunkHandler::updateData(Chunk c, ifstream& stream, uint64_t idBeginning, uint64_t size, uint64_t offset){
	idBeginning += offset; ///Table
	idBeginning -= c.getSize(); ///chunk must be overwrite

	std::ostringstream strId;
	strId<<c.getId();
	string location = getFile( c.getId() );

	ofstream oStream( location.c_str(), ios::binary);
	stream.seekg( idBeginning, stream.beg );

	///Write first part of the chunk
	char* buffer = new char[ c.getSize()-offset ];
	stream.read( buffer, c.getSize()-offset);
	oStream.write( buffer, c.getSize()-offset);
	delete[] buffer;

	///Skip offset's bytes
	stream.seekg( offset, stream.cur );

	///Write last part of the chunk
	char* buffer2 = new char[ size ];
	stream.read( buffer2, size);
	oStream.write( buffer2, size);
	delete[] buffer2;

	oStream.flush();
	oStream.close();

	///SQL UPDATE
	c.setSize( c.getSize()-offset+size );
	manager->update( c );

	save( c.getId() );
}

vector<Chunk> ChunkHandler::makeChunks( ifstream& stream, uint64_t idBeginning, uint64_t size ){
	uint64_t nbrNeeded = ceil( (float)size / (float)(Chunk::CHUNK_SIZE_MAX) );
	stream.seekg( idBeginning );

	///Chunks creation
	vector<Chunk> chunks;
	for(uint64_t i=0; i<nbrNeeded; i++){
		if( i<nbrNeeded-1)
			chunks.push_back( Chunk(Chunk::CHUNK_SIZE_MAX) );
		else
			chunks.push_back( Chunk( size-(nbrNeeded-1)*(Chunk::CHUNK_SIZE_MAX)) );//Size less than max
	}

	///SQL insertion and writting on hard drive
	vector<uint64_t> ids = manager->insert( chunks );
	uint32_t chunkSize = Chunk::CHUNK_SIZE_MAX;

	for( uint64_t i=0; i<ids.size(); i++){
		chunks[i].setId( ids[i] );

		if( i == ids.size()-1 )
			chunkSize = min( (uint64_t)Chunk::CHUNK_SIZE_MAX, size-(ids.size()-1)*(Chunk::CHUNK_SIZE_MAX) );
		else
			chunkSize = Chunk::CHUNK_SIZE_MAX;

        writeChunk( ids[i], stream, idBeginning+i*(Chunk::CHUNK_SIZE_MAX), chunkSize);
	}

	return chunks;
}
