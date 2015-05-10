#include "Mutation.h"


///Read
	void Mutation::readMutation( fstream& stream ){
		stream.read( (char *)&type, 1);
		stream.read( (char *)&idBegining, 8);
		stream.read( (char *)&size, 8);
	}

///Apply
void Mutation::applyInsert(WorkingRev& workingRev, fstream& stream){
	char* buffer[BUFFER_LENGTH];

	for(uint64_t i=0; i<(size%BUFFER_LENGTH); i++){
		stream.read(buffer, BUFFER_LENGTH);
		workingRev.add( buffer );
	}
	
	int64_t length = size-(size%BUFFER_LENGTH) * BUFFER_LENGTH;
	char* buffer[length];
	
	stream.read(buffer, length);
	workingRev.add( buffer );
}

void Mutation::applyDelete(WorkingRev& workingRev){
	workingRev.erase( idBeginning, size);
}

void Mutation::applyUpdate(vector<char>& data, fstream& stream){
	char* buffer[BUFFER_LENGTH];

	for(uint64_t i=0; i<(size%BUFFER_LENGTH); i++){
		stream.read(buffer, BUFFER_LENGTH);
		workingRev.update( idBeginning+i, BUFFER_LENGTH, buffer );
	}
	
	int64_t begin = (size%BUFFER_LENGTH) * BUFFER_LENGTH
	int64_t length = size-begin;
	char* buffer[length];
	
	stream.read(buffer, length);
	workingRev.update( begin, length, buffer );
}

void Mutation::apply( WorkingRev& workingRev, fstream& stream){
void Mutation::apply( WorkingRev& workingRev, fstream& stream){
	if( type == INSERT )
		applyInsert( workingRev, stream );
	else if( type == DELETE )
		applyDelete( workingRev );
	else
		applyUpdate( workingRev, stream );
}

///Write
void Mutation::writeHeader(fstream& descriptor){
	descriptor.write( (char*)&type,1);
	descriptor.write( (char*)&idBeginning, 8);
	descriptor.write( (char*)&size, 8);
}

void Mutation::write(WorkingRev& workingRev, fstream& descriptor){
	///Header
	writeHeader(o_description);

	///Content
	char* buffer[BUFFER_LENGTH];

	for(uint64_t i=pos; i<(length%BUFFER_LENGTH); i++){
		for(uint64_t j=0, j<BUFFER_LENGTH; j++)
			buffer[j] = workingRev[i+j];
		descriptor.write(buffer, BUFFER_LENGTH);
	}

	length = length-(length%BUFFER_LENGTH) * BUFFER_LENGTH;
	char* buffer[length];
	
	for(uint64_t j=0, j<length; j++)
		buffer[j] = workingRev[i+j];
	descriptor.write(buffer, BUFFER_LENGTH);
	
	descriptor.flush();
}
