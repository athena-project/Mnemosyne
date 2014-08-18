#include "Ressource.h"


namespace Athena{
    namespace Mnemosyne{

    ///Ressource
        Ressource::Ressource(){
            rev = new Revision();
        }

        Ressource::~Ressource(){
             delete rev;
        }

        bool Ressource::empty(){
            return (content == "");
        }


    ///RessourceHandler

        vector< TableElement> RessourceHandler::x_buildTable( Ressource& r ){
            ChunkHandler    chHandler;
            ChunkManager    chManager;
            RevisionHandler revHandler;

            /// Getting all chunks needed to the building of the table
            vector<uint64_t> chunkIds = r.getChunkIds();
            string lastChunkLocation = chHandler.getFile( chunkIds[chunkIds.size()-1]);
            ifstream lastChunkStream( lastChunkLocation.c_str() , ios::binary);
            uint32_t nbrChuncksTable = revHandler.extractSizeTable( &lastChunkStream );
            vector< uint32_t > idChuncksTable;
            for(uint32_t i = (chunkIds.size()-nbrChuncksTable-1) ; i<chunkIds.size()  ; i++ )
                idChuncksTable.push_back( chunkIds[i] );
            vector<Chunk> chuncksTable = chManager.get( chunkIds );

            ///Tmp file
            std::time_t timestamp = std::time(0);  // t is an integer type
            std::ostringstream tmpId;
            tmpId << r.getId();
            tmpId << timestamp;
            tmpId << "table";

            string location = Ressource::TMP_DIR()+"/"+tmpId.str() ;

            ofstream tableStream ( location.c_str(), ios::binary );

            for( int i=0; i<chuncksTable.size(); i++){
                ifstream tmpStreamTable( chHandler.getFile( chuncksTable[i].getId() ).c_str(), ios::binary );
                char* buffer = new char[ chuncksTable[i].getSize() ];
                tmpStreamTable.read( buffer, chuncksTable[i].getSize() );
                tableStream.write( buffer, chuncksTable[i].getSize() );
                delete[] buffer;
            }
            tableStream.flush();

            ifstream stream( location.c_str(), ios::binary);
            vector< TableElement> table = revHandler.extractTable( &stream );

            std::remove( location.c_str() );
            return table;
        }

        string RessourceHandler::buildRevision( Ressource& r, uint32_t n ){
            if( n > r.getCurrentRevision() )
                return "";
            else if( n==r.getCurrentRevision() && r.getContent() != "")
                return r.getContent();

            RevisionHandler revHandler;
            ChunkHandler    chHandler;
            ChunkManager    chManager;
            vector<Chunk> chunks = r.getChunks();
            vector< string > tmpFiles; //All files which must be deleted
            vector<char> data;
            stringstream value;

            ///Revision's tree building
            vector< TableElement> table =  x_buildTable( r );
            Revision* rev = revHandler.buildStructure( table ); //Root
            while( rev->getN() != n && rev->getNext() != NULL )
                rev = rev->getNext();

             ///Loading only chunks needed
             vector<Revision*> parents = rev->getParents();
             vector<Revision*>::iterator it = parents.begin();
             vector< pair<uint64_t, uint64_t> > extrematIds; // min,max , chunkIds by revision needed
             Revision* current = rev->getRoot();
             int i=0; uint64_t s=0;  //Size indice

                ///Calulate chunks needed
                 while( it != parents.end() ){
                    if( (*it)->getN() == i ){
                        uint64_t m = floor( double(s+1) / double(Chunk::CHUNK_SIZE_MAX) );
                        uint64_t M = floor( double(s+(*it)->getSize()) / double(Chunk::CHUNK_SIZE_MAX) );
                        extrematIds.push_back( pair<uint64_t, uint64_t>(m,M) );
                        it++;
                    }
                    i++;
                    s+= current->getSize();
                    current = current->getNext();
                 }

                ///Hydrating rev tree
                 uint64_t lastMax = 0;
                 for(int j=0; j<extrematIds.size(); j++){
                    ///Tmp file
                    std::time_t timestamp = std::time(0);  // t is an integer type
                    std::ostringstream tmpId;
                    tmpId << r.getId();
                    tmpId << timestamp;
                    tmpId << extrematIds[j].first << extrematIds[j].second;

                    string location = Ressource::TMP_DIR()+"/"+tmpId.str();

                    ofstream stream ( location.c_str(), ios::binary );

                    ///Filling tmpfile
                    for(int k=extrematIds[j].first; k<=extrematIds[j].second; k++){
                        uint64_t size = chunks[ k ].getSize();
                        const char* tmpLocation = chHandler.getFile( chunks[ k ].getId() ).c_str();
                        ifstream tmpStream( tmpLocation, ios::binary);

                        char* buffer = new char[ size ];
                        tmpStream.read( buffer, size);
                        stream.write( buffer, size);
                        delete[] buffer;
                    }
                    stream.close();
                    tmpFiles.push_back( location.c_str() );

                    parents[j]->setIStream( location );
                 }

            ///Hydrating the data
            for( int i=0 ; i<int(parents.size())-1 ; i++)
                revHandler.applyMutations( data, parents[i] );

            revHandler.applyMutations( data, rev); //Data is now hydrate

            ///We must converte vector<char> to string
            for(uint64_t j=0; j<data.size(); j++)
                value<<data[j];

            for( int l=0; l<tmpFiles.size() ; l++)
                std::remove( tmpFiles[l].c_str() );

            return value.str();
        }

