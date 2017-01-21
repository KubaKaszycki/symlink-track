/*
 * Copyright (C) 2016 Jakub Kaszycki
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <config.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Current file. This buffer is <code>malloc()</code>ed and <code>free()</code>ed.
 * 
 * @since 0.1.0
 */
static char *current;

/**
 * Next file. Contents of this variable should be considered internal. 
 * 
 * @since 0.1.0
 */
static char *next;

/**
 * The number of hops performed so far.
 * 
 * @since 0.1.0
 */
static size_t hops = 0;

/**
 * Maximum number of hops allowed.
 * 
 * @since 0.1.0
 */
static size_t max_hops = SIZE_MAX;

/**
 * Long options.
 * 
 * @since 0.1.0
 */
static const struct option LONGOPTS[] = {
  {	"help", no_argument, NULL, 'h' },
  { "max", required_argument, NULL, 'm' },
  { "version", no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 },
};

#ifndef S_ISLNK
# define S_ISLNK(val) (((val) & S_IFMT) == S_IFLNK)
#endif

/**
 * Finds the next component. Returns a code from:
 * 
 * <table>
 *  <tr><td>0</td><td>current element was a symlink, found next one</td></tr>
 *  <tr><td>1</td><td>current element is not a symlink</td></tr>
 *  <tr><td>2</td><td>current points to inexistent file</td></tr>
 *  <tr><td>3</td><td>error, errno preserved</td></tr>
 * </table>
 * 
 * @return code
 * 
 * @since 0.1.0
 */
static int
find_next (void)
{
  struct stat sbuf;
  if (lstat (current, &sbuf) == -1)
    {
      return 3;
    }

  if (!S_ISLNK (sbuf.st_mode))
    {
      return 1;
    }

  /* Fix for GNU Hurd */
#if !defined (PATH_MAX) && defined (__gnu_hurd__)
  /* Hurd has no limits ;-)
     Define it to a sane value */
#define PATH_MAX 32767
#endif
  next = (char *) malloc (PATH_MAX + 1);
  if (!next)
    {
      return 3;
    }
  memset (next, 0, PATH_MAX + 1);

  ssize_t rl_ret = readlink (current, next, PATH_MAX + 1);

  if (rl_ret < 0)
    {
      free (next);
      return 3;
    }

  if (rl_ret == PATH_MAX + 1)
    {
      free (next);
      errno = ENAMETOOLONG;
      return 3;
    }
  next[(size_t) rl_ret] = '\0';

  char *dir = (char *) malloc (PATH_MAX + 1);
  if (!dir)
    {
      return -3;
    }
  memset (dir, 0, PATH_MAX + 1);
  memcpy (dir, current, strlen (current) + 1);

    {
      char *old_dir = dir;
      dir = dirname (old_dir);
      if (dir != old_dir)
        {
          memcpy (old_dir, dir, strlen (dir) + 1);
          dir = old_dir;
        }
    }

  if (next[0] != '/')
    {
      char *old_next = next;
      next = (char *) malloc (PATH_MAX + 1);
      memset (next, 0, PATH_MAX + 1);
      if (snprintf (next, PATH_MAX, "%s/%s", dir, old_next) < 1)
        {
          return 3;
        }
      free (old_next);
    }

  free (dir);

  if (lstat (next, &sbuf) == -1)
    {
      if (errno == ENOENT)
        {
          return 2;
        }
      else
        {
          return 3;
        }
    }

  return 0;
}

/**
 * Gets a file type string from a path name. It is guaranteed to return a valid
 * string which <strong>shall not</strong> be freed.
 * 
 * @param path path of file
 * @return file type string
 * 
 * @since 0.1.0
 */
