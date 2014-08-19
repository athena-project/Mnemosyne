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

#ifndef TAR_H_INCLUDED
#define TAR_H_INCLUDED

#include <libtar.h>
#include <iostream>
#include <fcntl.h> //O_RDONLY
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <cstring>
using namespace std;

namespace Athena{
    namespace Mnemosyne{

        class Tar{
            protected:

            public:
                Tar();


                bool tar_dir( char* location, char* path, char* name );
                bool untar( char* source, char* location);
        };

    }
}


#endif // TAR_H_INCLUDED
