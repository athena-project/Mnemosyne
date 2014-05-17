#include "Manager.h"

namespace Athena{
    namespace Mnemosyne{
        Manager::Manager(){
            conn = SQLFactory::getSQLConnexion();
        }

        Manager::Manager( mysqlpp::Connection& co ){ conn=co; }

        Manager::~Manager(){}

    }
}
