#include "Mutation.h"

namespace Athena{
    namespace Mnemosyne{

        void Mutation::applyInsert(vector<bool>& data, ifstream& stream){
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

        void Mutation::applyDelete(vector<bool>& data){
            data.erase( data.begin()+idBeginning, data.begin()+idBeginning + size);
        }

        void Mutation::applyUpdate(vector<bool>& data, ifstream& stream){
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

        void Mutation::applyInsert(ofstream& newStream, ifstream& stream){
            char c;
            for(uint64_t i=0; i< size;  i++){
                stream.get(c);
                newStream<<c;
            }
        }

        void Mutation::applyDelete(ifstream& data){
            data.seekg( size, ios::cur);
            //Nothing to do
        }

        void Mutation::applyUpdate(ofstream& newStream, ifstream& data, ifstream& stream){
            data.seekg( size, ios::cur);
            char c;
            for(uint64_t i=0; i< size;  i++){
                stream.get(c);
                newStream<<c;
            }
        }

        void Mutation::apply( vector<bool>& data, ifstream& stream){
            if( type == INSERT )
                applyInsert( data, stream );
            else if( type == DELETE )
                applyDelete( data );
            else
                applyUpdate( data, stream );
        }

        void Mutation::apply( ofstream& newStream, ifstream& data, ifstream& stream){
            if( type == INSERT )
                applyInsert( newStream, stream );
            else if( type == DELETE )
                applyDelete( data );
            else
                applyUpdate( newStream, data, stream );
        }
    }
}
