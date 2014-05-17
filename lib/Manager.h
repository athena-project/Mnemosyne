#ifndef MANAGER_H_INCLUDED
#define MANAGER_H_INCLUDED

#include <mysql++/mysql++.h>

#include "SQLFactory.h"

using namespace std;

namespace Athena{
    namespace Mnemosyne{
        class Manager{
            protected :
                mysqlpp::Connection conn;
            public :
                Manager();
                Manager( mysqlpp::Connection& co );
                virtual ~Manager();

                //virtual get( string queryStr ) = 0;
        };
    }
}

#endif // MANAGER_H_INCLUDED