        Revision* RessourceHandler::buildAllRevisions(Ressource& r){
            Revision* rev;

            // Building the tmpFile, which will store the revision data, from the chunks
            std::time_t timestamp = std::time(0);  // t is an integer type
            std::ostringstream tmpId;
            tmpId << r.getId();
            tmpId << timestamp;

            string location = (Ressource::TMP_DIR()+"/"+tmpId.str()) ;
            ofstream tmpFile( location.c_str() );


            if( r.getChunkIds().size() == 0 ){
                rev = new Revision();
                rev->setIStream( location );
                rev->setOStream( location );
                return rev;
            }

            ChunkManager chManager;
            ChunkHandler chHandler;
            RevisionHandler revHandler;
            vector<Chunk> chuncksTable = chManager.get( r.getChunkIds() );

            char c;
            for(int i=0; i< chuncksTable.size(); i++){
                ifstream tmpStream( chHandler.getFile( chuncksTable[i].getId() ).c_str() , ios::binary);
                int length = chuncksTable[i].getSize();

                char * buffer = new char [ length ];
                tmpStream.read (buffer, length );
                tmpFile.write (buffer, length);
                delete[] buffer;

                tmpStream.close();
            }
            tmpFile.flush();
            tmpFile.close();


            ifstream stream( location.c_str() );
            vector< TableElement> table = revHandler.extractTable( &stream );

            //Body building
            rev = revHandler.buildStructure( table ); //Root
            rev->setIStream( location );
            rev->setOStream( location );

            while( rev->getNext() != NULL ){
                rev = rev->getNext();
                rev->setIStream( location );
                rev->setOStream( location );
            }

            return rev;
        }

        //Input is the ressource content
        void RessourceHandler::newRevision( Ressource* r, string dataStr ){
            vector< char > data;
            for(uint64_t i=0; i<dataStr.size(); i++)
                data.push_back( dataStr[i] );

            RevisionHandler revHandler;
            Revision* rev = buildAllRevisions( *r );
            rev = revHandler.bestOrigin( rev, data );

            ///Maj de l'instance courrante
            Revision* newRev = revHandler.newRevision( rev,  data );
            r->setCurrentRevision( r->getCurrentRevision() + 1 );

            ///Création des nv chunk
            vector< Chunk > chunks = r->getChunks();
            ChunkHandler cHandler;
            uint64_t sizeUpdate = newRev->getSize() + newRev->getLast()->getN()  * Revision::REVISION_SIZE_TABLE + 2;
            uint64_t sizeUpdateLastChunk = (chunks.size() == 0) ? 0 : min(sizeUpdate, (uint64_t)Chunk::CHUNK_SIZE_MAX);
            uint64_t offset = (newRev->getLast()->getN()-1) * Revision::REVISION_SIZE_TABLE + 2;
            ifstream* currentStream = newRev->getIStream();

            if(chunks.size() > 0)
                cHandler.updateData( chunks[ chunks.size()-1 ], *currentStream, newRev->getIdBeginning(), sizeUpdateLastChunk, offset);
            cHandler.makeChunks( *currentStream, newRev->getIdBeginning()+sizeUpdateLastChunk, sizeUpdate-sizeUpdateLastChunk);

            std::remove( rev->getIStreamLocation().c_str() );
        }
    }
}
