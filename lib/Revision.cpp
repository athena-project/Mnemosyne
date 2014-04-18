#include "Revision.h"

namespace Athena{
    namespace Mnemosyne{
            Revision::~Revision(){
                for(int i=0; i<children.size(); i++)
                    delete children[i];
                if( stream != NULL )
                    delete stream;
            }

//            vector<bool> Revision::applyTo(){}
//
//            vector<bool> Revision::buildV(){
//                if( type != -1 )
//                    return applyTo( parent->buildV() );
//                return vector<bool>();
//            }


    }
}
