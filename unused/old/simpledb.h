/*
Copyright (C) 2010  Peter Mora, Zoltan Papp

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SIMPLEDB_HEADER_INCLUDED
#define SIMPLEDB_HEADER_INCLUDED

#include <map>
#include <string>

typedef std::map<std::string, std::string> DICT;

class SimpleDB
{
	bool is_changed;
	std::map<std::string,DICT> plugins;
	std::map<std::string,DICT> users;
public:
	SimpleDB();
	std::string readplugin(std::string plugin, std::string value, std::string def="");
};


#endif
