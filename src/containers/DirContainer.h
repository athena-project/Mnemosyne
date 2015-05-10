#ifndef DIRCONTAINER_H_INCLUDED
#define DIRCONTAINER_H_INCLUDED

#include <map>
#include <utility>

#include "AbstractContainer.h"
#include "ContainerFactory.h"

using namespace std;

class DirContainerIndex : public AbstractContainerIndex{
	public:
		DirContainerIndex() : AbstractContainerIndex("DirContainer"){}
};

/**
 * Store a collection of Documents
 */
class DirContainer : public AbstractContainer{
	protected:
	
	public:
		DirContainer() : AbstractContainer(){}
		DirContainer(uint64_t i, uint64_t p) : AbstractContainer(i,p){}
		
		void addChild(AbstractContainer* child ){
			child->setLocation( location );
			children.insert( pair<uint64_t, AbstractContainer*>(child->getId(), child); 
		}
		/**
		 * Expose a child
		 * @param id - id of the exposed child
		 */
		AbstractContainer& expose( uint64_t id ){ return &children[i]; }
};

#endif // DIRCONTAINER_H_INCLUDED
