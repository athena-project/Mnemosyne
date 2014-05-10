#include "RevisionHandler.h"
#include <iostream>

namespace Athena{
    namespace Mnemosyne{

//        vect<bool> to vect<char>

        RevisionHandler::RevisionHandler(){

        }

        RevisionHandler::~RevisionHandler(){
        }

        /**
         *      Building functions
        **/
        uint32_t RevisionHandler::extractSizeTable( ifstream& stream ){
            uint16_t number = 0;
            vector< uint8_t > table;
            char c;
            stream.seekg(-2, stream.end );//revision number is a uint16_t

            stream.get(c);
            number = uint16_t(c);
            number << 8;

            stream.get(c);
            number +=  uint16_t(c);

            return uint32_t(number) * Revision::REVISION_SIZE_TABLE;
        }

        vector<char> RevisionHandler::extractTable(ifstream& stream){
            uint32_t sizeTable = extractSizeTable( stream );
            uint32_t beginningTable = -1 * ( sizeTable +2);
            vector< char > table;
            char c;

            for( uint32_t i=0; i<sizeTable; i++){
                stream.get(c);
                table.push_back( c );
            }

            stream.seekg (-2, stream.beg );
            return table;
        }

        uint16_t RevisionHandler::getOrigine( vector<char>& table, vector<char>::iterator& it ){
            uint16_t n=0b0;
            for(int i=0; i<2; i++){
                n+= *it;
                n<<1;
                it++;
            }
            return n;
        }

        uint64_t RevisionHandler::getIdBeginning( vector<char>& table, vector<char>::iterator& it ){
            uint64_t n=0b0;
            for(int i=0; i<8; i++){
                n+= *it;
                n<<1;
                it++;
            }
            return n;
        }

        uint64_t RevisionHandler::getSize( vector<char>& table, vector<char>::iterator& it ){
            uint64_t n=0b0;
            for(int i=0; i<8; i++){
                n+= *it;
                n<<1;
                it++;
            }
            return n;
        }

        uint16_t RevisionHandler::getDiff( vector<char>& table, vector<char>::iterator& it ){
            uint16_t n=0b0;
            for(int i=0; i<2; i++){
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

        Revision* RevisionHandler::buildStructure( vector<char>& table ){
            vector<char>::iterator it = table.begin();
            if( it == table.end() )
                return NULL;

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

        void RevisionHandler::writeTable( vector<char>& table, ofstream& stream){
            stream.seekp( -1, stream.end );
            for( uint64_t i=0; i<table.size(); i++){
                stream<<table[i];
            }
        }

        /**
         *  Diff functions
        **/
        //Write révision
        void RevisionHandler::write( vector<char>& data, uint64_t pos, uint64_t length, ofstream& stream){
            for(uint64_t j=pos; j<length; j++)
                stream<<data[j];
        }

        uint64_t RevisionHandler::diff( vector<char>& origine, vector<char>& data ){
            uint64_t num= min( origine.size(), data.size() );
            uint64_t diff = max( origine.size(), data.size() ) - num;

            for(uint64_t i=0; i<num; i++){
                if( origine[i] != data[i] )
                    diff++;
            }
            return diff;
        }




        //pos position a partir de la fin du fichier
        void RevisionHandler::createdMutations( vector<char>& origine, vector<char>& data, ofstream& stream, uint64_t pos){
            stream.seekp(pos, stream.end);

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
                idBegining+=c;
                idBegining << 8;
            }

            //size
            for(int i=0; i<8; i++){//8bytes <=>uint64_t
                stream.get( c );
                size+=c;
                size << 8;
            }

            Mutation m( type, idBegining, size);
            return m;
        }

        //Relative pos correspond à la position équivalente du début du flux(0) si on ne charge que des bouts de fichiers par défaut à 0
        void RevisionHandler::applyMutations( vector<char>& data, Revision* rev){
            ifstream* stream = (rev->getStream());
            Mutation m;
            uint64_t i =0;
            stream->seekg( rev->getIdBeginning() - rev->getRelativeO(), stream->beg );


            while( i < rev->getSize() ){
                Mutation m=readMutation( *stream );
                m.apply(data, *stream);
                i++;
            }
        }

        /**
         * Search for the best origine
        **/
        /**
         * @return vector order by n° of revision
        */
        vector< uint64_t > RevisionHandler::calculDifferences( Revision* rev,  vector<char>& data ){
            vector< char > tmpData(data);
            vector< uint64_t > differences;
            Revision* tmp = (rev->getRoot()); //Diff with racine <=> rev origine

            while( tmp != NULL ){
                applyMutations( tmpData, tmp);
                differences.push_back( diff(tmpData, data) );
                tmp = tmp->getNext();
            }
            return differences;
        }


        /**
         *
         *  Update : not to calcul all differences ??
        **/
        Revision* RevisionHandler::bestOrgin( Revision* rev,  vector<char>& data ){
            if( rev->getN() == -1 )//root
                return rev;

            vector< uint64_t > differences = calculDifferences( rev, data);
            uint64_t tmpDiff=differences[0];
            Revision* tmpRev = rev->getRoot();

            for( int i=1; i<differences.size(); i++){
                if( tmpDiff > differences[i] ){
                    tmpDiff = differences[i];
                    tmpRev    = tmpRev->getNext();
                }
            }
            return tmpRev;
        }





    }
}
