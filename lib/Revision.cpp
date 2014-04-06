#include "Revision.h"

namespace Athena{
    namespace Mnemosyne{
            Revision::Revision(){}
            Revision::~Revision(){
                for(int i=0; i<children.size(); i++)
                    delete children[i];
            }



    }
}
