#include "Mutation.h"

namespace Athena{
    namespace Mnemosyne{

        void Mutation::applyInsert(vector<bool>& data){
        }
        void Mutation::applyDelete(vector<bool>& data){};
        void Mutation::applyUpdate(vector<bool>& data){};
        void Mutation::applyInsert(ofstream& data){};
        void Mutation::applyDelete(ofstream& data){};
        void Mutation::applyUpdate(ofstream& data){};

        void Mutation::apply( vector<bool>& data){
            switch( type ){
                case INSERT :
                    applyInsert( data );
                break;
                case DELETE :
                    applyDelete( data );
                break;
                case UPDATE :
                    applyUpdate( data );
                break;
            }
        }
        void Mutation::apply( ofstream& data){
            switch( type ){
                case INSERT :
                    applyInsert( data );
                break;
                case DELETE :
                    applyDelete( data );
                break;
                case UPDATE :
                    applyUpdate( data );
                break;
            }
        }
    }
}
