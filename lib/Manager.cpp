#include "Manager.h"

namespace Athena{
    namespace Mnemosyne{
        Manager::Manager(){
            conn = SQLFactory::getSQLConnexion();
        }


    }
}
