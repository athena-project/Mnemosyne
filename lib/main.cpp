#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>

#include <boost/filesystem.hpp>

#include "Ressource.h"
#include "Revision.h"
using namespace std;
using namespace Athena::Mnemosyne;



int main(){
    string str;
    ifstream s("/home/severus/Desktop/1.html",ios::in);
    char c;
    while( s.get(c) )
        str += c;


    cout<<"str size"<<str.size()<<endl;
    Ressource r;
    RessourceHandler rHandler;

        r.getRevision()->setIStream( "/home/severus/Desktop/1.stream");
        r.getRevision()->setOStream( "/home/severus/Desktop/1.stream");

///Creation ressource

    rHandler.newRevision(&r, str);


///lecture
//    vector<uint64_t> ids;
//    ids.push_back(3);
//    r.setChunkIds( ids );
//    rHandler.buildRevision(r, 0);

    return 0;
}
