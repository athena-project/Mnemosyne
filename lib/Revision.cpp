#include "Revision.h"


namespace Athena{
    namespace Mnemosyne{

        /**
         *  Revision
         */
            Revision::Revision(){
                root=this;
                initPtr();
            }

            Revision::Revision(uint16_t num) : n(num){
                initPtr();
            }

            Revision::Revision(uint16_t num, uint64_t id, uint64_t s, uint16_t d) : n(num), idBeginning(id), size(s), diff(d){
                initPtr();
            }

            void Revision::initPtr(){
                parent = NULL;
                previous = NULL;
                next = NULL;
                last = NULL;
                root = NULL;

                oStream=NULL;
                iStream=NULL;
            }

            Revision::~Revision(){
                for(int i=0; i<children.size(); i++)
                    delete children[i];
            }

            vector< Revision* > Revision::getParents(){
                if( parent != NULL && this != this->getRoot() ){
                    vector< Revision* > parents = parent->getParents();
                    parents.push_back( this );
                    return parents;
                }else
                    return vector< Revision* >();
            }

        /**
         *  Revision Handler
         */

            RevisionHandler::RevisionHandler(){}

            RevisionHandler::~RevisionHandler(){}

            uint16_t RevisionHandler::extractSizeTable( ifstream* stream ){
                if( stream == NULL )
                    return 0;

                uint16_t number = 0;
                stream->seekg(-2, stream->end );
                stream->read((char*)&number, 2);

                return number;
            }

            vector<TableElement> RevisionHandler::extractTable(ifstream* stream){
                if( stream == NULL )
                    return vector<TableElement>();

                vector<TableElement> table;
                uint16_t sizeTable = extractSizeTable( stream );
                int newOrigin = (-1)* ( sizeTable * Revision::REVISION_SIZE_TABLE + 2);//+2 : number of table element uint16_t
                stream->seekg(newOrigin , stream->end);

                for(uint16_t i=0 ; i<sizeTable ; i++ ){
                    TableElement element;
                    stream->read((char*)&element.idBeginning, 8);
                    stream->read((char*)&element.size, 8);
                    stream->read((char*)&element.diff, 2);
                    stream->read((char*)&element.origin, 2);
                    table.push_back( element );

                    cout<<"extract table :: id "<<element.idBeginning<<endl;
                    cout<<"extract table :: size "<<element.size<<endl;
                    cout<<"extract table :: diff "<<element.diff<<endl;
                    cout<<"extract table :: origin "<<element.origin<<endl;
                }

                return table;
            }

            vector<int> RevisionHandler::extractChildren( vector< int >& origines, int parent ){
                vector<int> children;
                for(int i=0; i<origines.size(); i++)
                    if( parent == origines[i] )
                        children.push_back( i );
                return children;
            }

            void RevisionHandler::buildChildren( vector<int>& origines, vector< Revision* > revisions, Revision* current, int alreadyBuilt){
                vector<int> children = extractChildren(origines, current->getN());
                Revision* currentChild;

                for(int i=0; i<children.size(); i++){
                    currentChild = revisions[ children[i] ];
                    currentChild->setParent( current );
                    currentChild->setRoot( current->getRoot() );
                    if( currentChild->getN() > 0 )
                        currentChild->setPrevious( revisions[ currentChild->getN()-1 ] );
                    else
                        currentChild->setPrevious( current->getRoot() );

                    if( currentChild->getN() < children.size()-1 ){
                        currentChild->setNext( revisions[ currentChild->getN()+1 ] );
                        currentChild->setLast(  revisions[ children.size()-1 ] );
                    }

                    current->addChild( currentChild );
                    current->setNext( currentChild );
                    if( alreadyBuilt < revisions.size()-1 )
                        buildChildren( origines, revisions,  currentChild, alreadyBuilt + 1 );
                }
            }

            Revision* RevisionHandler::buildStructure( vector<TableElement>& table ){
                if( table.size() == 0 )
                    return NULL;

                vector< int > origines;
                vector< Revision* > revisions; // pair< begin, size >

                for( uint16_t i=0 ; i<table.size() ; i++){
                    origines.push_back( table[i].origin );
                    revisions.push_back( new Revision(revisions.size(), table[i].idBeginning, table[i].size, table[i].diff) );
                }

                Revision* root=new Revision();
                root->setRoot( root );
                buildChildren( origines, revisions, root);
                return root;
            }

            Mutation RevisionHandler::readMutation( ifstream& stream ){
                uint8_t type(0);
                uint64_t idBegining(0);
                uint64_t size(0);

                stream.read( (char *)&type, 1);
                stream.read( (char *)&idBegining, 8);
                stream.read( (char *)&size, 8);

                Mutation m( type, idBegining, size);
                return m;
            }

            void RevisionHandler::applyMutations( vector<char>& data, Revision* rev){
                if( rev == NULL || rev->getRoot() == rev )
                    return;
                ifstream* stream = (rev->getIStream());



                Mutation m;
                uint64_t i =0;
                stream->seekg( rev->getIdBeginning() - rev->getRelativeO(), stream->beg );

                while( i < rev->getSize() ){
                    Mutation m=readMutation( *stream );
                    m.apply(data, *stream);
                    i+=m.getSize() + 17; //17 Bytes => header of a mutation
                }
            }

