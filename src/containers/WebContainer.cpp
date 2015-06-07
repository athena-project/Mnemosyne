#include "DirContainer.h"

void WebContainerIndex::build( AbstractContainer* abs){///here AbstractContainer is a WebContainer
	DirContainerIndex::build( abs );
	
	XMLElement* uri = xml->NewElement("uri");
	xml->LinkEndChild( uri );

	
	uri->setText( abs->getUri() );
	root->InsertLastElement( uri );

}


