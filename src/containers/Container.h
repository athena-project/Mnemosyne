#ifndef CONTAINER_H_INCLUDED
#define CONTAINER_H_INCLUDED

#include "AbstractContainer.h"
#include "../../../../Hermes/trunk/src/Document.h"
#include "../../../../Hermes/trunk/src/Metadata.h"

using namespace std;



class ContainerIndex : public AbstractContainerIndex{
	public:
		ContainerIndex() : AbstractContainerIndex("Container"){}
};

/**
 * Store a DocumentObject
 */
class Container : public AbstractContainer{
	protected:
		Metadata* metadata = NULL;
		Document* document = NULL;

	public:
		Container() : AbstractContainer(){}
		Container(uint64_t i, uint64_t p) : AbstractContainer(i,p){}
		Container(Metadata* meta, Document* doc) : Container(), metadata(meta), document(doc){}
		Container(uint64_t i, uint64_t p, Metadata* meta, Document* doc) : Container(i, p), Container(meta, doc) {}

		Metadata* getMetadata(){ return metadata; }
		Document* getDocument(){ return document; }

		/**
		 * copy the document (ie the data) to dir/document-identificateur
		 *
		 */
		virtual void copyDocumentTo(string dir);
		virtual void copyMetadataTo(string dir);

		/**
		 * All data load in memory, not a good idea to used it without irresistible urge
		 * @return the document store in this object, the managment of the document is left to the client.
		 */
		//Document* readDocument();

		/**
		 * @return the metadata store in this object, the managment of the metadata is left to the client.
		 */
		Metadata* readMetadata();

};
#endif // CONTAINER_H_INCLUDED
