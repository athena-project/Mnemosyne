#include "X_Container.h"

void X_Container::build(){
	AbstractContainer::build();

	
	if( !compress( document->location()+"/data", location.string().c_str()+"/data.xz" ) || !fs::copy( document->metadataLocation(), location+fs::path("/metadata") ) )
		throw "X_Container can not be build";
}

void X_Container::copyDocumentTo( string path ){
	if( !decompress( location.string().c_str()+"/data.xz", path.c_str() ) )
		throw "Document cannot be copy to "+path;
}
