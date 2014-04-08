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
        //Write révision
        void RevisionHandler::write( vector<bool>& data, unsigned int pos, unsigned length, ofstream& stream){
            uint8_t buffer(0);
            cout<<length<<endl;
            cout<<length/8<<endl;
            for(uint64_t j=0; j<(length/8); j+=8){ //Only byte can be write in a file
                buffer=0;
                for(int k=0; k<8; k++){
                    buffer+= data[j*8+k];
                    buffer<<1;
                }
                stream<<buffer;
            }

            int mod=length % 8;
            if(  mod != 0 ){
                uint64_t lastPos=pos+length-mod;
                buffer=0;
                for(int k=0; k<mod ; k++){
                    buffer+= data[lastPos+k];
                    buffer<<1;
                }
                stream<<buffer;
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
            stream.seekp(pos, stream.beg);

            //Update first
            uint64_t i(0);
            uint64_t n = min( origine.size(), data.size() );

            uint64_t length(0);
            while( i<n ){
                if( origine[i] !=  data[i] ){
                    length++;
                }else if(length != 0){
                    stream<<Mutation::UPDATE << (i-length) << length; //type(uint8_t)beginning(uint64_t)size(uint64_t)
                    write(data, i-length, length, stream);
                    length=0;
                }
                i++;
            }

            //Delete
            if( origine.size()>data.size() )
                stream<<Mutation::DELETE << data.size() << origine.size() - data.size(); //type(uint8_t)beginning(uint64_t)size(uint64_t)

            //Insert
            if( origine.size()<data.size() ){
                stream<<Mutation::INSERT << origine.size() << data.size() - origine.size();
                write(data, origine.size(), data.size()-origine.size(), stream);
            }
        }


        /**
         *  Apply mutations
        **/
        //Place le curseur au début des données de la mutation après le header
        Mutation RevisionHandler::readMutation( ifstream& stream ){
            char c; bitset<8> buffer;
            uint8_t type(0);
            uint64_t idBegining(0);
            uint64_t size(0);

            stream.get( c );
            type = uint8_t(c);

            //idBegining
            for(int i=0; i<8; i++){//8bytes <=>uint64_t
                stream.get( c );
                buffer = bitset<8>(c);
                for(int j=0; j<8; j++){//8bit by byte
                    idBegining+=buffer[j];
                    idBegining<<1;
                }
            }

            //size
            for(int i=0; i<8; i++){//8bytes <=>uint64_t
                stream.get( c );
                buffer = bitset<8>(c);
                for(int j=0; j<8; j++){//8bit by byte
                    size+=buffer[j];
                    size<<1;
                }
            }

            Mutation m( type, idBegining, size);
            return m;
        }

        //Relative pos correspond à la position équivalente du début du flux(0) si on ne charge que des bouts de fichiers par défaut à 0
        void RevisionHandler::applyMutations( vector<bool>& data, Revision* rev, ifstream& stream, uint64_t fileSize, uint64_t relativePos){
            stream.seekg( rev->getIdBeginning() - relativePos, stream.beg );

            while( stream.tellg() < fileSize ){
                Mutation m=readMutation( stream );
                m.apply(data, stream);
            }
        }

        void RevisionHandler::recopy( ifstream& data, ofstream& newStream, uint64_t size){
            char c;
            for(uint64_t i=0; i< size;  i++){
                data.get(c);
                newStream<<c;
            }
        }

        void RevisionHandler::applyMutations( ifstream& data, ofstream& newStream, Revision* rev, ifstream& stream, uint64_t fileSize, uint64_t relativePos){
            stream.seekg( rev->getIdBeginning() - relativePos, stream.beg );

            while( stream.tellg() < fileSize ){
                Mutation m=readMutation( stream );
                if( m.getIdBeginning() != data.tellg() )
                    recopy( data, newStream, m.getIdBeginning() - data.tellg() );
                m.apply(newStream, data, stream);
            }
        }

        /**
         * Search for the best origine
        **/




    }
}
