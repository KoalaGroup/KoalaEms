/* makepath.c -- Ensure that a directory path exists.
   Copyright (C) 1990, 1997, 1998, 1999, 2000, 2002, 2003 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Written by David MacKenzie <djm@gnu.ai.mit.edu> and Jim Meyering.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#if __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #  pragma alloca
#  else
char *alloca ();
#  endif
# endif
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if STAT_MACROS_BROKEN
# undef S_ISDIR
#endif

#if !defined S_ISDIR && defined S_IFDIR
# define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_IRWXUGO
# define S_IRWXUGO (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

#if STDC_HEADERS
# include <stdlib.h>
#endif

#include <errno.h>

#ifndef errno
extern int errno;
#endif

#if HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
# ifndef strchr
#  define strchr index
# endif
#endif

#ifndef S_ISUID
# define S_ISUID 04000
#endif
#ifndef S_ISGID
# define S_ISGID 02000
#endif
#ifndef S_ISVTX
# define S_ISVTX 01000
#endif
#ifndef S_IRUSR
# define S_IRUSR 0200
#endif
#ifndef S_IWUSR
# define S_IWUSR 0200
#endif
#ifndef S_IXUSR
# define S_IXUSR 0100
#endif

#ifndef S_IRWXU
# define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#endif

#define WX_USR (S_IWUSR | S_IXUSR)

/* Include this before libintl.h so we get our definition of PARAMS. */
#include "makepath.h"

#define CLEANUP						\
  do							\
    {							\
      umask (oldmask);					\
    }							\
  while (0)

/* Attempt to create directory DIR (aka DIRPATH) with the specified MODE.
   If CREATED_DIR_P is non-NULL, set *CREATED_DIR_P to non-zero if this
   function creates DIR and to zero otherwise.  Give a diagnostic and
   return non-zero if DIR cannot be created or cannot be determined to
   exist already.  Use DIRPATH in any diagnostic, not DIR.
   Note that if DIR already exists, this function returns zero
   (indicating success) and sets *CREATED_DIR_P to zero.  */

int
make_dir (const char *dir, const char *dirpath, mode_t mode, int *created_dir_p)
{
  int fail = 0;
  int created_dir;

  created_dir = (mkdir (dir, mode) == 0);

  if (!created_dir)
    {
      struct stat stats;
      int saved_errno = errno;

      /* The mkdir and stat calls below may appear to be reversed.
	 They are not.  It is important to call mkdir first and then to
	 call stat (to distinguish the three cases) only if mkdir fails.
	 The alternative to this approach is to `stat' each directory,
	 then to call mkdir if it doesn't exist.  But if some other process
	 were to create the directory between the stat & mkdir, the mkdir
	 would fail with EEXIST.  */

      if (stat (dir, &stats))
	{
	  fprintf(stderr, "cannot create directory %s: %s\n", dirpath, strerror(saved_errno));
	  fail = 1;
	}
      else if (!S_ISDIR (stats.st_mode))
	{
	  fprintf(stderr, "%s exists but is not a directory\n", dirpath);
	  fail = 1;
	}
      else
	{
	  /* DIR (aka DIRPATH) already exists and is a directory. */
	}
    }

  if (created_dir_p)
    *created_dir_p = created_dir;

  return fail;
}

/* Ensure that the directory ARGPATH exists.

   Create any leading directories that don't already exist, with
   permissions PARENT_MODE.
   If the last element of ARGPATH does not exist, create it as
   a new directory with permissions MODE.
   If OWNER and GROUP are non-negative, use them to set the UID and GID of
   any created directories.
   If VERBOSE_FMT_STRING is nonzero, use it as a printf format
   string for printing a message after successfully making a directory,
   with the name of the directory that was just made as an argument.
   If PRESERVE_EXISTING is non-zero and ARGPATH is an existing directory,
   then do not attempt to set its permissions and ownership.

   Return 0 if ARGPATH exists as a directory with the proper
   ownership and permissions when done, otherwise 1.  */

