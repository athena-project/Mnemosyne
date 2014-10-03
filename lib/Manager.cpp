#include "Manager.h"

Manager::Manager(){
	conn = SQLFactory::getSQLConnexion();
}

Manager::Manager( mysqlpp::Connection& co ){
	conn=co;
}

Manager::~Manager(){}
