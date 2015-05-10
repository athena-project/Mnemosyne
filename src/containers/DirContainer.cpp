#include "DirContainer.h"

/// DirContainerIndex

void DirContainerIndex::build( AbstractContainer* abs){///here AbstractContainer is a DirContainer
	AbstractContainerIndex::build( abs );
	XMLElement* children = xml->NewElement("children");
	
	
	for(map<uint64_t, AbstractContainer*>::iterator it=abs->children.begin(); it!=abs->children.end(); ++it){
		AbstractContainerIndex* current = it->second->getIndex();
		current->build( it->second );
		children->InsertLastElement( &build->expose() );
	}

	root->InsertLastChild( childrend );
}

void DirContainerIndex::hydrate( AbstractContainer* abs){
	AbstractContainerIndex::hydrate( abs );
	XMLElment* elmt = root;
	elmt = elmt->FirstChildElement();
	
	///Now building documents
	elmt = elmt->FirstChildElement("children");
	elmt = elmt->FirstChildElement();
	while( elmt != NULL ){
		AbstractContainer* container = ContainerFactory::getFromName( elmt->getName() );
		container->setParentId( id );
		container->setLocation( location + fs::path("/") + fs::path( elmt->FirstChildElement("id")->GetText() ) );
		container->getIndex()->hydrate();
		
		children.insert( pair<uint64_t, AbstractContainer>( container->getId(), container ) );
		map_children.insert( container->getMap_children() );
		
		elmt->NextSiblingElement();
	}
}



/// DirContainer

void DirContainer::build(){
	AbstractContainer::build();

	for(map<uint64_t, AbstractContainer*>::iterator it=abs->children.begin(); it!=abs->children.end(); ++it)
		it->second->build();
}
