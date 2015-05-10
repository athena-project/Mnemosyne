#include "AbstractContainer.h"



void AbstractContainer::buildIndex( ){
	tx::XMLElement* elmt_id = xml->NewElement("id");
	elmt_id->SetText( (unsigned int)id );
	root->InsertFirstChild( elmt_id );
}

void AbstractContainer::hydrate( ){
	tx::XMLElement* elmt = root;
	elmt = elmt->FirstChildElement("id");
	setId( elmt->GetText() );
}


void AbstractContainer::writeIndex( ){
	fs::path dir=location;
	dir+=fs::path("/index");
	xml->SaveFile( dir.string().c_str() );
}



void AbstractContainer::copyTo( string path){
	fs::copy_directory( location, fs::path(path));
}


void AbstractContainer::build(){
	if( !fs::is_directory( location ) && !fs::create_directory( location ) )
		throw "Container can not be build";

	buildIndex();
	writeIndex();
}
