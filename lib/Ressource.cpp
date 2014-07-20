#include "Ressource.h"


namespace Athena{
    namespace Mnemosyne{

    ///Ressource
        Ressource::Ressource(){}

        Ressource::~Ressource(){}

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

            // Table building, from the last chunks which host the end of the table
            vector<uint64_t> chunkIds = r.getChunkIds();
            ifstream lastChunkStream( chHandler.getFile( chunkIds[chunkIds.size()-1] ) .c_str() , ios::binary);
            uint32_t sizeTable = revHandler.extractSizeTable( lastChunkStream );
            uint32_t nbrChuncksTable = ceil( (float)sizeTable / (float)(Chunk::CHUNK_SIZE_MAX) );
            vector< uint32_t > idChuncksTable;
            for(uint32_t i = (chunkIds.size()-nbrChuncksTable-1); i<chunkIds.size()  ; i++ )
                idChuncksTable.push_back( chunkIds[i] );
            vector<Chunk> chuncksTable = chManager.get( chunkIds );


            // Building the tmpFile, which will store the revision data, from the chunkIds
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
            vector< char> table = revHandler.extractTable( stream );


            //Body building
            Revision* rev = revHandler.buildStructure( table ); //Root
            while( rev->getN() != n )
                rev = rev->getNext();

            list< Revision* > parents = rev->getParents();
            for( list< Revision* >::iterator it = parents.begin() ; it!=parents.end() ; it++ )
                revHandler.applyMutations( data, *it);

            revHandler.applyMutations( data, rev); //Data is now hydrate

            //We must converte vector<char> to string
            for(uint64_t j=0; j<data.size(); j++)
                value<<data[j];

            if( location !="" )
                remove( location.c_str() );
            return value.str();
        }

        /**
         *  @return ifstream* - is the stream use by each revision
        **/
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
            ifstream* stream = new ifstream( (Ressource::TMP_DIR()+"/"+tmpId.str()).c_str() );


            vector< char> table = revHandler.extractTable( *stream );

            //Body building
            rev = revHandler.buildStructure( table ); //Root
            while( rev->getNext() != NULL ){
                rev->setIStream( stream );
                rev = rev->getNext();
            }

            return rev;

        }

        //Input is the ressource content
        void RessourceHandler::newRevision( Ressource* r, string dataStr ){
            vector< char > data;
            for(uint64_t i=0; i<dataStr.size(); i++)
                data.push_back( dataStr[i] );

            Revision* rev = r->getRevision();
            RevisionHandler* revHandler =new RevisionHandler();
            Revision* origin = revHandler->bestOrigin( rev, data );
            uint32_t tableSize = revHandler->extractSizeTable( *rev->getIStream() );

            vector<char> tmpData;
            list< Revision* > parents = origin->getParents();
            for( list< Revision* >::iterator it = parents.begin() ; it!=parents.end() ; it++ )
                revHandler->applyMutations( tmpData, *it);

            revHandler->applyMutations( tmpData, origin); //Data is now hydrate
            Revision* newRev = revHandler->newRevision( origin,  data );

            ///Maj de l'instance courrante
            r->setCurrentRevision( r->getCurrentRevision() + 1 );


            ///Création des nv chunk
            vector< Chunk > chunks = r->getChunks();
            ChunkHandler* cHandler = new ChunkHandler();
            uint64_t sizeUpdate = min(newRev->getSize(), (uint64_t)Chunk::CHUNK_SIZE_MAX);
            ifstream* currentStream = newRev->getIStream();

            currentStream->seekg(newRev->getIdBeginning());
            cHandler->updateData( chunks[ chunks.size()-1 ], *currentStream, newRev->getIdBeginning(), sizeUpdate);
            currentStream->seekg(newRev->getIdBeginning()+sizeUpdate);
            cHandler->makeChunks( *currentStream, newRev->getIdBeginning()+sizeUpdate, newRev->getSize()-sizeUpdate);

            delete revHandler;
        }
    }
}
