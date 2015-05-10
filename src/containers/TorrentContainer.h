#ifndef TORRENTCONTAINER_H_INCLUDED
#define TORRENTCONTAINER_H_INCLUDED


#include "X_Container.h"
#include "DirContainer.h"

using namespace std;

class TorrentContainerIndex : public X_ContainerIndex, public DirContainerIndex{
	public:
		TorrentContainerIndex() : AbstractContainerIndex("TorrentContainer"){}
};

/**
 * Store a collection of Documents
 */
class DirContainer : public X_Container, public DirContainer{
	
	public:
		TorrentContainer() : AbstractContainer(){}
		TorrentContainer(uint64_t i, uint64_t p) : AbstractContainer(i,p){}
		TorrentContainer(Metadata* meta, Document* doc) : X_Container(), metadata(meta), document(doc){}
		TorrentContainer(uint64_t i, uint64_t p, Metadata* meta, Document* doc) : X_Container(i, p), X_Container(meta, doc) {}
};

#endif // DIRCONTAINER_H_INCLUDED