static const char *
get_ftype (const char *path)
{
  struct stat sbuf;

  if (lstat (path, &sbuf) == -1)
    {
      return "unknown file";
    }

  switch (sbuf.st_mode & S_IFMT)
    {
    case S_IFREG:
      return "regular file";
    case S_IFSOCK:
      return "socket";
    case S_IFCHR:
      return "character device";
    case S_IFBLK:
      return "block device";
    case S_IFIFO:
      return "named pipe, FIFO";
    case S_IFDIR:
      return "directory";
    case S_IFLNK:
      return "symbolic link";
#ifdef S_IFWHT
    case S_IFWHT:
      return "whiteout";
#endif
    default:
      return "unknown file";
    }
}

static void
print_usage (FILE *stream, const char *alias)
{
  fprintf (stream, "Usage: %s [options...] <FILE>\n", alias);
}

/**
 * Does all the necessary option parsing.
 * 
 * @param argc number of arguments
 * @param argv arguments
 * @return zero if successful, exit error code otherwise
 * 
 * @since 0.1.0
 */
static int
do_optparse (int argc, char **argv)
{
  int x;

  while ((x = getopt_long (argc, argv, "hm:v", LONGOPTS, NULL)) > 0)
    {
      switch (x)
        {
        case 'h':
          print_usage (stdout, argv[0]);
          printf ("Create a path of resolving symbolic links\n"
                  "\n"
                  "Options:\n"
                  "  -h --help                 - Display this help\n"
                  "  -m <MAX> --max-hops <MAX> - Do a maximum number of <MAX>\n"
                  "                              hops\n"
                  "  -v --version              - Display package version\n"
                  "\n"
                  "Report any bugs to <%s>.\n", PACKAGE_BUGREPORT);
          exit (0);
          return 0;
        case 'm':;
          char *tmp;
          ssize_t maxhop = strtoll (optarg, &tmp, 10);
          if (*tmp || maxhop <= 0)
            {
              fprintf (stderr, "Invalid argument to -m, must be positive "
                       "integer: %s\n", optarg);
              return 2;
            }
          max_hops = (size_t) maxhop;
          break;
        case 'v':
          puts (PACKAGE_STRING);
          exit (0);
          return 0;
        case '?':
          print_usage (stderr, argv[0]);
          fputs ("Invoke with parameter --help for more information\n",
                 stderr);
          return 2;
        default:
          ;
        }
    }

  char *alias = argv[0];

  argc -= optind;
  argv += optind;

  if (argc < 1)
    {
      print_usage (stderr, alias);
      fputs ("Invoke with parameter --help for more information\n", stderr);
      return 2;
    }
  if (argc > 1)
    {
      fprintf (stderr, "Warning: Currently, %s supports only one file at a "
               "time.\n", alias);
    }

  size_t len = strlen (argv[0]) + 1;
  current = malloc (len);
  memcpy (current, argv[0], len);

  return 0;
}

/**
 * Program entry point.
 * 
 * @param argc number of arguments
 * @param argv arguments
 * @return exit code
 * 
 * @since 0.1.0
 */
int
main (int argc, char **argv)
{
  int x;

  /* No, it SHALL NOT be == */
  if ((x = do_optparse (argc, argv)))
    {
      return x;
    }

  while (1)
    {
      int status = find_next ();

      hops++;

      if (status == 1 || hops >= max_hops)
        {
          // End this.
          printf ("%s (%s)\n", current, get_ftype (current));
          free (current);
          break;
        }
      else if (status == 0)
        {
          printf ("%s -> ", current);
          free (current);
          current = next;
        }
      else if (status == 2)
        {
          printf ("%s (nonexistent)\n", current);
          free (current);
          return EXIT_FAILURE;
        }
      else if (status == 3)
        {
          printf ("%s (I/O error)\n", current);
          fprintf (stderr, "Error while tracking %s: %s\n", current,
                   strerror (errno));
          free (current);
          return EXIT_FAILURE;
        }
      else
        {
          printf ("%s (internal error)\n", current);
          fprintf (stderr, "Internal error: wrong find_next() code: %d\n",
                   status);
          free (current);
          return EXIT_FAILURE;
        }
    }

  return EXIT_SUCCESS;
}
