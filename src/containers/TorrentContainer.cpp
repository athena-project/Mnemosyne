#include "TorrentContainer.h"

void DirContainerIndex::build( AbstractContainer* abs){
	X_Container::build(abs);
	DirContainer::build(abs);
}


void DirContainer::build(){
	X_Container::build();
	DirContainer::build();
}

