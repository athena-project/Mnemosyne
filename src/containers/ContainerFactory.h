#ifndef CONTAINERFACTORY_H_INCLUDED
#define CONTAINERFACTORY_H_INCLUDED

#include "AbstractContainer.h"
#include "Container.h"
#include "X_Container.h"
#include "IncrementalContainer.h"
#include "DirContainer.h"
#include "TorrentContainer.h"

using namespace std;

class ContainerFactory{
	protected:
	
	public:
		ContainerFactory(){}
		
		template<typename T>
		static AbstractContainer* getFromName( T name ){
			switch( name ){
				case "Container":
					return new Container();
				break;
				case "X_Container":
					return new X_Container();
				break;
				case "IncrementalContainer":
					return new IncrementalContainer();
				break;
				case "DirContainer":
					return new DirContainer();
				break;
				case "TorrentContainer":
					return new TorrentContainer();
				break;
				case "WebContainer":
					return new WebContainer();
				break;
				default:
					return new AbstractContainer();
				break;
			}
			
		}
};

#endif // CONTAINERFACTORY_H_INCLUDED