int
make_path (const char *argpath,
	   int mode,
	   int parent_mode)
{
  struct stat stats;
  int retval = 0;

  if (stat (argpath, &stats))
    {
      char *slash;
      int tmp_mode;		/* Initial perms for leading dirs.  */
      int re_protect;		/* Should leading dirs be unwritable? */
      struct ptr_list
      {
	char *dirname_end;
	struct ptr_list *next;
      };
      struct ptr_list *p, *leading_dirs = NULL;
      char *basename_dir;
      char *dirpath;

      /* Temporarily relax umask in case it's overly restrictive.  */
      mode_t oldmask = umask (0);

      /* Make a copy of ARGPATH that we can scribble NULs on.  */
      dirpath = (char *) alloca (strlen (argpath) + 1);
      strcpy (dirpath, argpath);

      /* If leading directories shouldn't be writable or executable,
	 or should have set[ug]id or sticky bits set and we are setting
	 their owners, we need to fix their permissions after making them.  */
      if ((parent_mode & WX_USR) != WX_USR)
	{
	  tmp_mode = S_IRWXU;
	  re_protect = 1;
	}
      else
	{
	  tmp_mode = parent_mode;
	  re_protect = 0;
	}

      slash = dirpath;

      /* Skip over leading slashes.  */
      while (*slash == '/')
	slash++;

      while (1)
	{
	  int newly_created_dir;
	  int fail;

	  /* slash points to the leftmost unprocessed component of dirpath.  */
	  basename_dir = slash;

	  slash = strchr (slash, '/');
	  if (slash == NULL)
	    break;

	  basename_dir = dirpath;

	  *slash = '\0';
	  fail = make_dir (basename_dir, dirpath, tmp_mode, &newly_created_dir);
	  if (fail)
	    {
	      CLEANUP;
	      return 1;
	    }

	  if (newly_created_dir)
	    {
	      if (re_protect)
		{
		  struct ptr_list *new = (struct ptr_list *)
		    alloca (sizeof (struct ptr_list));
		  new->dirname_end = slash;
		  new->next = leading_dirs;
		  leading_dirs = new;
		}
	    }

	  *slash++ = '/';

	  /* Avoid unnecessary calls to `stat' when given
	     pathnames containing multiple adjacent slashes.  */
	  while (*slash == '/')
	    slash++;
	}

      basename_dir = dirpath;

      /* Done creating leading directories.  Restore original umask.  */
      umask (oldmask);

      /* We're done making leading directories.
	 Create the final component of the path.  */

      if (make_dir (basename_dir, dirpath, mode, NULL))
	{
	  CLEANUP;
	  return 1;
	}

      /* The above chown may have turned off some permission bits in MODE.
	 Another reason we may have to use chmod here is that mkdir(2) is
	 required to honor only the file permission bits.  In particular,
	 it need not honor the `special' bits, so if MODE includes any
	 special bits, set them here.  */
      if ((mode & ~S_IRWXUGO)
	  && chmod (basename_dir, mode))
	{
            fprintf(stderr, "cannot change permissions of %s\n", dirpath);
	  retval = 1;
	}

      /* If the mode for leading directories didn't include owner "wx"
	 privileges, we have to reset their protections to the correct
	 value.  */
      for (p = leading_dirs; p != NULL; p = p->next)
	{
	  *(p->dirname_end) = '\0';
	  if (chmod (dirpath, parent_mode))
	    {
                fprintf(stderr, "cannot change permissions of %s\n", dirpath);
	      retval = 1;
	    }
	}
    }
  else
    {
      /* We get here if the entire path already exists.  */

      const char *dirpath = argpath;

      if (!S_ISDIR (stats.st_mode))
	{
            fprintf(stderr, "%s exists but is not a directory\n", dirpath);
	  return 1;
	}

      if (1 /*!preserve_existing*/)
	{
	  if (chmod (dirpath, mode))
	    {
                fprintf(stderr, "cannot change permissions of %s\n", dirpath);
	      retval = 1;
	    }
	}
    }

  return retval;
}
