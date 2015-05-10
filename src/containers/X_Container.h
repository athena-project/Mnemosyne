#ifndef X_CONTAINER_H_INCLUDED
#define X_CONTAINER_H_INCLUDED

#include "Container.h"
#include "../Xz.h"


class X_ContainerIndex : public ContainerIndex{
	public:
		X_ContainerIndex() : ContainerIndex("X_Container"){}
};

class X_Container : public Container, public Xz{
	protected:
		const uint32_t PRESET = 6;
		
	public:
		X_Container() : Container(){ init_encoder(PRESET); }
		X_Container(uint64_t i, uint64_t p) : Container(i,p){ init_encoder(PRESET); }
		X_Container(Metadata* meta, Document* doc) : X_Container(), metadata(meta), document(doc){}
		X_Container(uint64_t i, uint64_t p, Metadata* meta, Document* doc) : X_Container(i, p), X_Container(meta, doc) {}
}; 



#endif // X_CONTAINER_H_INCLUDED
