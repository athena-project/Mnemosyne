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
 * @author Severus21
 */


#ifndef XZ_H_INCLUDED
#define XZ_H_INCLUDED

#include <iostream>
#include <stdio.h>
#include <lzma.h>


using namespace std;

class Xz{
	protected:
		lzma_stream* ptr;
		lzma_stream stream;

		uint32_t preset = 6;
	public:
		Xz();
		~Xz();
		void init();

		void setPreset( uint32_t p ){ preset = p; }

		bool init_encoder();
		bool compress(const char* inpath, const char* outpath);
		bool compress(FILE* infile, const char* outpath);
		bool compress(FILE* infile, FILE* outfile);

		bool init_decoder();
		bool decompress( const char* inpath, const char* outpath );
		bool decompress( FILE* infile, const char* outpath );
		bool decompress( const char* inpath, FILE* outfile );
		bool decompress( FILE* infile, FILE* outfile );

};

#endif // XZ_H_INCLUDED
