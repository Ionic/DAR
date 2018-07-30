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
// $Id: filtre.cpp,v 1.51 2002/06/26 22:20:20 denis Rel $
//
/*********************************************************************/

#include <map>
#include "user_interaction.hpp"
#include "erreurs.hpp"
#include "filtre.hpp"
#include "filesystem.hpp"
#include "ea.hpp"
#include "defile.hpp"
#include "test_memory.hpp"
#include "null_file.hpp"

static void save_inode(const string &info_quoi, inode * & ino, compressor *stock, bool info_details);
static bool save_ea(const string & info_quoi, inode * & ino, compressor *stock, const inode * ref, bool info_details);

void filtre_restore(const mask &filtre,
		    const mask & subtree,
		    catalogue & cat,
		    bool detruire,
		    const path & fs_racine,
		    bool fs_allow_overwrite,
		    bool fs_warn_overwrite,
		    bool info_details,
		    statistics & st,
		    bool only_if_more_recent,
		    bool restore_ea_root,
		    bool restore_ea_user)
{
    defile juillet = fs_racine;
    const eod tmp_eod;
    const entree *e;
    st.clear();

    filesystem_set_root(fs_racine, fs_allow_overwrite, fs_warn_overwrite, info_details, restore_ea_root, restore_ea_user);
    cat.reset_read();
    filesystem_reset_write();

    while(cat.read(e))
    {
	const nomme *e_nom = dynamic_cast<const nomme *>(e);
	const directory *e_dir = dynamic_cast<const directory *>(e);

	juillet.enfile(e);
	if(e_nom != NULL)
	{
	    try
	    {
		if(subtree.is_covered(juillet.get_string()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
		{
		    const detruit *e_det = dynamic_cast<const detruit *>(e);
		    const inode *e_ino = dynamic_cast<const inode *>(e);
		    const hard_link *e_hard = dynamic_cast<const hard_link *>(e);
		    const etiquette *e_eti = dynamic_cast<const etiquette *>(e);
		    entree *dolly = NULL; // inode of replacement for hard links

		    try
		    {
			if(e_hard != NULL)
			{
			    inode *tmp = NULL;
			    dolly = e_hard->get_inode()->clone();
			    if(dolly == NULL)
				throw Ememory("filtre_restore");
			    tmp = dynamic_cast<inode *>(dolly);
			    if(tmp == NULL)
				throw SRC_BUG; // should be an inode
			    tmp->change_name(e_hard->get_name());
			    e_ino = const_cast<const inode *>(tmp);
			    if(e_ino == NULL)
				throw SRC_BUG; // !?! how is this possible ?
			    st.hard_links++;
			}

			if(e_det != NULL)
			{
			    if(detruire)
			    {
				if(info_details)
				    user_interaction_warning(string("removing file ") + juillet.get_string());
				if(filesystem_write(e))
				    st.deleted++;
			    }
			}
			else
			    if(e_ino != NULL)
			    {
				nomme *exists_nom = filesystem_get_before_write(e_ino);
				inode *exists = dynamic_cast<inode *>(exists_nom);

				if(exists_nom != NULL && exists == NULL)
				    throw SRC_BUG;

				try
				{
					// checking the file contents & inode
				    if(e_ino->get_saved_status() || (e_hard != NULL && filesystem_known_etiquette(e_hard->get_etiquette())))
				    {
					if(!only_if_more_recent || exists == NULL || !e_ino->same_as(*exists) || e_ino->is_more_recent_than(*exists))
					{
					    if(info_details)
						user_interaction_warning(string("restoring file ") + juillet.get_string());
					    if(filesystem_write(e)) // e and not e_ino, it may be a hard link now
						st.treated++;
					}
					else // file is less recent than the one in the filesystem
					{
						// if it is a directory, just recording we go in int now
					    if(e_dir != NULL)
						filesystem_pseudo_write(e_dir);
					    if(e_eti != NULL) // future hard link will get linked against this file
						filesystem_write_hard_linked_target_if_not_set(e_eti, juillet.get_string());
					    st.tooold++;
					}
				    }
				    else // no data saved for this entry (no change since reference backup)
				    {
					    // if it is a directory, just recording we go in int now
					if(e_dir != NULL)
					    filesystem_pseudo_write(e_dir);
					if(e_eti != NULL) // future hard link will get linked against this file
					    filesystem_write_hard_linked_target_if_not_set(e_eti, juillet.get_string());
					st.skipped++;
				    }

				    if(restore_ea_user || restore_ea_root)
				    {
					    // checking the EA list
					    //
					    // need to have EA data to restore and an
					    // existing inode of the same type in
					    // filesystem, to be able to set EA to
					    // an existing inode
					if(((e_ino->ea_get_saved_status() == inode::ea_full &&
					     (exists != NULL && exists->same_as(*e_ino)))
					    || e_ino->get_saved_status()))
					{
					    try
					    {
						if(filesystem_set_ea(e_nom, *(e_ino->get_ea())))
						    st.ea_treated++;
					    }
					    catch(Erange & e)
					    {
						user_interaction_warning(string("Error while restoring EA for ") + juillet.get_string() + ": " + e.get_message());
					    }
					    e_ino->ea_detach(); // in any case we clear memory
					}
				    } // end of EA considerations
				}
				catch(...)
				{
				    if(exists_nom != NULL)
					delete exists_nom;
				    throw;
				}
				if(exists_nom != NULL)
				    delete exists_nom;
			    }
			    else
				throw SRC_BUG; // a nomme is neither a detruit nor an inode !
		    }
		    catch(...)
		    {
			if(dolly != NULL)
			    delete dolly;
			throw;
		    }
		    delete dolly;
		}
		else // inode not covered
		{
		    st.ignored++;
		    if(e_dir != NULL)
		    {
			cat.skip_read_to_parent_dir();
			juillet.enfile(&tmp_eod);
		    }
		}
	    }
	    catch(Ebug & e)
	    {
		throw;
	    }
	    catch(Euser_abort & e)
	    {
		user_interaction_warning(juillet.get_string() + " not restored (user choice)");
		const directory *e_dir = dynamic_cast<const directory *>(e_nom);
		if(e_dir != NULL)
		{
		    cat.skip_read_to_parent_dir();
		    user_interaction_warning("No file in this directory will be restored.");
		}
		st.errored++;
	    }
	    catch(Egeneric & e)
	    {
		user_interaction_warning(string("Error while restoring ") + juillet.get_string() + " : " + e.get_message());
		st.errored++;

		if(e_dir != NULL)
		{
		    user_interaction_warning(string("Warning! No file in that directory will be restored: ") + juillet.get_string());
		    cat.skip_read_to_parent_dir();
		    juillet.enfile(&tmp_eod);
		}
	    }
	}
	else
	    (void)filesystem_write(e); // eod; don't care returned value
    }
    filesystem_freemem();
}

void filtre_sauvegarde(const mask &filtre,
		       const mask &subtree,
		       compressor *stockage,
		       catalogue & cat,
		       catalogue &ref,
		       const path & fs_racine,
		       bool info_details,
		       statistics & st,
		       bool make_empty_dir,
		       bool save_ea_root,
		       bool save_ea_user)
{
    entree *e;
    const entree *f;
    defile juillet = fs_racine;
    const eod tmp_eod;
    st.clear();

    filesystem_set_root(fs_racine, false, false, info_details, save_ea_root,
			save_ea_user);
    cat.reset_add();
    ref.reset_compare();
    filesystem_reset_read();

    while(filesystem_read(e))
    {
	nomme *nom = dynamic_cast<nomme *>(e);
	directory *dir = dynamic_cast<directory *>(e);

	juillet.enfile(e);
	if(nom != NULL)
	{
	    try
	    {
		if(subtree.is_covered(juillet.get_string()) && (dir != NULL || filtre.is_covered(nom->get_name())))
		{
		    hard_link *e_hard = dynamic_cast<hard_link *>(e);

		    if(e_hard != NULL)
		    {
			cat.add(e);
			st.hard_links++;
			st.treated++;
		    }
		    else // "e" is an inode
		    {
			inode *e_ino = dynamic_cast<inode *>(e);
			bool known = ref.compare(e, f);

			try
			{
			    if(known)
			    {
				const inode *f_ino = dynamic_cast<const inode *>(f);

				if(e_ino == NULL || f_ino == NULL)
				    throw SRC_BUG; // filesystem has provided a "nomme" which is not a "inode" thus which is a "detruit"

				if(e_ino->has_changed_since(*f_ino))
				{
				    if(!e_ino->get_saved_status())
					throw SRC_BUG; // filsystem should always provide "saved" "entree"

				    save_inode(juillet.get_string(), e_ino, stockage, info_details);
				    st.treated++;
				}
				else // inode has not changed since last backup
				{
				    e_ino->set_saved_status(false);
				    st.skipped++;
				}

				if(save_ea(juillet.get_string(), e_ino, stockage, f_ino, info_details))
				    st.ea_treated++;
			    }
			    else // inode not present int catalogue of reference
				if(e_ino != NULL)
				{
				    save_inode(juillet.get_string(), e_ino, stockage, info_details);
				    st.treated++;
				    if(save_ea(juillet.get_string(), e_ino, stockage, NULL, info_details))
					st.ea_treated++;
				}
				else
				    throw SRC_BUG;  // filesystem has provided a "nomme" which is not a "inode" thus which is a "detruit"

			    file *tmp = dynamic_cast<file *>(e);
			    if(tmp != NULL)
				tmp->clean_data();

			    cat.add(e);
			}
			catch(...)
			{
			    if(dir != NULL)
				ref.compare(&tmp_eod, f);
			    throw;
			}
		    }
		}
		else // inode not covered
		{
		    nomme *ig = NULL;
		    inode *ignode = NULL;

		    if(dir != NULL && make_empty_dir)
			ig = ignode = new ignored_dir(*dir);
		    else
			ig = new ignored(nom->get_name());
		    st.ignored++;

		    if(ig == NULL)
			throw Ememory("filtre_sauvegarde");
		    else
			cat.add(ig);

		    if(dir != NULL)
		    {
			if(make_empty_dir)
			{
			    bool known = ref.compare(dir, f);

			    try
			    {
				const inode *f_ino = known ? dynamic_cast<const inode *>(f) : NULL;
				bool tosave = false;

				if(known)
				    if(f_ino != NULL)
					tosave = dir->has_changed_since(*f_ino);
				    else
					throw SRC_BUG;
				    // catalogue::compare() with a directory should false or give a directory as
				    // second argument or here f is not an inode (f_ino == NULL) !
				    // and known == true
				else
				    tosave = true;

				ignode->set_saved_status(tosave);
			    }
			    catch(...)
			    {
				ref.compare(&tmp_eod, f);
				throw;
			    }
			    ref.compare(&tmp_eod, f);
			}
			filesystem_skip_read_to_parent_dir();
			juillet.enfile(&tmp_eod);
		    }
		    delete e;
		}
	    }
	    catch(Ebug & e)
	    {
		throw;
	    }
	    catch(Euser_abort & e)
	    {
		throw;
	    }
	    catch(Egeneric & ex)
	    {
		nomme *tmp = new ignored(nom->get_name());
		user_interaction_warning(string("Error while saving ") + juillet.get_string() + ": " + ex.get_message());
		st.errored++;
		delete e;

		if(tmp == NULL)
		    throw Ememory("fitre_sauvegarde");
		cat.add(tmp);

		if(dir != NULL)
		{
		    filesystem_skip_read_to_parent_dir();
		    juillet.enfile(&tmp_eod);
		    user_interaction_warning("NO FILE IN THAT DIRECTORY CAN BE SAVED.");
		}
	    }
	}
	else // eod
	{
	    ref.compare(e, f);
	    cat.add(e);
	}
    }
    filesystem_freemem();
}

void filtre_difference(const mask &filtre,
		       const mask &subtree,
		       catalogue & cat,
		       const path & fs_racine,
		       bool info_details, statistics & st,
		       bool check_ea_root,
		       bool check_ea_user)
{
    const entree *e;
    defile juillet = fs_racine;
    const eod tmp_eod;

    st.clear();
    filesystem_set_root(fs_racine, false, false, info_details, check_ea_root, check_ea_user);
    filesystem_reset_read();
    cat.reset_read();
    while(cat.read(e))
    {
	const directory *e_dir = dynamic_cast<const directory *>(e);
	const nomme *e_nom = dynamic_cast<const nomme *>(e);

	juillet.enfile(e);
	try
	{
	    if(e_nom != NULL)
	    {
		if(subtree.is_covered(juillet.get_string()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
		{
		    nomme *exists_nom = NULL;
		    const inode *e_ino = dynamic_cast<const inode *>(e);

		    if(e_ino != NULL)
			if(filesystem_read_filename(e_ino->get_name(), exists_nom))
			{
			    try
			    {
				inode *exists = dynamic_cast<inode *>(exists_nom);
				if(exists != NULL)
				{
				    try
				    {
					e_ino->compare(*exists, check_ea_root, check_ea_user);
					if(info_details)
					    user_interaction_warning(string("OK   ")+juillet.get_string());
					st.treated++;
				    }
				    catch(Erange & e)
				    {
					user_interaction_warning(string("DIFF ")+juillet.get_string()+": "+ e.get_message());
					st.errored++;
				    }
				}
				else // existing file is not an inode
				    throw SRC_BUG; // filesystem, should always return inode with filesystem_read_filename()
			    }
			    catch(...)
			    {
				delete exists_nom;
				throw;
			    }
			    delete exists_nom;
			}
			else // can't compare, nothing of that name in filesystem
			{
			    user_interaction_warning(string("DIFF ")+ juillet.get_string() + ": file not present in filesystem");
			    if(e_dir != NULL)
			    {
				cat.skip_read_to_parent_dir();
				juillet.enfile(&tmp_eod);
			    }
			    st.errored++;
			}
		    else // not an inode (for example a detruit, hard_link etc...), nothing to do
			st.treated++;
		}
		else // not covered by filters
		{
		    st.ignored++;
		    if(e_dir != NULL)
		    {
			cat.skip_read_to_parent_dir();
			juillet.enfile(&tmp_eod);
		    }
		}
	    }
	    else // eod ?
		if(dynamic_cast<const eod *>(e) != NULL) // yes eod
		    filesystem_skip_read_filename_in_parent_dir();
		else // no ?!?
		    throw SRC_BUG; // not nomme neither eod ! what's that ?
	}
	catch(Euser_abort &e)
	{
	    throw;
	}
	catch(Ebug &e)
	{
	    throw;
	}
	catch(Egeneric & e)
	{
	    user_interaction_warning(string("ERR  ")+juillet.get_string()+" : "+e.get_message());
	    st.deleted++;
	}
    }
    filesystem_freemem();
}

void filtre_test(const mask &filtre,
		 const mask &subtree,
		 catalogue & cat,
		 bool info_details,
		 statistics & st)
{
    const entree *e;
    defile juillet = path("<ROOT>");
    null_file black_hole = gf_write_only;
    ea_attributs ea;
    infinint offset;
    crc check, original;
    const eod tmp_eod;

    st.clear();
    cat.reset_read();
    while(cat.read(e))
    {
	juillet.enfile(e);
	try
	{
	    const file *e_file = dynamic_cast<const file *>(e);
	    const inode *e_ino = dynamic_cast<const inode *>(e);
	    const directory *e_dir = dynamic_cast<const directory *>(e);
	    const nomme *e_nom = dynamic_cast<const nomme *>(e);

	    if(e_nom != NULL)
	    {
		if(subtree.is_covered(juillet.get_string()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
		{
			// checking data file if any
		    if(e_file != NULL && e_file->get_saved_status())
		    {
			generic_file *dat = e_file->get_data();
			if(dat == NULL)
			    throw Erange("filtre_test", "Can't read saved data.");
			try
			{
			    dat->skip(0);
			    dat->copy_to(black_hole, check);
			    if(e_file->get_crc(original)) // CRC is not present in format "01"
				if(!same_crc(check, original))
				    throw Erange("fitre_test", "CRC error: data corruption.");
			}
			catch(...)
			{
			    delete dat;
			    throw;
			}
			delete dat;
		    }
			// checking inode EA if any
		    if(e_ino != NULL && e_ino->ea_get_saved_status() == inode::ea_full)
		    {
			ea_attributs tmp = *(e_ino->get_ea());
			tmp.check();
			e_ino->ea_detach();
		    }
		    st.treated++;

			// still no exception raised, this all is fine
		    if(info_details)
			user_interaction_warning(string("OK  ") + juillet.get_string());
		}
		else // excluded by filter
		{
		    if(e_dir != NULL)
		    {
			juillet.enfile(&tmp_eod);
			cat.skip_read_to_parent_dir();
		    }
		    st.skipped++;
		}
	    }
	}
	catch(Euser_abort & e)
	{
	    throw;
	}
	catch(Ebug & e)
	{
	    throw;
	}
	catch(Egeneric & e)
	{
	    user_interaction_warning(string("ERR ") + juillet.get_string() + " : " + e.get_message());
	    st.errored++;
	}
    }
}

void filtre_isolate(catalogue & cat,
		    catalogue & ref,
		    bool info_details)
{
    const entree *e;
    const eod tmp_eod;
    map<infinint, file_etiquette *> corres;

    ref.reset_read();
    cat.reset_add();

    if(info_details)
	user_interaction_warning("Removing references to saved data from catalogue...");

    while(ref.read(e))
    {
	const inode *e_ino = dynamic_cast<const inode *>(e);

	if(e_ino != NULL) // specific treatment for inode
	{
	    entree *f = e_ino->clone();
	    inode *f_ino = dynamic_cast<inode *>(f);
	    file_etiquette *f_eti = dynamic_cast<file_etiquette *>(f);
		// note about file_etiquette: the cloned object has the same etiquette
		// and thus each etiquette correspond to two instances

	    try
	    {
		if(f_ino == NULL)
		    throw SRC_BUG; // inode should clone an inode

		    // all data must be dropped
		f_ino->set_saved_status(false);

		    // all EA must be dropped also
		if(f_ino->ea_get_saved_status() == inode::ea_full)
		    f_ino->ea_set_saved_status(inode::ea_partial);

		    // mapping each etiquette to its file_etiquette clone address
		if(f_eti != NULL)
		{
		    if(corres.find(f_eti->get_etiquette()) == corres.end()) // not found
			corres[f_eti->get_etiquette()] = f_eti;
		    else
			throw SRC_BUG;
			// two file_etiquette clones have the same etiquette
			// this could be caused by a write error
			// a bit error in an infinint is still possible and
			// may make the value of the infinint (= etiquette here)
			// be changed without incoherence.
			// But, this error should have been detected at
			// catalogue reading as some hard_link cannot be associate
			// with a file_etiquette, thus this is a bug here.
		}

		cat.add(f);
	    }
	    catch(...)
	    {
		if(f != NULL)
		    delete f;
		throw;
	    }
	}
	else // other entree than inode
	    if(e != NULL)
	    {
		entree *f = e->clone();
		hard_link *f_hard = dynamic_cast<hard_link *>(f);

		try
		{
		    if(f_hard != NULL)
		    {
			map<infinint,file_etiquette *>::iterator it = corres.find(f_hard->get_etiquette());

			if(it != corres.end())
			    f_hard->set_reference(it->second);
			else
			    throw SRC_BUG;
			    // no file_etiquette of that etiquette has ever been cloned,
			    // the order being respected, an file_etiquette is come always first
			    // before any hard_link on it, as there is no filter to skip the
			    // file_etiquette, thus it's a bug.
		    }

		    cat.add(f);
		}
		catch(...)
		{
		    if(f != NULL)
			delete f;
		    throw;
		}
	    }
	    else
		throw SRC_BUG; // read provided NULL while returning true
    }
}



static void dummy_call(char *x)
{
    static char id[]="$Id: filtre.cpp,v 1.51 2002/06/26 22:20:20 denis Rel $";
    dummy_call(id);
}

static void save_inode(const string & info_quoi, inode * & ino, compressor *stock, bool info_details)
{
    if(ino == NULL || stock == NULL)
	throw SRC_BUG;
    if(!ino->get_saved_status())
	return;
    if(info_details)
	user_interaction_warning(string("Adding file to archive: ") + info_quoi);

    file *fic = dynamic_cast<file *>(ino);

    if(fic != NULL)
    {
	infinint start = stock->get_position();
	generic_file *source = fic->get_data();
	crc val;

	fic->set_offset(start);
	if(source != NULL)
	{
	    try
	    {
		source->copy_to(*stock, val);
		stock->flush_write();
		fic->set_crc(val);
	    }
	    catch(...)
	    {
		delete source;
		throw;
	    }
	    delete source;
	}
	else
	    throw SRC_BUG; // saved_status = true, but no data available, and no exception raised;
	fic->set_storage_size(stock->get_position() - start);
    }
}

static bool save_ea(const string & info_quoi, inode * & ino, compressor *stock, const inode * ref, bool info_details)
{
    bool ret = false;
    try
    {
	switch(ino->ea_get_saved_status())
	{
	case inode::ea_full: // if there is something to save
	    if(ref == NULL || ref->ea_get_saved_status() == inode::ea_none || ref->get_last_change() < ino->get_last_change())
	    {
		if(ino->get_ea() != NULL)
		{
		    crc val;

		    if(info_details)
			user_interaction_warning(string("Saving Extended Attributes for ") + info_quoi);
		    ino->ea_set_offset(stock->get_position());
		    stock->reset_crc(); // start computing CRC for any read/write on stock
		    try
		    {
			ino->get_ea()->dump(*stock);
		    }
		    catch(...)
		    {
			stock->get_crc(val); // keeps stocks in a coherent status
			throw;
		    }
		    stock->get_crc(val);
		    ino->ea_set_crc(val);
		    ino->ea_detach();
		    stock->flush_write();
		    ret = true;
		}
		else
		    throw SRC_BUG;
	    }
	    else // EA have not changed, dropping the EA infos
		ino->ea_set_saved_status(inode::ea_partial);
	    break;
	case inode::ea_partial:
	    throw SRC_BUG; //filesystem, must not provide inode in such a status
	case inode::ea_none: // no EA has been seen
	    if(ref != NULL && ref->ea_get_saved_status() != inode::ea_none) // if there was some before
	    {
		    // we must record the EA have been dropped since ref backup
		ea_attributs ea;
		ino->ea_set_saved_status(inode::ea_full);
		ino->ea_set_offset(stock->get_position());
		ea.clear(); // be sure it is empty
		if(info_details)
		    user_interaction_warning(string("Saving Extended Attributes for ") + info_quoi);
		ea.dump(*stock);
		stock->flush_write();
		    // no need to detach, as the brand new ea has not been attached
		ret = true;
	    }
	    break;
	default:
	    throw SRC_BUG;
	}
    }
    catch(Ebug & e)
    {
	throw;
    }
    catch(Euser_abort & e)
    {
	throw;
    }
    catch(Egeneric & e)
    {
	user_interaction_warning(string("Error saving Extended Attributs for ") + info_quoi + ": " + e.get_message());
    }
    return ret;
}