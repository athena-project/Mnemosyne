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

        string RessourceHandler::buildRevision( Ressource& r, uint32_t n ){
            if( n > r.getCurrentRevision() )
                return "";
            else if( n==r.getCurrentRevision() && r.getContent() != "")
                return r.getContent();


            RevisionHandler revHandler;
            ChunkHandler    chHandler;
            ChunkManager    chManager;
            vector<char> data;
            stringstream value;

            /// Table building, from the last chunks which host the end of the table
            vector<uint64_t> chunkIds = r.getChunkIds();
            string lastChunkLocation = chHandler.getFile( chunkIds[chunkIds.size()-1]);
            ifstream lastChunkStream( lastChunkLocation.c_str() , ios::binary);
            uint32_t nbrChuncksTable = revHandler.extractSizeTable( &lastChunkStream );
            vector< uint32_t > idChuncksTable;
            for(uint32_t i = (chunkIds.size()-nbrChuncksTable-1) ; i<chunkIds.size()  ; i++ )
                idChuncksTable.push_back( chunkIds[i] );
            vector<Chunk> chuncksTable = chManager.get( chunkIds );


            /// Building the tmpFile, which will store the revision data, from the chunkIds
            std::ostringstream tmpId;
            tmpId << r.getId();
            string location ="";

            if( chuncksTable.size() == 1 )
                location = chHandler.getFile( chuncksTable[0].getId() );
            else{
                ofstream tmpFile( (Ressource::TMP_DIR()+"/"+tmpId.str()).c_str() );
                char(c);
                for(int i=0; i< chuncksTable.size(); i++){
                    ifstream tmpStream(chHandler.getFile( chuncksTable[i].getId() ).c_str() , ios::binary);
                    while( tmpStream.get(c) ){
                        tmpFile << c;
                    }
                }
                location = Ressource::TMP_DIR()+"/"+tmpId.str();
            }

            ifstream stream( location.c_str() , ios::binary);
            vector< TableElement> table = revHandler.extractTable( &stream );

            ///Body building
            Revision* rev = revHandler.buildStructure( table ); //Root
            if( rev == rev->getRoot() )
                rev = rev->getNext();

            rev->setIStream( location.c_str() );

            while( rev->getN() != n ){
                rev = rev->getNext();
                rev->setIStream( location.c_str() );
            }


            vector< Revision* > parents = rev->getParents();
            for( int i=0 ; i<parents.size()-1 ; i++)
                revHandler.applyMutations( data, parents[i] );

            revHandler.applyMutations( data, rev); //Data is now hydrate

//            //We must converte vector<char> to string
//            for(uint64_t j=0; j<data.size(); j++)
//                value<<data[j];
//
//            if( location !="" )
//                remove( location.c_str() );
//            return value.str();
return "";
        }

        Revision* RessourceHandler::buildAllRevisions(Ressource& r){
            if( r.getCurrentRevision() == 0 )
                return new Revision();

            ChunkManager chManager;
            ChunkHandler chHandler;
            RevisionHandler revHandler;
            Revision* rev = new Revision();
            vector<Chunk> chuncksTable = chManager.get( r.getChunkIds() );


            // Building the tmpFile, which will store the revision data, from the chunks
            std::ostringstream tmpId;
            tmpId << r.getId();
            string location ="";
            ofstream tmpFile( (Ressource::TMP_DIR()+"/"+tmpId.str()).c_str() );
                char(c);
            for(int i=0; i< chuncksTable.size(); i++){
                ifstream tmpStream( chHandler.getFile( chuncksTable[i].getId() ).c_str() , ios::binary);
                while( tmpStream.get(c) ){
                    tmpFile << c;
                }
            }
            string streamLocation = (Ressource::TMP_DIR()+"/"+tmpId.str()).c_str() ;
            ifstream* stream = new ifstream( streamLocation.c_str() );


            vector< TableElement> table = revHandler.extractTable( stream );

            //Body building
            rev = revHandler.buildStructure( table ); //Root
            while( rev->getNext() != NULL ){
                rev->setIStream( streamLocation );
                rev = rev->getNext();
            }

            return rev;

        }

        //Input is the ressource content
        void RessourceHandler::newRevision( Ressource* r, string dataStr ){
            vector< char > data;
            for(uint64_t i=0; i<dataStr.size(); i++)
                data.push_back( dataStr[i] );

            RevisionHandler revHandler;
            Revision* newRev = revHandler.newRevision( r->getRevision(),  data );

            ///Maj de l'instance courrante
            r->setCurrentRevision( r->getCurrentRevision() + 1 );


            ///Création des nv chunk
            vector< Chunk > chunks = r->getChunks();
            ChunkHandler cHandler;
            uint64_t sizeUpdate = newRev->getSize() + (newRev->getLast()->getN() + 1) * Revision::REVISION_SIZE_TABLE;
            uint64_t sizeUpdateLastChunk = (chunks.size() == 0) ? 0 : min(sizeUpdate, (uint64_t)Chunk::CHUNK_SIZE_MAX);
            ifstream* currentStream = newRev->getIStream();



            currentStream->seekg(newRev->getIdBeginning());
            if(chunks.size() > 0)
                cHandler.updateData( chunks[ chunks.size()-1 ], *currentStream, newRev->getIdBeginning(), sizeUpdateLastChunk);
            currentStream->seekg( newRev->getIdBeginning() + sizeUpdateLastChunk );
            cHandler.makeChunks( *currentStream, newRev->getIdBeginning()+sizeUpdateLastChunk, sizeUpdate-sizeUpdateLastChunk);

        }
    }
}
