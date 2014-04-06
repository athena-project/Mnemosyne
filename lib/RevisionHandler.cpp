#include "RevisionHandler.h"
#include <iostream>

namespace Athena{
    namespace Mnemosyne{

        RevisionHandler::RevisionHandler(){

        }

        RevisionHandler::~RevisionHandler(){
        }

        /**
         *      Building functions
        **/
        int RevisionHandler::getNumber( vector<bool>& table, vector<bool>::iterator& it  ){
            int n=0b0;
            for(int i=16; i>-1; i--){
                n+= *it;
                n<<1;
                it++;
            }
            return n;
        }

        uint16_t RevisionHandler::getOrigine( vector<bool>& table, vector<bool>::iterator& it ){
            uint16_t n=0b0;
            for(int i=16; i>-1; i--){
                n+= *it;
                n<<1;
                it++;
            }
            return n;
        }

        uint64_t RevisionHandler::getIdBeginning( vector<bool>& table, vector<bool>::iterator& it ){
            uint64_t n=0b0;
            for(int i=64; i>-1; i--){
                n+= *it;
                n<<1;
                it++;
            }
            return n;
        }

        uint64_t RevisionHandler::getSize( vector<bool>& table, vector<bool>::iterator& it ){
            uint64_t n=0b0;
            for(int i=64; i>-1; i--){
                n+= *it;
                n<<1;
                it++;
            }
            return n;
        }

        uint16_t RevisionHandler::getDiff( vector<bool>& table, vector<bool>::iterator& it ){
            uint16_t n=0b0;
            for(int i=16; i>-1; i--){
                n+= *it;
                n<<1;
                it++;
            }
        }

        vector<int> RevisionHandler::extractChildren( vector< int >& origines, int parent ){
            vector<int> children;
            for(int i=0; i<origines.size(); i++)
                if( parent == origines[i] )
                    children.push_back( i );
            return children;
        }

        void RevisionHandler::buildChildren( vector<int>& origines, vector< Revision* > revisions, Revision* current){
            vector<int> children = extractChildren(origines, current->getN());
            Revision* currentChild;
            for(int i=0; i<children.size(); i++){
                currentChild = revisions[ children[i] ];

                currentChild->setParent( current );
                if( currentChild->getN() > 0 )
                    currentChild->setPrevious( revisions[ currentChild->getN()-1 ] );
                else
                    currentChild->setPrevious( current ); //Current == root

                if( currentChild->getN() < children.size()-1 ){
                    currentChild->setNext( revisions[ currentChild->getN()+1 ] );
                    currentChild->setLast(  revisions[ children.size()-1 ] );
                }


                buildChildren( origines, revisions,  currentChild );
                current->addChild( currentChild );
            }
        }

        Revision* RevisionHandler::build( vector<bool>& table ){
            vector<bool>::iterator it = table.begin();
            if( it == table.end() )
                return NULL;

            int number = getNumber( table, it);
            vector< int > origines;
            vector< Revision* > revisions; // pair< begin, size >

            uint64_t tmpBegin;
            uint64_t tmpSize;
            uint16_t tmpDiff;
            while( it != table.end() ){
                tmpBegin = getIdBeginning(table, it);
                tmpSize  = getSize(table, it);
                tmpDiff  = getDiff(table,it);
                origines.push_back( getOrigine(table, it) );
                revisions.push_back( new Revision(revisions.size(), tmpBegin, tmpSize, tmpDiff) );
            }

            Revision* root=new Revision(-1);
            buildChildren( origines, revisions, root);
            root;
        }

        /**
         *  Diff functions
        **/
        void RevisionHandler::write( vector<bool>& data, unsigned int pos, unsigned length, ifstream& stream){
            uint8_t buffer(0);
            for(uint64_t j=0; j<length; j+=8){ //Only byte can be write in a file
                buffer=0;
                for(int k=0; k<8; k++){
                    buffer+= data[j-k];
                    buffer<<1;
                }
                str<<buffer;
            }

            int mod=length % 8;
            if(  mod != 0 ){
                uint64_t lastPos=pos+length-mod;
                buffer=0;
                for(int k=0; k<mod ; k++){
                    buffer+= data[lastPos+k];
                    buffer<<1;
                }
                str<<buffer;
            }
        }

        float RevisionHandler::diff( vector<bool>& origine, vector<bool>& data ){
            unsigned int num= min( origine.size(), data.size() );
            float diff = (num - origine.size()) + (num - data.size());

            for(unsigned int i=0; i<num; i++){
                if( origine[i] != data[i] )
                    diff++;
            }
            return diff/origine.size();
        }

        float RevisionHandler::diff( ifstream& origine, ifstream& data ){ //We work with bytes
            if(!origine && !data )
                throw "";

            uint64_t origineSize=1; //Avoid division by zero
            float diff=1;
            char b1,b2;

            origine.seekg( 0, origine.beg );
            data.seekg( 0, data.beg );
            while( origine.get( b1 ) && data.get( b2 ) ){
                if( b1 != b2 )
                    diff++;
                origineSize++;
            }

            if( origine.get(b1) ){
                while( origine.get( b1 ) ){
                    diff++; origineSize++;
                }
            }else if( data.get(b1) ){
                while( data.get( b1 ) )
                    diff++;
            }

            return diff/origineSize;

        }

        void RevisionHandler::createdMutations( vector<bool>& origine, vector<bool>& data, ofstream& stream, uint64_t pos){
            stream.seekd(pos, stream.beg);

            //Update first
            uint64_t i(0);
            uint64_t n = min( origine.size(), data.size() );

            uint64_t lenght(0);
            while( i<n ){
                if( origine[i] !=  data[i] ){
                    length++;
                }else if(lenght != 0){
                    stream<<Revision::UPDATE << (i-length) << lenght; //type(uint8_t)beginning(uint64_t)size(uint64_t)
                    write(data, i-length, length, stream);
                    length=0;
                }
            }

            //Delete
            if( origine.size()>data.size() )
                stream<<Revision::DELETE << data.size() << origine.size() - data.size(); //type(uint8_t)beginning(uint64_t)size(uint64_t)

            //Insert
            if( origine.size()<data.size() ){
                stream<<Revision::INSERT << origine.size() << data.size() - origine.size();
                write(data, origine.size(), data.size()-origine.size(), stream);
            }
        }

        void RevisionHandler::applyMutations
    }
}
