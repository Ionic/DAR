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
// $Id: generic_file.cpp,v 1.29 2002/06/20 20:44:14 denis Rel $
//
/*********************************************************************/

#pragma implementation

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "generic_file.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"

void clear(crc & value)
{
    for(int i = 0; i < CRC_SIZE; i++)
	value[i] = '\0';
}

bool same_crc(const crc &a, const crc &b)
{
    int i = 0;
    while(i < CRC_SIZE && a[i] == b[i])
	i++;
    return i == CRC_SIZE;
}

void copy_crc(crc & dst, const crc & src)
{
    for(int i = 0; i < CRC_SIZE; i++)
	dst[i] = src[i];
}

#define BUFFER_SIZE 102400

int generic_file::read(char *a, size_t size)
{
    if(rw == gf_write_only)
	throw Erange("generic_file::read", "reading a write only generic_file");
    else
	return (this->*active_read)(a, size);
}

int generic_file::write(const char *a, size_t size)
{
    if(rw == gf_read_only)
	throw Erange("generic_file::write", "writing to a read only generic_file");
    else
	return (this->*active_write)(a, size);
}

int generic_file::read_back(char &a)
{
    if(skip_relative(-1))
    {
	int ret = read(&a,1);
	skip_relative(-1);
	return ret;
    }
    else
	return 0;
}

void generic_file::copy_to(generic_file & ref)
{
    char buffer[BUFFER_SIZE];
    int lu, ret = 0;

    do
    {
	lu = this->read(buffer, BUFFER_SIZE);
	if(lu > 0)
	    ret = ref.write(buffer, lu);
    }
    while(lu > 0 && ret > 0);
}

void generic_file::copy_to(generic_file & ref, crc & value)
{
    reset_crc();
    copy_to(ref);
    get_crc(value);
}

unsigned long generic_file::copy_to(generic_file & ref, unsigned long int size)
{
    char buffer[BUFFER_SIZE];
    int lu = 1, ret = 1, pas;
    unsigned long wrote = 0;

    while(wrote < size && ret > 0 && lu > 0)
    {
	pas = size > BUFFER_SIZE ? BUFFER_SIZE : size;
	lu = read(buffer, pas);
	if(lu > 0)
	{
	    ret = ref.write(buffer, lu);
	    wrote += lu;
	}
    }

    return wrote;
}

infinint generic_file::copy_to(generic_file & ref, infinint size)
{
    unsigned long int tmp = 0, delta;
    infinint wrote = 0;

    size.unstack(tmp);

    do
    {
	delta = copy_to(ref, tmp);
	wrote += delta;
	tmp -= delta;
	if(tmp == 0)
	    size.unstack(tmp);
    }
    while(tmp > 0);

    return wrote;
}

bool generic_file::diff(generic_file & f)
{
    char buffer1[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];
    int lu1 = 0, lu2 = 0;
    bool diff = false;

    if(get_mode() == gf_write_only || f.get_mode() == gf_write_only)
	throw Erange("generic_file::diff", "cannot compare files in write only mode");
    skip(0);
    f.skip(0);
    do
    {
	lu1 = read(buffer1, BUFFER_SIZE);
	lu2 = f.read(buffer2, BUFFER_SIZE);
	if(lu1 == lu2)
	{
	    register int i = 0;
	    while(i < lu1 && buffer1[i] == buffer2[i])
		i++;
	    if(i < lu1)
		diff = true;
	}
	else
	    diff = true;
    }
    while(!diff && lu1 > 0);

    return diff;
}

void generic_file::reset_crc()
{
    if(active_read == &generic_file::read_crc)
	throw SRC_BUG; // crc still active, previous CRC value never read
    clear(value);
    enable_crc(true);
    crc_offset = 0;
}

void generic_file::enable_crc(bool mode)
{
    if(mode) // routines with crc features
    {
	active_read = &generic_file::read_crc;
	active_write = &generic_file::write_crc;
    }
    else
    {
	active_read = &generic_file::inherited_read;
	active_write = &generic_file::inherited_write;
    }
}

void generic_file::compute_crc(const char *a, int size)
{
    for(register int i = 0; i < size; i++)
	value[(i+crc_offset)%CRC_SIZE] ^= a[i];
    crc_offset = (crc_offset + size) % CRC_SIZE;
}

int generic_file::read_crc(char *a, size_t size)
{
    int ret = inherited_read(a, size);
    compute_crc(a, ret);
    return ret;
}

int generic_file::write_crc(const char *a, size_t size)
{
    int ret = inherited_write(a, size);
    compute_crc(a, ret);
    return ret;
}

fichier::fichier(int fd) : generic_file(generic_file_get_mode(fd))
{
    filedesc = fd;
}

fichier::fichier(char *name, gf_mode m) : generic_file(m)
{
    fichier::open(name, m);
}