            void RevisionHandler::writeTable( vector<TableElement>& table, ofstream* stream){
                stream->seekp( 0, stream->end );
                for( uint64_t i=0; i<table.size(); i++){
                    cout<<"id "<<table[i].idBeginning<<endl;
                    cout<<"size "<<table[i].size<<endl;
                    cout<<"diff "<<table[i].diff<<endl;
                    cout<<"origin "<<table[i].origin<<endl;
                    stream->write( (char*)&table[i].idBeginning, 8);
                    stream->write( (char*)&table[i].size, 8);
                    stream->write( (char*)&table[i].diff, 2);
                    stream->write( (char*)&table[i].origin, 2);
                }

                ///Add nbr of table element
                uint16_t s = table.size();
                stream->write( (char*)&s, 2);

                stream->flush();
            }

            void RevisionHandler::write( vector<char>& data, uint64_t pos, uint64_t length, ofstream* stream){
                for(uint64_t j=pos; j<length; j++)
                    (*stream)<<data[j];

                stream->flush();
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

            Revision* RevisionHandler::bestOrigin( Revision* rev,  vector<char>& data ){
                vector< uint64_t > differences = calculDifferences( rev, data);
                uint64_t tmpDiff = 0;
                if( differences.size() > 0)
                    tmpDiff = differences[0];
                else
                    return rev;

                Revision* tmpRev = rev->getRoot();

                for( int i=1; i<differences.size(); i++)
                    if( tmpDiff > differences[i] ){
                        tmpDiff = differences[i];
                        tmpRev    = tmpRev->getNext();
                    }

                return (tmpRev != NULL) ? tmpRev : rev;
            }

            void RevisionHandler::createMutations( vector<char>& origine, vector<char>& data, ofstream* stream, uint64_t pos){
                stream->seekp(pos, stream->end);

                uint8_t type = 0;
                uint64_t idBeginning = 0;
                uint64_t size = 0;

                //Update first
                uint64_t i(0);
                uint64_t n = min( origine.size(), data.size() );

                uint64_t length(0);
                while( i<n ){
                    if( origine[i] !=  data[i] ){
                        length++;
                    }else if(length != 0){
                        type = Mutation::UPDATE;
                        idBeginning = i-length;
                        size = length;

                        stream->write( (char*)&type,1);
                        stream->write( (char*)&idBeginning, 8);
                        stream->write( (char*)&size, 8);
                        write(data, i-length, length, stream);
                        length=0;
                    }
                    i++;
                }

                //Delete
                if( origine.size()>data.size() ){
                    type = Mutation::DELETE;
                    idBeginning = data.size();
                    size = origine.size() - data.size();

                    stream->write( (char*)&type,1);
                    stream->write( (char*)&idBeginning, 8);
                    stream->write( (char*)&size, 8);
                }

                //Insert
                if( origine.size()<data.size() ){
                    type = Mutation::INSERT ;
                    idBeginning = origine.size();
                    size = data.size() - origine.size();

                    stream->write( (char*)&type,1);
                    stream->write( (char*)&idBeginning, 8);
                    stream->write( (char*)&size, 8);
                    write(data, origine.size(), data.size()-origine.size(), stream);
                }
                stream->flush();
            }

            Revision* RevisionHandler::newRevision( Revision* rev,  vector<char>&newData){
                if( rev != NULL && rev->getLast() != NULL )
                    rev = rev->getLast();

                Revision* origin = bestOrigin( rev, newData );

                uint32_t tableSize = extractSizeTable( rev->getIStream() );
                vector<TableElement> table = extractTable( rev->getIStream() );

                ///Building of origin
                vector<char> tmpData;
                vector< Revision* > parents = origin->getParents();
                if( parents.size() > 0){
                    for( int i=0; i<parents.size()-1 ; i++)
                        applyMutations( tmpData, parents[i]);

                    applyMutations( tmpData, origin); //Data is now hydrate
                }

                ///Rev creation, size will be hydrate later
                Revision* newRev= new Revision( rev->getN()+1, rev->getIdBeginning()+rev->getSize(), 0, diff( tmpData, newData) );
                rev->addChild( newRev );
                newRev->setPrevious( rev );
                newRev->setParent( origin );
                newRev->setRoot( rev->getRoot() );
                newRev->setIStream( rev->getIStreamLocation() );
                newRev->setOStream( rev->getOStreamLocation() );
                newRev->setLast( newRev );

                createMutations( tmpData, newData, newRev->getOStream(), tableSize);

                ///Size
                ofstream* oStream = rev->getOStream();
                oStream->seekp (0, oStream->end);
                uint64_t length = oStream->tellp();
                newRev->setSize( length - rev->getIdBeginning()-rev->getSize() );

                ///Maj of the table
                TableElement newElement;
                newElement.idBeginning = newRev->getIdBeginning();
                newElement.size = newRev->getSize();
                newElement.diff = newRev->getDiff();
                newElement.origin = ( origin != NULL ) ? origin->getN() : 0;

                table.push_back( newElement );

                writeTable( table, oStream );

                return newRev;
            }

    }
}
