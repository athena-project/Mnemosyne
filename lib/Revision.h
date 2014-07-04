#ifndef REVISION_CPP_INCLUDED
#define REVISION_CPP_INCLUDED

#include <vector>
#include <list>
#include <string>
#include <fstream>
#include <stdint.h>


#include <algorithm>

using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class Revision{


            protected :
                int n;                  //N° de la révision, -1 si représente la racine
                uint64_t idBeginning;   //Id du premier caractère de la révision
                uint64_t size;          //Taille de la révision
                uint32_t diff;          //E[%*10000] de différence avec l'origine(abstract)


                Revision* parent;
                vector< Revision* > children;
                Revision* previous;     //révision n-1
                Revision* next;         //révision n+1
                Revision* last;         //Dernière révision ajoutée : nmax
                Revision* root;

                ifstream* iStream;       //mutations's instructions
                ofstream* oStream;
                uint64_t relativeO = 0;     //Origine relative du flux ie debut de la rev

            public :
                static const uint32_t REVISION_SIZE_TABLE = 20; // octets , origine(uint16_t) idBeginning(uint64_t) size(uint64_t) diff(uint16_t)

                Revision(){ root=this; n=-1;}
                Revision(int num) : n(num){ root = (n == -1 ) ? this : NULL; }
                Revision(int num, uint64_t id, uint64_t s, uint32_t d) : n(num), idBeginning(id), size(s), diff(d){ root = (n == -1 ) ? this : NULL; }
                ~Revision();

                int getN(){                 return n; }
                uint64_t getIdBeginning(){  return idBeginning; }
                uint64_t getSize(){         return size; }
                uint32_t getDiff(){         return diff; }
                ifstream* getStream(){      return stream; }
                uint64_t getRelativeO(){    return relativeO; }
                Revision* getRoot(){        return root; }
                Revision* getLast(){        return last; }
                Revision* getNext(){        return next; }


                void setParent( Revision* rev ){    parent=rev; }
                void setPrevious( Revision* rev ){  previous=rev; }
                void setNext( Revision* rev ){      next=rev; }
                void setLast( Revision* rev ){      last=rev; }
                void setStream( ifstream* s ){      stream=s; }
                void setRelativeO( uint64_t n ){    relativeO=n; }
                void setRoot( Revision* r ){        root = r; }

                void addChild( Revision* child ){ child->setRoot( root ); children.push_back(child); }

                /**
                 *  @return parents of the current revision order by n asc ie root first
                **/
                list< Revision* > getParents(){
                    if( n != -1 ){
                        list< Revision* > parents = parent->getParents();
                        parents.push_back( this );
                    }else
                        return list< Revision* >();
                }

                vector< uint32_t > getAllDiffs( vector< uint32_t >& vect ){
                    if( n != -1 )
                        vect.push_back( diff );
                    next->getAllDiffs( vect );
                    return vect;
                }

                /**
                 * @return (minDiff,maxDiff)
                **/
                bool compDiff( uint32_t d1, uint32_t d2){
                    return d1>d2;
                }

                pair<uint32_t, uint32_t> getExtremaDiff(){
                    vector< uint32_t > vect;
                    last->getAllDiffs( vect );
                    if(vect.size() == 0)
                        return pair<uint32_t, uint32_t>(0,0);

                    uint32_t maxDiff = vect[0];
                    uint32_t minDiff = vect[0];
                    for(int i=1; i<vect.size(); i++){
                        maxDiff = ( maxDiff<vect[i] ) ? vect[i] : maxDiff;
                        minDiff = ( minDiff>vect[i] ) ? vect[i] : minDiff;
                    }
                    return pair<uint32_t, uint32_t>( minDiff, maxDiff);
                }


                /*
                 * Build the current revision(data) in the argument, if we keep the result in RAM
                 * @param data     -
                 */
                void hydrate( vector<bool>& data );

        };

    }
}

#endif // REVISION_CPP_INCLUDED
