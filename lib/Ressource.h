/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA 02110-1301, USA.
*
* @autor Severus21
*/

#ifndef RESSOURCE_H_INCLUDED
#define RESSOURCE_H_INCLUDED


#include <string>
#include <vector>
#include <stdint.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <ctime>

#include "Revision.h"
#include "Chunk.h"


using namespace std;



class Ressource{
	protected :
		uint64_t id;                ///SQL id
		uint32_t currentRevision;   ///id, 0 => no revision yet
		vector<uint64_t> chunkIds;  ///ids of chunks

		vector<Chunk> chunks;
		string content;             /// current data if needed
		Revision* rev;              /// current revision if needed
	public :

		static string DIR(){ return "/home/toor/Desktop/ressources"; }
		static string TMP_DIR(){ return "/home/toor/Desktop/ressources/tmp"; }

		Ressource();
		virtual ~Ressource();

		uint64_t getId(){ return id; }
		uint32_t getCurrentRevision(){ return currentRevision; }
		vector<uint64_t> getChunkIds(){ return chunkIds; }
		vector<Chunk> getChunks(){ return chunks; }
		string getContent(){ return content; }
		Revision* getRevision(){ return rev; }

		void setId(int param){ id = param; }
		void setCurrentRevision( uint32_t cR ){ currentRevision = cR; }
		void setContent(string param){ content = param; }
		void setChunkIds( vector<uint64_t> ids){
			chunkIds=ids;

			ChunkManager chManager;
			chunks = chManager.get( chunkIds );
		}
		void setRevision(Revision* newRev){
			delete rev;
			rev = newRev;
		}

		/**
		 * @brief if content exists or not
		 */
		bool empty();
};

class RessourceHandler{
	protected :

	public :

		/**
		 * @brief build the revisions' table of the current ressource,
		 * only loading the needed chunk.
		 * @param r             - ressource
		 * @return all the table
		 */
		vector< TableElement> x_buildTable( Ressource& r );

		/**
		 * @brief Build the content of the n revision of the ressource
		 * @param r             - ressource
		 * @param n             - number of the revision wanted
		 * @return content
		 */
		string buildRevision( Ressource& r, uint32_t n );

		/**
		 * @brief Hydrate the content of the revision tree
		 * @param r             - ressource
		 * @return new revision tree
		 */
		Revision* buildAllRevisions(Ressource& r);

		/**
		 * @brief Create a  new revision of the ressource from dataStr
		 * @param rev           - ressource
		 * @param dataStr       - new data
		 */
		void newRevision( Ressource* rev, string dataStr );
};


#endif // RESSOURCE_H_INCLUDED
