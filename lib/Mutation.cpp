#include "Mutation.h"

namespace Athena{
    namespace Mnemosyne{

        void Mutation::applyInsert(vector<char>& data, ifstream& stream){
            char c;
            bitset<8> buffer;
            for(uint64_t i=0; i<(size/8); i++){ //travail sur des octets
                stream.get(c);
                buffer=bitset<8>(c);
                for(int j=0; j<8; j++){
                    data.push_back( buffer[j] );
                }
            }
        }

        void Mutation::applyDelete(vector<char>& data){
            data.erase( data.begin()+idBeginning, data.begin()+idBeginning + size);
        }

        void Mutation::applyUpdate(vector<char>& data, ifstream& stream){
            char c;
            bitset<8> buffer;
            for(uint64_t i=0; i<(size/8); i++){ // /8 car on travail sur des octets
                stream.get(c);
                buffer=bitset<8>(c);
                for(int j=0; j<8; j++){
                    data[idBeginning+i+j] = buffer[j];
                }
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
