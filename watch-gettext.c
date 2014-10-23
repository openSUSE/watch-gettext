/* wrap-gettext.c -- gettext() wrapper

   Copyright (C) SUSE Linux
   Written by Stanislav Brabec, 2014

trace-wrappers is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

trace-wrappers is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with trace-wrappers; see the file COPYING.  If not, a
copy can be downloaded from  http://www.gnu.org/licenses/lgpl.html,
or obtained by writing to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <dlwrap/dlwrap.h>
#include <stdio.h>
#include <libintl.h>
#include <execinfo.h>
#include <glib.h>

#define int_category_decl		int
#define	int_category_printf		"%s"

/* FIXME: repeated gettext calls cause memory lags and requires better implementation */

static void libinit(void) __attribute__((constructor));
static void libexit(void) __attribute__((destructor));

static int refno = 0;
static FILE *dlwrap_file;
static GHashTable *msg_table;

/* Operate with msgid as a pointer, not as a string. It can cause
 * duplicite entries, but not more as the number of ocurrences in the
 * source code. It is easy to convert to use string hashes. */

void libinit(void) {
	dlwrap_file = fopen ("watch-gettext.log", "w");
	msg_table = g_hash_table_new (NULL, NULL);
}

void libexit(void) {
	fclose (dlwrap_file);
	g_hash_table_destroy (msg_table);
}


/* Obtain a backtrace and print it to stdout. */
#define STACK_LEVELS 3
#define SKIP_STACK_TOP 3
void
print_trace (void)
{
  void *array[STACK_LEVELS+SKIP_STACK_TOP];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, STACK_LEVELS+SKIP_STACK_TOP);
  strings = backtrace_symbols (array, size);

  for (i = SKIP_STACK_TOP; i < size; i++)
     fprintf (dlwrap_file, "%s\n", strings[i]);

  fprintf (dlwrap_file, "\n");
  free (strings);
}

int log_gettext (const char *domainname, const char *msgid, const char * msgid_plural)
{
	int ref;

	if (!(ref = GPOINTER_TO_INT(g_hash_table_lookup (msg_table, msgid))))
	{
		ref = ++refno;
		g_hash_table_insert (msg_table, (gpointer)msgid, GINT_TO_POINTER (ref));
		fprintf (dlwrap_file, "[%d] (%p) %s: %s\n", refno, msgid, (domainname ? domainname : textdomain (NULL)), msgid);
		fflush (dlwrap_file);
		print_trace();
	}
	return ref;
}

#undef dlwrap_macro_3_nonvoid
#define dlwrap_macro_3_nonvoid(return_type, name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3) \
	return_type##_decl return_code; \
	return_type##_decl modreturn_code; \
	return_code = dlwrap_orig_3_nonvoid(return_type, name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3); \
	asprintf (&modreturn_code, "[%d]%s", log_gettext(arg1, arg2, NULL), return_code); \
	return modreturn_code;

#undef dlwrap_macro_5_nonvoid
#define dlwrap_macro_5_nonvoid(return_type, name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4, arg5_type, arg5) \
	return_type##_decl return_code; \
	return_type##_decl modreturn_code; \
	return_code = dlwrap_orig_5_nonvoid(return_type, name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4, arg5_type, arg5); \
	asprintf (&modreturn_code, "[%d]%s", log_gettext(arg1, arg2, arg3), return_code); \
	return modreturn_code;

dlwrap_install_3 (char_pointer, dcgettext, constant_char_pointer, constant_char_pointer, int_category);
dlwrap_install_5 (char_pointer, dcngettext, constant_char_pointer, constant_char_pointer, constant_char_pointer, unsigned_long_int, int);
