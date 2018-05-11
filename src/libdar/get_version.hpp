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

    /// \file get_version.hpp
    /// \brief routine to initialize libdar and manage its running threads
    /// \ingroup API



#ifndef GET_VERSION_HPP
#define GET_VERSION_HPP

#include "../my_config.h"

extern "C"
{
#if MUTEX_WORKS
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#endif
}

#include <string>
#include "integers.hpp"
#include "thread_cancellation.hpp"

    /// libdar namespace encapsulate all libdar symbols

namespace libdar
{

	/// \addtogroup API
	/// @{

	///  libdar Major version defined at compilation time
    constexpr U_I LIBDAR_COMPILE_TIME_MAJOR = 6;
	///  libdar Medium version defined at compilation time
    constexpr U_I LIBDAR_COMPILE_TIME_MEDIUM = 0;
	///  libdar Minor version defined at compilation time
    constexpr U_I LIBDAR_COMPILE_TIME_MINOR = 0;


	////////////////////////////////////////////////////////////////////////
	// LIBDAR INITIALIZATION METHODS                                      //
	//                                                                    //
	//      A FUNCTION OF THE get_version*() FAMILY *MUST* BE CALLED      //
	//            BEFORE ANY OTHER FUNCTION OF THIS LIBRARY               //
	//                                                                    //
	// CLIENT PROGRAM MUST CHECK THAT THE MAJOR NUMBER RETURNED           //
	// BY THIS CALL IS NOT GREATER THAN THE VERSION USED AT COMPILATION   //
        // TIME. IF SO, THE PROGRAM MUST ABORT AND RETURN A WARNING TO THE    //
	// USER TELLING THE DYNAMICALLY LINKED VERSION IS TOO RECENT AND NOT  //
	// COMPATIBLE WITH THIS SOFTWARE. THE MESSAGE MUST INVITE THE USER    //
	// TO UPGRADE HIS SOFTWARE WITH A MORE RECENT VERSION COMPATIBLE WITH //
	// THIS LIBDAR RELEASE.                                               //
	////////////////////////////////////////////////////////////////////////

	/// return the libdar version, and make libdar initialization (may throw Exceptions)

	/// It is mandatory to call this function (or another one of the get_version* family)
	/// \param[out] major the major number of the version
	/// \param[out] medium the medium number of the version
	/// \param[out] minor the minor number of the version
	/// \param[in] init_libgcrypt whether to initialize libgcrypt if not already done (not used if libcrypt is not linked with libdar)
	/// \note the calling application should check that the major function
	/// is the same as the libdar used at compilation time. See API tutorial for a
	/// sample code.
    extern void get_version(U_I & major, U_I & medium, U_I & minor, bool init_libgcrypt = true);

	/// this method is to be used when you don't want to bother with major, medium and minor
    extern void get_version(bool init_libgcrypt = true);

	///////////////////////////////////////////////
	// CLOSING/CLEANING LIBDAR                   //
	///////////////////////////////////////////////

	// while libdar has only a single boolean as global variable
	// that defines whether the library is initialized or not
	// it must proceed to mutex, and dependent libraries initializations
	// (liblzo, libgcrypt, etc.), which is done during the get_version() call
	// Some library also need to clear some data so the following call
	// is provided in that aim and must be called when libdar will no more
	// be used by the application.

    extern void close_and_clean();

	///////////////////////////////////////////////
	// THREAD CANCELLATION ROUTINES              //
	///////////////////////////////////////////////

#if MUTEX_WORKS
	/// thread cancellation activation

	/// ask that any libdar code running in the thread given as argument be cleanly aborted
	/// when the execution will reach the next libdar checkpoint
	/// \param[in] tid is the Thread ID to cancel libdar in
	/// \param[in] immediate whether to cancel thread immediately or just signal the request to the thread
	/// \param[in] flag an arbitrary value passed as-is through libdar
    inline void cancel_thread(pthread_t tid, bool immediate = true, U_64 flag = 0) { thread_cancellation::cancel(tid, immediate, flag); }

	/// consultation of the cancellation status of a given thread

	/// \param[in] tid is the tid of the thread to get status about
	/// \return false if no cancellation has been requested for the given thread
    inline bool cancel_status(pthread_t tid) { return thread_cancellation::cancel_status(tid); }

	/// thread cancellation deactivation

	/// abort the thread cancellation for the given thread
	/// \return false if no thread cancellation was under process for that thread
	/// or if there is no more pending cancellation (thread has already been canceled).
    inline bool cancel_clear(pthread_t tid) { return thread_cancellation::clear_pending_request(tid); }
#endif


	/// @}

} // end of namespace

#endif
