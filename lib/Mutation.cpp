#include "Mutation.h"

namespace Athena{
    namespace Mnemosyne{

        void Mutation::applyInsert(vector<char>& data, ifstream& stream){
            char c;
            for(uint64_t i=0; i<size; i++){
                stream.get(c);
                data.push_back( c );
            }
        }

        void Mutation::applyDelete(vector<char>& data){
            data.erase( data.begin()+idBeginning, data.begin()+idBeginning + size);
        }

        void Mutation::applyUpdate(vector<char>& data, ifstream& stream){
            char c;
            for(uint64_t i=0; i<size ; i++){
                stream.get(c);
                data[idBeginning+i] = c;
            }
        }

        void Mutation::apply( vector<char>& data, ifstream& stream){
            stream.seekg( idBeginning, stream.beg );

            if( type == INSERT )
                applyInsert( data, stream );
            else if( type == DELETE )
                applyDelete( data );
            else
                applyUpdate( data, stream );
        }
    }
}
