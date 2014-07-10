#include <iostream>
#include <fstream>
#include <bitset>
#include <stdint.h>
#include <vector>

#include <boost/filesystem.hpp>

#include "Ressource.h"
#include "RevisionHandler.h"
using namespace std;
using namespace Athena::Mnemosyne;



int main(){
    string str;
    ifstream s("/home/severus/Desktop/t1",ios::in);
    char c;
    while( s.get(c) )
        str += c;


    Ressource re("url","contentType", 5,"abcde", 10);

    RevisionHandler* handler= new RevisionHandler();
    ofstream stream("/home/severus/test1", ios::app);

//    handler->createdMutations(s1 , s2, stream, 0);
//    ifstream s1( "/home/severus/s1", ios::binary);
//    ifstream s2 ("/home/severus/s2", ios::binary);
//    cout<<handler->diff(s1,s2)<<endl;
    delete handler;

    return 0;
}
