#include "Revision.h"

namespace Athena{
    namespace Mnemosyne{

            Revision::~Revision(){
                for(int i=0; i<children.size(); i++)
                    delete children[i];
                if( iStream != NULL )
                    delete iStream;
                if( oStream != NULL )
                    delete oStream;
            }

            list< Revision* > Revision::getParents(){
                if( n != -1 ){
                    list< Revision* > parents = parent->getParents();
                    parents.push_back( this );
                }else
                    return list< Revision* >();
            }

            void Revision::getAllDiffs( vector< uint32_t >& vect ){
                if( next != NULL )
                    vect.push_back( diff );

                next->getAllDiffs( vect );
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

    }
}
