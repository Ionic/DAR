/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2052 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

} // end extern "C"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "cygwin_adapt.hpp"
#include "int_tools.hpp"
#include "fichier.hpp"
#include "tools.hpp"

#include <iostream>
#include <sstream>

#define BUFFER_SIZE 102400
#ifdef SSIZE_MAX
#if SSIZE_MAX < BUFFER_SIZE
#undef BUFFER_SIZE
#define BUFFER_SIZE SSIZE_MAX
#endif
#endif

using namespace std;

namespace libdar
{

    void fichier_global::inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;

	while(wrote < size)
	{
	    wrote += fichier_global_inherited_write(a+wrote, size-wrote);
	    if(wrote < size)
	    {
		if(x_dialog == NULL)
		    throw SRC_BUG;
		x_dialog->pause(gettext("No space left on device, you have the opportunity to make room now. When ready : can we continue ?"));
	    }
	}
    }

    U_I fichier_global::inherited_read(char *a, U_I size)
    {
	U_I ret = 0;
	U_I read = 0;
	string message;

	while(!fichier_global_inherited_read(a+ret, size-ret, read, message))
	{
	    ret += read;
	    if(x_dialog == NULL)
		throw SRC_BUG;
	    x_dialog->pause(message);
	}

	ret += read;

	return ret;
    }

    fichier_global::fichier_global(const user_interaction & dialog, gf_mode mode) : generic_file(mode)
    {
	x_dialog = dialog.clone();
	if(x_dialog == NULL)
	    throw SRC_BUG;
    }


    void fichier_global::copy_from(const fichier_global & ref)
    {
	if(ref.x_dialog != NULL)
	{
	    x_dialog = ref.x_dialog->clone();
	    if(x_dialog == NULL)
		throw Ememory("fichier_global::copy_from");
	}
	else
	    x_dialog = NULL;
    }

    void fichier_global::copy_parent_from(const fichier_global & ref)
    {
	generic_file *me_g = this;
	const generic_file *you_g = &ref;
	thread_cancellation *me_t = this;
	const thread_cancellation *you_t = &ref;
	*me_g = *you_g;
	*me_t = *you_t;
    }


} // end of namespace
