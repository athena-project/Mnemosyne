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
#include <boost/python.hpp>
#include "Ressource.h"

using namespace std;
using namespace boost::python;
using namespace Athena::Mnemosyne;

BOOST_PYTHON_MODULE(PyRessource){
    class_<Ressource>("Ressource", init<>())
        .def("getId", &Ressource::getId)
        .def("getCurrentRevision", &Ressource::getCurrentRevision)
        .def("getChunkIds", &Ressource::getChunkIds)
        .def("setId", &Ressource::setId)
        .def("setCurrentRevision", &Ressource::setCurrentRevision)
        .def("getChunkIds", &Ressource::getChunkIds)
    ;

    class_<RessourceHandler>("RessourceHandler", init<>())
        .def("buildRevision", &RessourceHandler::buildRevision)
        .def("newRevision", &RessourceHandler::newRevision)
    ;
}
