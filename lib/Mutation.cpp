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


        void Mutation::applyInsert(ofstream& newFile, ifstream& stream){
            char c;
            for(uint64_t i=0; i< size; i++){
                stream.get(c);
                newFile<<c;
            }
        }

        void Mutation::applyDelete(){
        }

        void Mutation::applyUpdate( ofstream& newFile, ifstream& stream){
            char c;
            for(uint64_t i=0; i< size; i++){
                stream.get(c);
                newFile<<c;
            }
        }



        void Mutation::apply( vector<bool>& data, ifstream& stream){
            stream.seekg( idBeginning, stream.beg );

            if( type == INSERT )
                applyInsert( data, stream );
            else if( type == DELETE )
                applyDelete( data );
            else
                applyUpdate( data, stream );
        }

        void Mutation::apply(ifstream& oldFile, ofstream& newFile, ifstream& stream){
            stream.seekg( idBeginning, stream.beg );

            if( type == INSERT )
                applyInsert( newFile, stream );
            else if( type == DELETE ){
                applyDelete( );
                oldFile.seekg( size, oldFile.cur );
            }else{
                applyUpdate( newFile, stream );
                oldFile.seekg( size, oldFile.cur );
            }
        }
    }
}