fichier::fichier(const path &chemin, gf_mode m) : generic_file(m)
{
    char *name = tools_str2charptr(chemin.display());

    try
    {
	open(name, m);
    }
    catch(Egeneric & e)
    {
	delete name;
	throw;
    }
    delete name;
}

infinint fichier::get_size() const
{
    struct stat dat;
    infinint filesize;

    if(filedesc < 0)
	throw SRC_BUG;

    if(fstat(filedesc, &dat) < 0)
	throw Erange("fichier::get_size()", string("error getting size of file : ") + strerror(errno));
    else
	filesize = dat.st_size;

    return filesize;
}

bool fichier::skip(infinint pos)
{
    off_t delta;
    if(lseek(filedesc, 0, SEEK_SET) < 0)
	return false;

    do {
	delta = 0;
	pos.unstack(delta);
	if(delta > 0)
	    if(lseek(filedesc, delta, SEEK_CUR) < 0)
		return false;
    } while(delta > 0);

    return true;
}

bool fichier::skip_to_eof()
{
    return lseek(filedesc, 0, SEEK_END) >= 0;
}

bool fichier::skip_relative(signed int x)
{
    if(x > 0)
    {
	if(lseek(filedesc, x, SEEK_CUR) < 0)
	    return false;
	else
	    return true;
    }

    if(x < 0)
    {
	bool ret = true;
	off_t actu = lseek(filedesc, 0, SEEK_CUR);

	if(actu < -x)
	{
	    actu = 0;
	    ret = false;
	}
	else
	    actu += x; // x is negative
	if(lseek(filedesc, actu, SEEK_SET) < 0)
	    ret = false;

	return ret;
    }

    return true;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: generic_file.cpp,v 1.29 2002/06/20 20:44:14 denis Rel $";
    dummy_call(id);
}

infinint fichier::get_position()
{
    off_t ret = lseek(filedesc, 0, SEEK_CUR);

    if(ret == -1)
	throw Erange("fichier::get_position", string("error getting file position : ") + strerror(errno));

    return ret;
}

int fichier::inherited_read(char *a, size_t size)
{
    int ret;
    unsigned int lu = 0;

    do
    {
	ret = ::read(filedesc, a+lu, size-lu);
	if(ret < 0)
	{
	    switch(errno)
	    {
	    case EINTR:
		break;
	    case EAGAIN:
		throw SRC_BUG; // non blocking read not compatible with
		    // generic_file
	    case EIO:
		throw Ehardware("fichier::inherited_read", "");
	    default :
		throw Erange("fichier::inherited_read", string("error while reading from file: ") + strerror(errno));
	    }
	}
	else
	    lu += ret;
    }
    while(lu < size && ret != 0);

    return lu;
}

int fichier::inherited_write(const char *a, size_t size)
{
    int ret;
    size_t total = 0;
    while(total < size)
    {
	ret = ::write(filedesc, a+total, size-total);
	if(ret < 0)
	{
	    switch(errno)
	    {
	    case EINTR:
		break;
	    case EIO:
		throw Ehardware("fichier::inherited_write", strerror(errno));
	    case ENOSPC:
		user_interaction_pause("no space left on device, you have the oportunity to make room now. When ready : can we continue ?");
		break;
	    default :
		throw Erange("fichier::inherited_write", string("error while writing to file: ") + strerror(errno));
	    }
	}
	else
	    total += ret;
    }

    return total;
}

void fichier::open(const char *name, gf_mode m)
{
    int mode;
    int perm = 0777;

    switch(m)
    {
    case gf_read_only :
	mode = O_RDONLY;
	break;
    case gf_write_only :
	mode = O_WRONLY|O_CREAT;
	break;
    case gf_read_write :
	mode = O_RDWR|O_CREAT;
	break;
    default:
	throw SRC_BUG;
    }

    do
    {
	filedesc = ::open(name, mode, perm);
	if(filedesc < 0)
	{
	    if(filedesc == ENOSPC)
		user_interaction_pause("no space left for inode, you have the oportunity to make some room now. When done : can we continue ?");
	    else
		throw Erange("fichier::open", string("can't open file : ") + strerror(errno));
	}
    }
    while(filedesc == ENOSPC);
}

gf_mode generic_file_get_mode(int fd)
{
    int flags = fcntl(fd, F_GETFL) & O_ACCMODE;
    gf_mode ret;

    switch(flags)
    {
    case O_RDONLY:
	ret = gf_read_only;
	break;
    case O_WRONLY:
	ret = gf_write_only;
	break;
    case O_RDWR:
	ret = gf_read_write;
	break;
    default:
	throw Erange("generic_file_get_mode", "file mode is neither read nor write");
    }

    return ret;
}

const string generic_file_get_name(gf_mode mode)
{
    string ret;

    switch(mode)
    {
    case gf_read_only:
	ret = "read only";
	break;
    case gf_write_only:
	ret = "write only";
	break;
    case gf_read_write:
	ret = "read and write";
	break;
    default:
	throw SRC_BUG;
    }

    return ret;
}