#ifndef INCREMENTALCONTAINER_H_INCLUDED
#define INCREMENTALCONTAINER_H_INCLUDED

#include <vector>

#include "AbstractContainer.h"
#include "../IncrementalFile.h"

using namespace std;



class IncrementalContainerIndex : public AbstractContainerIndex {
	public:
		IncrementalContainerIndex() : AbstractContainer("IncrementalContainer"){}
};

/**
 * Store a set of DocumentObject with low redondances, do not used with binary data
 */
class IncrementalContainer : public AbstractContainer{
	protected:
		vector<Document> documents; ///associate a revision and a document
		uint32_t num_documents = 0; ///Already saved
		
		IncrementalFile* incrFile = NULL;
		
	public:
		IncrementalContainer() : AbstractContainer(){ incrFile = new IncrementalFile( location+"/content"); }
		IncrementalContainer(uint64_t i, uint64_t p) : AbstractContainer(i,p){ incrFile = new IncrementalFile(location+"/content"); }
		
		~IncrementalContainer(){
			delete incrFile;
			for(int i=0; i<documents.size(); i++)
				delete documents[i];
			~AbstractContainer();
		}
		
		int getDocumentsSize(){ return documents.size(); }
		Document* getDocument(int n){ return documents[i]; }
		IncrementalFile* incrFile(){ return incrFile; } 
		
		//il faut differentier les tdocuments deka sotcker et les nouveau de même pour le .xml eviter les doublons 
		//etc, seul contenaire mutable....
		
		void addDocument( Document doc){ documents.push_back( doc );}
		void addDocument( const char* id, const char* category){
			uint64_t i, c;
			(streamstring(id) )>>i;
			(streamstring(category) )>>c;
			
			documents.push_back(  Document(i,c) );
		}
};
#endif // INCREMENTALCONTAINER_H_INCLUDED
