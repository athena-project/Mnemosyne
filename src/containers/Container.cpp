#include "Container.h"

void ContainerIndex::build( AbstractContainer* abs){///here AbstractContainer is a Container
	AbstractContainerIndex::build( abs );
	
	XMLElement* document = xml->NewElement("document");
	XMLElement* document_id = xml->NewElement("id");
	XMLElement* document_class = xml->NewElement("class");

	document_id->setText( abs->getDocument()->getId() ); 
	document_class->setText( abs->getDocument()->getClass() ); 

	document->InsertLastChild( document_id );
	document->InsertLastChild( document_class );
	root->InsertLastChild( document );
}




void Container::build(){
	AbstractContainer::build();
	
	if( !fs::copy( document->location(), location+fs::path("/data") ) || !fs::copy( document->metadataLocation(), location+fs::path("/metadata") ) )
		throw "Container can not be build";
}

void Container::copyDocumentTo( string path ){
	if( !fs::copy( location()+fs::path("/data"), fs::path(path) )
		throw "Document cannot be copy to "+path;
}

void Container::copyMetadataTo( string path ){
	if( !fs::copy( location()+fs::path("/metadata"), fs::path(path) )
		throw "Metadata cannot be copy to "+path;
}

XMLDocument Container::readMetadata(){
	XMLDocument doc;
	doc.LoadFile( location.string().c_str()+"/metadata" );
	return doc;
}
