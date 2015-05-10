#ifndef MUTATION_H_INCLUDED
#define MUTATION_H_INCLUDED

#include <stdint.h>
#include <ifstream>
#include <vector>
#include <fstream>

#include "Revision.h"

using namespace std;


class Mutation{
	protected :
		uint8_t type 		= 0;
		uint64_t idBeginning= 0;
		uint64_t size		= 0;

	public :
		static const uint8_t INSERT     = 0b00;
		static const uint8_t DELETE     = 0b01;
		static const uint8_t UPDATE     = 0b10;
		static const uint8_t HEADER_SIZE= 17; ///Bytes

		static const uint64_t BUFFER_LENGTH=64;

		Mutation(){}
		Mutation( uint8_t t, uint64_t id, uint64_t s) : type(t), idBeginning(id), size(s){}

		uint64_t getIdBeginning(){ return idBeginning; }
		uint64_t getSize(){ return size; }

		void setIdBeginning( uint64_t i ){ idBeginning = i; }
		void setSize( uint64_t s ){ size = s; }



		/**
		 *  @brief Return a mutation, the stream cursor is at the begining of the body of the mutation
		 *  @param stream       - location of the mutations' instructions
		 */
		Mutation read( fstream& stream );


		/**
		 * @brief Apply insertion to data in using the stream as a source
		 * @param data          - current data
		 * @param stream        - data use as reference
		 */
		void applyInsert(fstream& data, fstream& stream);

		/**
		 * @brief Delete a part of the vect
		 * @param data          - current data
		 */
		void applyDelete(WorkingRev& data);

		/**
		 * @brief Update the vector in using the stream as a source
		 * @param data          - current data
		 * @param stream        - data use as reference
		 */
		void applyUpdate(WorkingRev& data, fstream& stream);

		/**
		 * @brief Apply the current mutation at the data from the stream
		 * @param data          - current data
		 * @param stream        - data use as reference
		 */
		void apply( WorkingRev& data, fstream& stream);
		
		
		/**
		 * @brief Write the current mutation header in descriptor
		 * @param descriptor        - 
		 */
		void writeHeader( fstream& descriptor);
		
		/**
		 * @brief Write the current mutation  in descriptor
		 * @param workingRev		-
		 * @param descriptor        - 
		 */
		void write( WorkingRev& workingRev, fstream& descriptor);

};

#endif // MUTATION_H_INCLUDED
