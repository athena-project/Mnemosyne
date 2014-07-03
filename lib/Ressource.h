#ifndef RESSOURCE_H_INCLUDED
#define RESSOURCE_H_INCLUDED

#include <vector>
#include <string>
#include <stdint.h>
#include <cmath>
#include <fstream>
#include <sstream>

#include "RevisionHandler.h"
//#include "RessourceHandler.h"
#include "Revision.h"
#include "Chunk.h"


using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class Ressource{
            protected :
                int type;

                uint64_t id;
                string url;
                string domain;
                uint32_t currentRevision; //numero, 0 => no revision yet
               // string time; //last revision
                uint32_t size;
                string contentType; // [nbrRevConcernées:contentType;charset...][nbrRevConcernées:contentType2;charset...]
                vector<uint64_t> chunks; //ids of chunks

                string content; // current data if needed
                Revision* rev;  // current revision if needed
            public :
                enum type{
                    PAGE, CSS, JS, DEFAULT
                };

                static string TMP_DIR(){ return "/home/severus/Desktop"; }
                static int contentTypeToType(string contentType){ return 0; /*à faire*/ }

                Ressource();
                Ressource(string url, string contentType, unsigned int size, string content, unsigned int modified);
                Ressource(unsigned int id, string url, string contentType, unsigned int size, string content, unsigned int modified);
                virtual ~Ressource();

                int getId();
                string getUrl();
                uint32_t getCurrentRevision();
                string getContentType();
                uint32_t getSize();
                string getContent();
                vector<uint64_t> getChunks();
                unsigned int getModified();

                Revision* getRevision(){ return rev; }

                void setId(int param);
                virtual void setUrl(string param);
                void setContentType(string param);
                void setSize(unsigned int param);
                void setContent(string param);
                void setModified(unsigned int param);

                bool empty();

//                string buildRevision( uint32_t n );
//                void newRevision( string dataStr );
            /*


                string extractDomain();
                string extractDir();

                virtual list<string> collectURL();
                virtual void fetchLinks(GumboNode* node, list<string>& links);
                virtual list< string > urlFromLinks(list<string>& links);
            */
        };

        class RessourceHandler{
            protected :

            public :
                string buildRevision( Ressource& r, uint32_t n );
                Revision* buildAllRevisions(Ressource& r);
                void newRevision( Ressource* rev, string dataStr );
        };
    }
}


#endif // RESSOURCE_H_INCLUDED
