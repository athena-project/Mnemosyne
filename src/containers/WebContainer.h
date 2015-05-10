#ifndef CONTAINER_H_INCLUDED
#define CONTAINER_H_INCLUDED

#include "DirContainer.h"


using namespace std;



class ContainerIndex : public DirContainerIndex{
	public:
		ContainerIndex() : AbstractContainerIndex("Container"){}
};

/**
 * Store the story of an uri
 */
class WebContainer : public DirContainer{
	protected:
		string uri = "";
		
	public:
		WebContainer() : DirContainer(){}
		WebContainer(uint64_t i, uint64_t p) : DirContainer(i,p){}
		WebContainer(string u) : DirContainer(), uri(u){}
		WebContainer(uint64_t i, uint64_t p, string u) : WebContainer(i, p), WebContainer(u) {}
		
		string getUri(){ return uri; }
};
#endif // CONTAINER_H_INCLUDED
