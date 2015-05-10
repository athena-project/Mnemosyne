#include "IncrementalContainer.h"

/// IncrementalContainerIndex

void IncrementalContainerIndex::build( AbstractContainer* abs){///here AbstractContainer is a DirContainer
	AbstractContainerIndex::build( abs );
	XMLElement* revisions = xml->NewElement("revisions");
	
	
	for(int i=0; i<abs->getDocumentsSize(); i++){
		XMLElement* document = xml->NewElement("document");
		XMLElement* document_id = xml->NewElement("id");
		XMLElement* document_class = xml->NewElement("category");
		
		document_id->setText( abs->getDocument()->getId() ); 
		document_class->setText( abs->getDocument()->getClass() ); 

		document->InsertLastChild( document_id );
		document->InsertLastChild( document_class );
		revisions->InsertLastChild( document );
	}
	
	XMLElement* num_chunks = xml->NewElement("num_chunks");
	num_chunks->setText( abs->incrFile()->getNum_chunks() );
	
	XMLElement* num_revisions = xml->NewElement("num_revisions");
	num_revisions->setText( abs->incrFile()->getNum_revisions() );
	
	root->InsertLastChild( num_revisions );
	root->InsertLastChild( num_chunks );
	root->InsertLastChild( revisions );
}

void IncrementalContainerIndex::hydrate( AbstractContainer* abs){
	AbstractContainerIndex::hydrate( abs );
	XMLElment* elmt = root;
	elmt = elmt->FirstChildElement();
		
	elmt->NextSiblingElement("num_revisions");
	abs->incrFile()->setNumChunks( elmt->GetText() );
	elmt->NextSiblingElement("num_chunks");
	abs->incrFile()->setId( elmt->GetText() );
	elmt->NextSiblingElement("revisions");
	
	///Now building documents
	elmt->FirstChildElement("document");
	while( elmt != NULL ){
		const char* id = elmt->FirstChildElement("id")->GetText();
		const char* category = elmt->FirstChildElement("category")->GetText();
		abs->addDocument( id, category );
		elmt->NextSiblingElement("document");
		
		num_documents++;
	}
}


/// IncrementalContainer

void IncrementalContainer::build{
	/// Create new rev
	for(int i=num_documents; i<documents.size(); i++)
		incrFile->newRevision( WorkingRev( documents[i] ) ); //a implementer dans working rev
	num_documents = documents.size();

	AbstractContainer::build();
	incrFile->save();
}
