#include "Ressource.h"

namespace Athena{
    namespace Mnemosyne{

        Ressource::
        string Ressource::buildRevision( uint32_t n ){
            RevisionHandler revHandler();
            ChunkHandler    chHandler();
            ChunkManager    chManager();
            vector<char> data;
            stringstream value;

            // Table building
            uint32_t sizeTable = revHandler.extractSizeTable();
            uint32_t nbrChuncksTable = ceil( float(CHUNKS::SIZE_MAX) / float(sizeTable) );
            list< uint32_t > idChuncksTable;
            for(uint32_t i = chunks.size() ; i>chunks.size()-nbrChuncksTable ; i++ ){
                idChuncksTable.push_back( chunks[i] );
            }

            vector<Chunk> chuncksTable = chunchManager.get( idChuncksTable );
            ifstream stream;

            if( chuncksTable.size() == 0 )
                throw "No revision";
            else if( chuncksTable.size() == 1 )
                stream = chHandler.getFile( chuncksTable[0] );
            else{
                ofstream tmpFile( RESSOURCE::TMP_DIR+"/"+string(id), ios::out );
                char(c);
                for(int i=0; i< chuncksTable.size; i++){
                    stream = chHandler.getFile( chuncksTable[i] );
                    while( stream.get(c) ){
                        tmpFile << c;
                    }
                }
                stream = ifstream( RESSOURCE::TMP_DIR+"/"+string(id), ios::binary);
            }
            vector< char> table = extractTable( stream );
            ~stream();
            ~tmpFile();
            remove( RESSOURCE::TMP_DIR+"/"+string(id) );


            //Corps building
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

            return value.str();
        }

        //Input is the ressource content
        void Ressource::newRevision( string dataStr ){
            vector< char > data;
            for(uint64_t i=0; i<dataStr.size(); i++)
                data.push_back( dataStr[i] );

            RevisionHandler revHandler();
            Revision* origin = revHandler.bestOrgin( rev, data );
            uint32_t tableSize = revHandler.extractSizeTable( rev.getStream() );

            vector<char> tmpData;
            list< Revision* > parents = rev->getParents();
            for( list< Revision* >::iterator it = parents.begin() ; it!=parents.end() ; it++ )
                revHandler.applyMutations( tmpData, *it);

            revHandler.applyMutations( tmpData, rev); //Data is now hydrate

            revHandler.createdMutations(tmpData, data, rev.getStream(), -1*tableSize);
        }
    }
}
