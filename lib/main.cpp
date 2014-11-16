#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>
#include <fstream>

#include "Manager.h"
#include "Ressource.h"
#include "Revision.h"
using namespace std;


int main(){

    string str;
    ifstream s("/home/toor/Desktop/1.html.origin",ios::in);
    char c;
    while( s.get(c) )
        str += c;

    Ressource r;

    ChunkManager* cManager = new ChunkManager();
    RessourceHandler rHandler(cManager);
    vector<uint64_t> ids;
//    ids.push_back(950);
//    r.setChunkIds( ids );


///Creation ressource
    try{
        for(int i=0; i<1000; i++){
            rHandler.newRevision(r, str);
        }
    }catch( const char* e){
    }
    delete cManager;


///lecture

//
//    r.setCurrentRevision(2);
//    ofstream test("/home/toor/Desktop/2.html");
//    string a=rHandler.buildRevision(r, 2);
//    test<<a;
    return 0;
}
