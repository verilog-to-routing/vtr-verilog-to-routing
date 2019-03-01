/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "Hashtable.hpp"
#include "odin_types.h"
#include "vtr_memory.h"

void Hashtable::destroy_free_items()
{
	for(auto kv: my_map)
		vtr::free(kv.second);
}

void Hashtable::add(std::string key, void *item)
{
	auto v = this->my_map.find(key);
	//what if key already exists ??
	if(v == this->my_map.end())
	{
		this->my_map.insert({key,item});
	}
	else
	{
		this->my_map.insert({key,item});
	}
}

void* Hashtable::remove(std::string key)
{
	void *value = NULL;
	auto v = this->my_map.find(key);
	if(v != this->my_map.end())
	{
		value = this->my_map[key];
		this->my_map.erase(key);
	}
	return value;
}

void* Hashtable::get(std::string key)
{
	void *value = NULL;
	auto v = this->my_map.find(key);
	if(v != this->my_map.end())
		value = this->my_map[key];
	
	return value;
}

bool Hashtable::is_empty ()
{
	return (my_map.size() == 0);
}