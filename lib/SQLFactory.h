#ifndef SQLFACTORY_H_INCLUDED
#define SQLFACTORY_H_INCLUDED


#include <mysql++/mysql++.h>

using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class SQLFactory{
            public :
                static mysqlpp::Connection getSQLConnexion();
        };
    }
}

#endif // SQLFACTORY_H_INCLUDED
