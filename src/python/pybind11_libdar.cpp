/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/


#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

// this should be passed from Makefile
#define LIBDAR_MODE 64
#define HAVE_CONFIG_H 1
//

#include "../my_config.h"
#include "../libdar/libdar.hpp"
#include "../libdar/tools.hpp"
#include "../libdar/cat_nomme.hpp"

PYBIND11_MODULE(libdar, mod)
{
	// mod is of type pybind11::module

    mod.doc() = "libdar python binding";


	///////////////////////////////////////////
	// exceptions classes (from erreurs.hpp)
	//

    class pyEgeneric_pub : public libdar::Egeneric
    {
    public:
	using libdar::Egeneric::Egeneric;

	using libdar::Egeneric::exceptionID;
    };

    class pyEgeneric : public pyEgeneric_pub
    {
    public:
	using pyEgeneric_pub::pyEgeneric_pub;

	virtual std::string exceptionID() const override
	{
	    PYBIND11_OVERLOAD_PURE(
		std::string,       // return type
		pyEgeneric_pub,    // parent class
		exceptionID,       // name of the method in C++ (must match Python name)
		);                 // trailing comma needed for portability
	};
    };

    pybind11::class_<libdar::Egeneric, pyEgeneric> egeneric(mod, "Egeneric");
    egeneric
	.def(pybind11::init<const std::string &, const std::string &>())
	.def("get_message", &pyEgeneric_pub::get_message)
	.def("dump_str", &pyEgeneric_pub::dump_str)
	.def("exceptionID", &pyEgeneric_pub::exceptionID);

    pybind11::class_<libdar::Ememory>(mod, "Ememory", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Esecu_memory, libdar::Ememory>(mod, "Esecu_memory")
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Ebug>(mod, "Ebug", egeneric)
	.def(pybind11::init<const std::string &, libdar::S_I>());

    pybind11::class_<libdar::Einfinint>(mod, "Einfinint", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Elimitint>(mod, "Elimitint", egeneric)
	.def(pybind11::init<>());

    pybind11::class_<libdar::Erange>(mod, "Erange", egeneric)
	.def(pybind11::init<const std::string, const std::string>());

    pybind11::class_<libdar::Edeci>(mod, "Edeci", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Efeature>(mod, "Efeature", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Ehardware>(mod, "Ehardware", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Euser_abort>(mod, "Euser_abort", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Edata>(mod, "Edata", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Escript>(mod, "Escript", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Elibcall>(mod, "Elibcall", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Ecompilation>(mod, "Ecompilation", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Ethread_cancel>(mod, "Ethread_cancel", egeneric)
	.def(pybind11::init<bool, libdar::U_64>());

    pybind11::class_<libdar::Esystem> esystem(mod, "Esystem", egeneric);

    pybind11::enum_<libdar::Esystem::io_error>(esystem, "io_error")
	.value("io_exist", libdar::Esystem::io_exist)
	.value("io_absent", libdar::Esystem::io_absent)
	.value("io_access", libdar::Esystem::io_access)
	.value("io_ro_fs", libdar::Esystem::io_ro_fs)
	.export_values();

    esystem.def(pybind11::init<const std::string &, const std::string &, libdar::Esystem::io_error>())
	.def("get_code", &libdar::Esystem::get_code);


    pybind11::class_<libdar::Enet_auth>(mod, "Enet_auth", egeneric)
	.def(pybind11::init<const std::string &>());


	///////////////////////////////////////////
	// class path (from path.hpp)
	//

    pybind11::class_<libdar::path>(mod, "path")
	.def(pybind11::init<const std::string &, bool>(), pybind11::arg("s"), pybind11::arg("undisclosed") = false)
	.def("basename", &libdar::path::basename)
	.def("reset_read", &libdar::path::reset_read)
	.def("read_subdir", &libdar::path::read_subdir)
	.def("is_relative", &libdar::path::is_relative)
	.def("is_absolute", &libdar::path::is_absolute)
	.def("append", &libdar::path::append)
	.def(pybind11::self + pybind11::self) // operator +
	.def(pybind11::self += pybind11::self) // operator += libdar::path
	.def(pybind11::self += std::string()) // operator += std::string
	.def("display", &libdar::path::display)
	.def("display_without_root", &libdar::path::display_without_root);


   	///////////////////////////////////////////
	// infinint (from real_infinint.hpp / limitint.hpp)
	//


#ifndef LIBDAR_MODE

    pybind11::class_<libdar::real_infinint>(mod, "infininit")
	.def(pybind11::init<off_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<time_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<size_t>(), pybind11::arg("a") = 0)
	.def("dump", &libdar::real_infinint::dump)
	.def("read", &libdar::real_infinint::read)
	.def("is_zero", &libdar::real_infinint::is_zero);

#else

    pybind11::class_<libdar::limitint<libdar::INFININT_BASE_TYPE> >(mod, "infinint")
	.def(pybind11::init<off_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<time_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<size_t>(), pybind11::arg("a") = 0)
	.def("dump", &libdar::limitint<libdar::INFININT_BASE_TYPE>::dump)
	.def("read", &libdar::limitint<libdar::INFININT_BASE_TYPE>::read)
	.def("is_zero", &libdar::limitint<libdar::INFININT_BASE_TYPE>::is_zero);

#endif

	///////////////////////////////////////////
	// class deci (from deci.hpp / limitint.hpp)
	//

    pybind11::class_<libdar::deci>(mod, "deci")
	.def(pybind11::init<std::string>())
	.def(pybind11::init<libdar::infinint &>())
	.def("computer", &libdar::deci::computer)
	.def("human", &libdar::deci::human);

	///////////////////////////////////////////
	// tools routines related to infinint
	//



    mod.def("tools_display_interger_in_metric_system", &libdar::tools_display_integer_in_metric_system,
	    pybind11::arg("number"),
	    pybind11::arg("unit"),
	    pybind11::arg("binary Ki (else K)"));

    mod.def("tools_get_extended_size", &libdar::tools_get_extended_size,
	    pybind11::arg("input numerical string with unit suffix"),
	    pybind11::arg("unit base: 1000 for SI, 1024 for computer science"));

    	///////////////////////////////////////////
	// class statistics
	//


    pybind11::class_<libdar::statistics>(mod, "statistics")
	.def(pybind11::init<bool>(), pybind11::arg("lock") = true)
	.def("clear", &libdar::statistics::clear)
	.def("total", &libdar::statistics::total)
	.def("get_treated", &libdar::statistics::get_treated)
	.def("get_hard_links", &libdar::statistics::get_hard_links)
	.def("get_skipped", &libdar::statistics::get_skipped)
	.def("get_inode_only", &libdar::statistics::get_inode_only)
	.def("get_ignored", &libdar::statistics::get_ignored)
	.def("get_tooold", &libdar::statistics::get_tooold)
	.def("get_errored", &libdar::statistics::get_errored)
	.def("get_deleted", &libdar::statistics::get_deleted)
	.def("get_ea_treated", &libdar::statistics::get_ea_treated)
	.def("get_byte_amount", &libdar::statistics::get_byte_amount)
	.def("get_fsa_treated", &libdar::statistics::get_fsa_treated)
	    //
	.def("get_treated_str", &libdar::statistics::get_treated_str)
	.def("get_hard_links_str", &libdar::statistics::get_hard_links_str)
	.def("get_skipped_str", &libdar::statistics::get_skipped_str)
	.def("get_inode_only_str", &libdar::statistics::get_inode_only_str)
	.def("get_ignored_str", &libdar::statistics::get_ignored_str)
	.def("get_tooold_str", &libdar::statistics::get_tooold_str)
	.def("get_errored_str", &libdar::statistics::get_errored_str)
	.def("get_deleted_str", &libdar::statistics::get_deleted_str)
	.def("get_ea_treated_str", &libdar::statistics::get_ea_treated_str)
	.def("get_byte_amount_str", &libdar::statistics::get_byte_amount_str)
	.def("get_fsa_treated_str", &libdar::statistics::get_fsa_treated_str);


    	///////////////////////////////////////////
	// data structures in crypto.hpp
	//

    pybind11::enum_<libdar::crypto_algo>(mod, "crypto_algo")
	.value("none", libdar::crypto_algo::none)
	.value("scrambling", libdar::crypto_algo::scrambling)
	.value("blowfish", libdar::crypto_algo::blowfish)
	.value("aes256", libdar::crypto_algo::aes256)
	.value("twofish256", libdar::crypto_algo::twofish256)
	.value("serpent256", libdar::crypto_algo::serpent256)
	.value("camellia256", libdar::crypto_algo::camellia256)
	.export_values();


    pybind11::class_<libdar::signator> pysignator(mod, "signator");

    pybind11::enum_<libdar::signator::result_t>(pysignator, "result_t")
	.value("good", libdar::signator::good)
	.value("bad", libdar::signator::bad)
	.value("unknown_key", libdar::signator::unknown_key)
	.value("error", libdar::signator::error)
	.export_values();

    pybind11::enum_<libdar::signator::key_validity_t>(pysignator, "key_validity_t")
	.value("valid", libdar::signator::valid)
	.value("expired", libdar::signator::expired)
	.value("revoked", libdar::signator::revoked)
	.export_values();

    pysignator
	.def_readonly("result", &libdar::signator::result)
	.def_readonly("key_validity", &libdar::signator::key_validity)
	.def_readonly("fingerprint", &libdar::signator::fingerprint)
	.def_readonly("signing_date", &libdar::signator::signing_date)
	.def_readonly("signature_expiration_date", &libdar::signator::signature_expiration_date)
	.def(pybind11::self  < pybind11::self)
	.def(pybind11::self == pybind11::self);

    mod.def("crypto_algo_2_string", &libdar::crypto_algo_2_string, pybind11::arg("algo"));
    mod.def("same_signatories", &libdar::same_signatories);


	///////////////////////////////////////////
	// masks_* classes
	//

        // trampoline class needed to address pure virtual methods
	// making a template of the trampoline to be able to derive
	// inherited class from libdar::mask in python environment

    class pymask : public libdar::mask
    {
    public:
	    // using the same constructors
	using libdar::mask::mask;

	virtual bool is_covered(const std::string & expression) const override
	{
	    PYBIND11_OVERLOAD_PURE(
		bool,
		libdar::mask,
		is_covered,
		expression);
	};

	virtual bool is_covered(const libdar::path & chemin) const override
	{
	    PYBIND11_OVERLOAD(
		bool,
		libdar::mask,
		is_covered,
		chemin);
	};

	virtual std::string dump(const std::string & prefix) const override
	{
	    PYBIND11_OVERLOAD_PURE(
		std::string,
		libdar::mask,
		dump,
		prefix);
	};

	virtual libdar::mask *clone() const override
	{
	    PYBIND11_OVERLOAD_PURE(
		libdar::mask *,
		libdar::mask,
		clone, // trailing comma needed for portability
		);
	};
    };


	// the trampoline for libdar::mask
    pybind11::class_<libdar::mask, pymask>(mod, "mask")
	.def(pybind11::init<>())
	.def("is_covered", (bool (libdar::mask::*)(const std::string &) const) &libdar::mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::mask::*)(const libdar::path &) const) &libdar::mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::mask::dump)
	.def("clone", &libdar::mask::clone);

	// inhertied class from libdar::mask will not have their virtual method overridable in python
	// only class libdar::mask can be that way inherited.
	// doing that way is simple. It does not worth effort to make things more complicated as there is no reason
	// to inherit from existing inherited libdar::mask classes

    pybind11::class_<libdar::bool_mask, libdar::mask>(mod, "bool_mask")
	.def(pybind11::init<bool>())
	.def("is_covered", (bool (libdar::bool_mask::*)(const std::string &) const) &libdar::bool_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::bool_mask::*)(const libdar::path &) const) &libdar::bool_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::bool_mask::dump)
	.def("clone", &libdar::bool_mask::clone);


    pybind11::class_<libdar::simple_mask, libdar::mask>(mod, "simple_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::simple_mask::*)(const std::string &) const) &libdar::simple_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::simple_mask::*)(const libdar::path &) const) &libdar::simple_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::simple_mask::dump)
	.def("clone", &libdar::simple_mask::clone);

    pybind11::class_<libdar::regular_mask, libdar::mask>(mod, "regular_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::regular_mask::*)(const std::string &) const) &libdar::regular_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::regular_mask::*)(const libdar::path &) const) &libdar::regular_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::regular_mask::dump)
	.def("clone", &libdar::regular_mask::clone);

    pybind11::class_<libdar::not_mask, libdar::mask>(mod, "not_mask")
	.def(pybind11::init<const libdar::mask &>())
	.def("is_covered", (bool (libdar::not_mask::*)(const std::string &) const) &libdar::not_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::not_mask::*)(const libdar::path &) const) &libdar::not_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::not_mask::dump)
	.def("clone", &libdar::not_mask::clone);

    pybind11::class_<libdar::et_mask, libdar::mask>(mod, "et_mask")
	.def(pybind11::init<>())
	.def("is_covered", (bool (libdar::et_mask::*)(const std::string &) const) &libdar::et_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::et_mask::*)(const libdar::path &) const) &libdar::et_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::et_mask::dump)
	.def("clone", &libdar::et_mask::clone)
	.def("add_mask", &libdar::et_mask::add_mask)
	.def("size", &libdar::et_mask::size)
	.def("clear", &libdar::et_mask::clear);

    pybind11::class_<libdar::ou_mask, libdar::mask>(mod, "ou_mask")
	.def(pybind11::init<>())
	.def("is_covered", (bool (libdar::ou_mask::*)(const std::string &) const) &libdar::ou_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::ou_mask::*)(const libdar::path &) const) &libdar::ou_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::ou_mask::dump)
	.def("clone", &libdar::ou_mask::clone)
	.def("add_mask", &libdar::ou_mask::add_mask)
	.def("size", &libdar::ou_mask::size)
	.def("clear", &libdar::ou_mask::clear);

    pybind11::class_<libdar::simple_path_mask, libdar::mask>(mod, "simple_path_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::simple_path_mask::*)(const std::string &) const) &libdar::simple_path_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::simple_path_mask::*)(const libdar::path &) const) &libdar::simple_path_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::simple_path_mask::dump)
	.def("clone", &libdar::simple_path_mask::clone);

    pybind11::class_<libdar::same_path_mask, libdar::mask>(mod, "same_path_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::same_path_mask::*)(const std::string &) const) &libdar::same_path_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::same_path_mask::*)(const libdar::path &) const) &libdar::same_path_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::same_path_mask::dump)
	.def("clone", &libdar::same_path_mask::clone);

    pybind11::class_<libdar::exclude_dir_mask, libdar::mask>(mod, "exclude_dir_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::exclude_dir_mask::*)(const std::string &) const) &libdar::exclude_dir_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::exclude_dir_mask::*)(const libdar::path &) const) &libdar::exclude_dir_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::exclude_dir_mask::dump)
	.def("clone", &libdar::exclude_dir_mask::clone);

    	///////////////////////////////////////////
	// mask_list_* class
	//

    pybind11::class_<libdar::mask_list, libdar::mask>(mod, "mask_list")
	.def(pybind11::init<const std::string &, bool, const libdar::path &, bool>())
	.def("is_covered", (bool (libdar::mask_list::*)(const std::string &) const) &libdar::mask_list::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::mask_list::*)(const libdar::path &) const) &libdar::mask_list::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::mask_list::dump)
	.def("clone", &libdar::mask_list::clone)
	.def("size", &libdar::mask_list::size);

    	///////////////////////////////////////////
	// binding for what's criterium.hpp
	//

    class pycriterium : public libdar::criterium
    {
    public:
	virtual bool evaluate(const libdar::cat_nomme & first, const libdar::cat_nomme & second) const override
	{
	    PYBIND11_OVERLOAD_PURE(bool,
				   libdar::criterium,
				   evaluate,
				   first,
				   second);
	}

	virtual criterium *clone() const override
	{
	    PYBIND11_OVERLOAD_PURE(libdar::criterium *,
				   libdar::criterium,
				   clone,); // trailing comma expected as this method has no argument
	};
    };

    pybind11::class_<libdar::criterium, pycriterium>(mod, "criterium")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::criterium::evaluate)
	.def("clone", &libdar::criterium::clone);

	///////////////////////////////////////////
	// binding for what's in crit_action.hpp
	//

    pybind11::enum_<libdar::saved_status>(mod, "saved_status")
	.value("saved", libdar::saved_status::saved)
	.value("inode_only", libdar::saved_status::inode_only)
	.value("fake", libdar::saved_status::fake)
	.value("not_saved", libdar::saved_status::not_saved)
	.value("delta", libdar::saved_status::delta)
	.export_values();

    pybind11::enum_<libdar::over_action_data>(mod, "over_action_data")
	.value("data_preserve", libdar::data_preserve)
	.value("data_overwrite", libdar::data_overwrite)
	.value("data_preserve_mark_already_saved", libdar::data_preserve_mark_already_saved)
	.value("data_overwrite_mark_already_saved", libdar::data_overwrite_mark_already_saved)
	.value("data_remove", libdar::data_remove)
	.value("data_undefined", libdar::data_undefined)
	.value("data_ask", libdar::data_ask)
	.export_values();

    pybind11::enum_<libdar::over_action_ea>(mod, "over_action_ea")
	.value("EA_preserve", libdar::EA_preserve)
	.value("EA_overwrite", libdar::EA_overwrite)
	.value("EA_clear", libdar::EA_clear)
	.value("EA_preserve_mark_already_saved", libdar::EA_preserve_mark_already_saved)
	.value("EA_overwrite_mark_already_saved", libdar::EA_overwrite_mark_already_saved)
	.value("EA_merge_preserve", libdar::EA_merge_preserve)
	.value("EA_merge_overwrite", libdar::EA_merge_overwrite)
	.value("EA_undefined", libdar::EA_undefined)
	.value("EA_ask", libdar::EA_ask)
	.export_values();

    class pycrit_action : public libdar::crit_action
    {
    public:
	virtual void get_action(const libdar::cat_nomme & first, const libdar::cat_nomme & second, libdar::over_action_data & data, libdar::over_action_ea & ea) const override
	{
	    PYBIND11_OVERLOAD_PURE(void,
				   libdar::crit_action,
				   get_action,
				   first,
				   second,
				   data,
				   ea);
	};

	virtual libdar::crit_action *clone() const override
	{
	    PYBIND11_OVERLOAD_PURE(libdar::crit_action *,
				   libdar::crit_action,
				   clone,); // training comma is expected as there is no argument to this method
	};
    };

    pybind11::class_<libdar::crit_action, pycrit_action>(mod, "crit_action")
	.def(pybind11::init<>())
	.def("get_action", &libdar::crit_action::get_action)
	.def("clone", &libdar::crit_action::clone);

    pybind11::class_<libdar::crit_constant_action, libdar::crit_action>(mod, "crit_constant_action")
	.def(pybind11::init<libdar::over_action_data, libdar::over_action_ea>())
	.def("get_action", &libdar::crit_constant_action::get_action)
	.def("clone", &libdar::crit_constant_action::clone);

    pybind11::class_<libdar::testing, libdar::crit_action>(mod, "testing")
	.def(pybind11::init<const libdar::criterium &, const libdar::crit_action &, const libdar::crit_action &>())
	.def("get_action", &libdar::testing::get_action)
	.def("clone", &libdar::testing::clone);

    pybind11::class_<libdar::crit_chain, libdar::crit_action>(mod, "crit_chain")
	.def(pybind11::init<>())
	.def("add", &libdar::crit_chain::add)
	.def("clear", &libdar::crit_chain::clear)
	.def("gobe", &libdar::crit_chain::gobe)
	.def("get_action", &libdar::crit_chain::get_action)
	.def("clone", &libdar::crit_chain::clone);

	///////////////////////////////////////////
	// archive_options_* classes
	//

    pybind11::class_<libdar::archive_options_read>(mod, "archive_options_read")
	.def("clear", &libdar::archive_options_read::clear)
	.def("set_crypto_algo", &libdar::archive_options_read::set_crypto_algo)
	.def("set_crypto_pass", &libdar::archive_options_read::set_crypto_pass)
	.def("set_crypto_size", &libdar::archive_options_read::set_crypto_size)
	.def("set_default_crypto_size", &libdar::archive_options_read::set_default_crypto_size)
	.def("set_input_pipe", &libdar::archive_options_read::set_input_pipe)
	.def("set_output_pipe", &libdar::archive_options_read::set_output_pipe)
	.def("set_execute", &libdar::archive_options_read::set_execute)
	.def("set_info_details", &libdar::archive_options_read::set_info_details)
	.def("set_lax", &libdar::archive_options_read::set_lax)
	.def("set_sequential_read", &libdar::archive_options_read::set_sequential_read)
	.def("set_slice_min_digits", &libdar::archive_options_read::set_slice_min_digits)
	.def("set_entrepot", &libdar::archive_options_read::set_entrepot)
	.def("set_ignore_signature_check_failure", &libdar::archive_options_read::set_ignore_signature_check_failure)
	.def("set_multi_threaded", &libdar::archive_options_read::set_multi_threaded)
	.def("set_external_catalogue", &libdar::archive_options_read::set_external_catalogue)
	.def("unset_external_catalogue", &libdar::archive_options_read::unset_external_catalogue)
	.def("set_ref_crypto_algo", &libdar::archive_options_read::set_ref_crypto_algo)
	.def("set_ref_crypto_pass", &libdar::archive_options_read::set_ref_crypto_pass)
	.def("set_ref_crypto_size", &libdar::archive_options_read::set_ref_crypto_size)
	.def("set_ref_execute", &libdar::archive_options_read::set_ref_execute)
	.def("set_ref_slice_min_digits", &libdar::archive_options_read::set_ref_slice_min_digits)
	.def("set_ref_entrepot", &libdar::archive_options_read::set_ref_entrepot)
	.def("set_header_only", &libdar::archive_options_read::set_header_only);


    pybind11::class_<libdar::archive_options_create>(mod, "archive_options_create")
	.def("clear", &libdar::archive_options_create::clear)
	.def("set_reference", &libdar::archive_options_create::set_reference)
	.def("set_selection", &libdar::archive_options_create::set_selection)
	.def("set_subtree", &libdar::archive_options_create::set_subtree)
	.def("set_allow_over", &libdar::archive_options_create::set_allow_over)
	.def("set_warn_over", &libdar::archive_options_create::set_warn_over)
	.def("set_info_details", &libdar::archive_options_create::set_info_details)
	.def("set_display_treated", &libdar::archive_options_create::set_display_treated)
	.def("set_display_skipped", &libdar::archive_options_create::set_display_skipped)
	.def("set_display_finished", &libdar::archive_options_create::set_display_finished)
	.def("set_pause", &libdar::archive_options_create::set_pause)
	.def("set_empty_dir", &libdar::archive_options_create::set_empty_dir)
	.def("set_compression", &libdar::archive_options_create::set_compression)
	.def("set_compression_level", &libdar::archive_options_create::set_compression_level)
	.def("set_slicing", &libdar::archive_options_create::set_slicing)
	.def("set_ea_mask", &libdar::archive_options_create::set_ea_mask)
	.def("set_execute", &libdar::archive_options_create::set_execute)
	.def("set_crypto_algo", &libdar::archive_options_create::set_crypto_algo)
	.def("set_crypto_pass", &libdar::archive_options_create::set_crypto_pass)
	.def("set_crypto_size", &libdar::archive_options_create::set_crypto_size)
	.def("set_gnupg_recipients", &libdar::archive_options_create::set_gnupg_recipients)
	.def("set_gnupg_signatories", &libdar::archive_options_create::set_gnupg_signatories)
	.def("set_compr_mask", &libdar::archive_options_create::set_compr_mask)
	.def("set_min_compr_size", &libdar::archive_options_create::set_min_compr_size)
	.def("set_nodump", &libdar::archive_options_create::set_nodump)
	.def("set_exclude_by_ea", &libdar::archive_options_create::set_exclude_by_ea)
	.def("set_what_to_check", &libdar::archive_options_create::set_what_to_check)
	.def("set_hourshift", &libdar::archive_options_create::set_hourshift)
	.def("set_empty", &libdar::archive_options_create::set_empty)
	.def("set_alter_atime", &libdar::archive_options_create::set_alter_atime)
	.def("set_furtive_read_mode", &libdar::archive_options_create::set_furtive_read_mode)
	.def("set_same_fs", &libdar::archive_options_create::set_same_fs)
	.def("set_snapshot", &libdar::archive_options_create::set_snapshot)
	.def("set_cache_directory_tagging", &libdar::archive_options_create::set_cache_directory_tagging)
	.def("set_fixed_date", &libdar::archive_options_create::set_fixed_date)
	.def("set_slice_permission", &libdar::archive_options_create::set_slice_permission)
	.def("set_slice_user_ownership", &libdar::archive_options_create::set_slice_user_ownership)
	.def("set_slice_group_ownership", &libdar::archive_options_create::set_slice_group_ownership)
	.def("set_retry_on_change", &libdar::archive_options_create::set_retry_on_change)
	.def("set_sequential_marks", &libdar::archive_options_create::set_sequential_marks)
	.def("set_sparse_file_min_size", &libdar::archive_options_create::set_sparse_file_min_size)
	.def("set_security_check", &libdar::archive_options_create::set_security_check)
	.def("set_user_comment", &libdar::archive_options_create::set_user_comment)
	.def("set_hash_algo", &libdar::archive_options_create::set_hash_algo)
	.def("set_slice_min_digits", &libdar::archive_options_create::set_slice_min_digits)
	.def("set_backup_hook", &libdar::archive_options_create::set_backup_hook)
	.def("set_ignore_unknow_inode_type", &libdar::archive_options_create::set_ignore_unknown_inode_type)
	.def("set_entrepot", &libdar::archive_options_create::set_entrepot)
	.def("set_fsa_scope", &libdar::archive_options_create::set_fsa_scope)
	.def("set_multi_threaded", &libdar::archive_options_create::set_multi_threaded)
	.def("set_delta_diff", &libdar::archive_options_create::set_delta_diff)
	.def("set_delta_signature", &libdar::archive_options_create::set_delta_signature)
	.def("set_delta_mask", &libdar::archive_options_create::set_delta_mask)
	.def("set_delta_sig_min_size", &libdar::archive_options_create::set_delta_sig_min_size)
	.def("set_auto_zeroing_neg_dates", &libdar::archive_options_create::set_auto_zeroing_neg_dates)
	.def("set_ignored_as_symlink", &libdar::archive_options_create::set_ignored_as_symlink)
	.def("set_modified_data_detection", &libdar::archive_options_create::set_modified_data_detection)
	.def("set_iteration_count", &libdar::archive_options_create::set_iteration_count)
	.def("set_kdf_hash", &libdar::archive_options_create::set_kdf_hash)
	.def("set_sig_block_len", &libdar::archive_options_create::set_sig_block_len);

}
