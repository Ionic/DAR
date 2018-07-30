/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: tronc.cpp,v 1.14 2002/06/12 18:38:00 denis Rel $
//
/*********************************************************************/

#pragma implementation

#include "tronc.hpp"
#include <errno.h>
#include <string.h>

tronc::tronc(generic_file *f, const infinint & offset, const infinint &size) : generic_file(f->get_mode())
{
    ref = f;
    sz = size;
    start = offset;
    current = size; // forces skipping the first time
}

tronc::tronc(generic_file *f, const infinint & offset, const infinint &size, gf_mode mode) : generic_file(mode)
{
    ref = f;
    sz = size;
    start = offset;
    current = size; // forces skipping the firt time
}

bool tronc::skip(infinint pos)
{
    if(current == pos)
	return true;

    if(pos > sz)
    {
	current = sz;
	ref->skip(start + sz);
	return false;
    }
    else
    {
	current = pos;
	return ref->skip(start + pos);
    }
}

bool tronc::skip_to_eof()
{
    current = sz;
    return ref->skip(start + sz);
}

bool tronc::skip_relative(signed int x)
{
    if(x < 0)
    {
	if(current < -x)
	{
	    ref->skip(start);
	    current = 0;
	    return false;
	}
	else
	{
	    bool r = ref->skip_relative(x);
	    if(r)
		current -= -x;
	    else
	    {
		ref->skip(start);
		current = 0;
	    }
	    return r;
	}
    }

    if(x > 0)
    {
	if(current + x >= sz)
	{
	    current = sz;
	    ref->skip(start+sz);
	    return false;
	}
	else
	{
	    bool r = ref->skip_relative(x);
	    if(r)
		current += x;
	    else
	    {
		ref->skip(start+sz);
		current = sz;
	    }
	    return r;
	}
    }

    return true;
}


static void dummy_call(char *x)
{
    static char id[]="$Id: tronc.cpp,v 1.14 2002/06/12 18:38:00 denis Rel $";
    dummy_call(id);
}

int tronc::inherited_read(char *a, size_t size)
{
    infinint avail = sz - current;
    unsigned long int macro_pas = 0, micro_pas;
    unsigned long lu = 0;
    int ret;

    do
    {
	avail.unstack(macro_pas);
	micro_pas = size - lu > macro_pas ? macro_pas : size - lu;
	if(micro_pas > 0)
	{
	    ret = ref->read(a+lu, micro_pas);
	    if(ret > 0)
	    {
		lu += ret;
		macro_pas -= ret;
	    }
	    if(ret < 0)
		throw Erange("tronc::inherited_read", strerror(errno));
	}
	else
	    ret = 0;
    }
    while(ret > 0);
    current += lu;

    return lu;
}

int tronc::inherited_write(const char *a, size_t size)
{
    infinint avail = sz - current;
    unsigned long int macro_pas = 0, micro_pas;
    unsigned long int wrote = 0;
    int ret;

    ref->skip(start + current);
    do
    {
	avail.unstack(macro_pas);
	if(macro_pas == 0 && wrote < size)
	    throw Erange("tronc::inherited_write", "tried to write out of size limited file");
	micro_pas = size - wrote > macro_pas ? macro_pas : size - wrote;
	ret = ref->write(a+wrote, micro_pas);
	if( ret > 0)
	{
	    wrote += ret;
	    macro_pas -= ret;
	}
	if(ret < 0)
	    throw Erange("tronc::inherited_write", strerror(errno));
    }
    while(ret > 0);
    current += wrote;

    return wrote;
}
