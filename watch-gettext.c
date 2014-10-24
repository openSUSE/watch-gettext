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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <libintl.h>
#include <execinfo.h>
#include <time.h>

#include <glib.h>

#include <dlwrap/dlwrap.h>

#define int_category_decl		int
#define	int_category_printf		"%s"

static void libinit(void) __attribute__((constructor));
static void libexit(void) __attribute__((destructor));

static int refno = 0;
static FILE *dlwrap_file;
static GHashTable *msg_table;

static void libinit(void)
{
	char *watch_gettext_dir = NULL, *filename, *fullname;
	time_t current_time;
	struct tm *time_tm;

	watch_gettext_dir = getenv ("WATCH_GETTEXT_DIR");

	asprintf (&filename, "watch-gettext-%s.po", program_invocation_short_name);
	if (watch_gettext_dir)
	{
		fullname = g_build_filename (watch_gettext_dir, filename, NULL);
		g_free (filename);
	}
	else
		fullname = filename;

	if (!(dlwrap_file = fopen (fullname, "wx")))
	{
		g_free (fullname);
		asprintf (&filename, "watch-gettext-%s-%d.po", program_invocation_short_name, getpid ());
		if (watch_gettext_dir)
		{
			fullname = g_build_filename (watch_gettext_dir, filename, NULL);
			free (filename);
		}
		else
			fullname = filename;
	}

	dlwrap_file = fopen (fullname, "w");
	g_free (fullname);

	time (&current_time);
	time_tm = localtime (&current_time);
	fprintf (dlwrap_file, "# wrap-gettext pseudo-po file\n# generated: %s",
		 asctime(time_tm));

	msg_table = g_hash_table_new (g_str_hash, g_str_equal);
}

static void libexit(void)
{
	fclose (dlwrap_file);
	g_hash_table_destroy (msg_table);
}


/* Obtain a backtrace and print it to stdout. */
#define STACK_LEVELS 3
#define SKIP_STACK_TOP 3
static void print_trace (void)
{
	void *array[STACK_LEVELS+SKIP_STACK_TOP];
	size_t size;
	char **strings;
	size_t i;
	size = backtrace (array, STACK_LEVELS+SKIP_STACK_TOP);
	strings = backtrace_symbols (array, size);
	for (i = SKIP_STACK_TOP; i < size; i++)
		fprintf (dlwrap_file, "# %s\n", strings[i]);
	free (strings);
}

typedef struct
{
	FILE *stream;
	bool opened;
	const char **current_ptr;
	bool is_pending_buf;
	const char *pending_buf;
}
msg_t;

static void mopen (msg_t *msg)
{
	if (!msg->opened)
	{
		fputc ('"', msg->stream);
		msg->opened = true;
	}
}
static void mflush (msg_t *msg)
{
	if (msg->is_pending_buf)
	{
		mopen (msg);
		fwrite ((const void *)(msg->pending_buf), sizeof (char), *msg->current_ptr - msg->pending_buf, msg->stream);
		msg->is_pending_buf = false;
		msg->pending_buf = *msg->current_ptr;
	}
}

static void mclose (msg_t *msg)
{
	mflush (msg);
	if (msg->opened)
	{
		fputs ("\"\n", msg->stream);
		msg->opened = false;
	}
}

static void xprint (msg_t *msg, char c)
{
	mflush (msg);
	mopen (msg);
	fputc ('\\', msg->stream);
	fputc (c, msg->stream);
	msg->pending_buf++;
}

static void print_esc (FILE *fp, const char *s, const char *msgtype)
{
	msg_t msg;

	msg.stream = fp;
	msg.opened = false;
	msg.current_ptr = &s;
	msg.pending_buf = s;
	msg.is_pending_buf = false;

	/* pretty printing: multiline => start with msgid "" */
	if (strchr (s, '\n'))
		fprintf (fp, "%s \"\"\n", msgtype);
	else
		fprintf (fp, "%s ", msgtype);

//	fprintf (fp, "@@@@%s@@@@\n", s);
	while (*s)
	{
		switch (*s)
		{
		case '\t':
			xprint (&msg, 't');
			break;
		case '\n':
			xprint (&msg, 'n');
//		fprintf (fp, "=> opened=%d, current=%p/pending=%p, is_pending=%d\n", msg.opened, *msg.current_ptr, msg.pending_buf, msg.is_pending_buf);
			mclose (&msg);
			break;
		case '\r':
			xprint (&msg, 'r');
			break;
		case '\f':
			xprint (&msg, 'f');
			break;
		case '\\':
		case '"':
			xprint (&msg, *s);
			break;
		default:
			msg.is_pending_buf = true;
			break;
		}
//		fprintf (fp, "=> opened=%d, current=%p/pending=%p, is_pending=%d\n", msg.opened, *msg.current_ptr, msg.pending_buf, msg.is_pending_buf);
		s++;
	}
	mclose (&msg);
}

static void wrap_gettext (char **mod_msgstr, const char *func, const char *domainname, const char *msgid, const char * msgid_plural, const char * msgstr)
{
	int ref;
	const char *pure_msgid = msgid, *pure_msgstr = msgstr, *ctx_ch;
	char *ctxt = NULL;

	if ((ctx_ch = strchr (msgid, '\004')))
	{
		ctxt = g_new (char, ctx_ch - msgid + 1);
		strncpy (ctxt, msgid, ctx_ch - msgid);
		ctxt[ctx_ch - msgid] = (char)0;
		pure_msgid = ctx_ch + 1;
		/* FIXME: Verify msgid_plural with context! */
		if ((ctx_ch = strchr (msgstr, '\004')))
		{
			pure_msgstr = ctx_ch + 1;
		}
	}

	if (!(ref = GPOINTER_TO_INT(g_hash_table_lookup (msg_table, msgid))))
	{
		ref = ++refno;
		g_hash_table_insert (msg_table, (gpointer)msgid, GINT_TO_POINTER (ref));
		fprintf (dlwrap_file, "\n"
			 "#. [%d] %s()\n"
			 "#: %s:%p\n",
			 refno, func,
			 (domainname ? domainname : textdomain (NULL)), msgid);
		print_trace();

		if (ctxt)
			fprintf (dlwrap_file, "msgctxt \"%s\"\n", ctxt);
		print_esc (dlwrap_file, pure_msgid, "msgid");
		if (msgid_plural)
		{
			print_esc (dlwrap_file, msgid_plural, "msgid_plural");
			/* FIXME: Fetch all plurals! */
			print_esc (dlwrap_file, pure_msgstr, "msgstr[FIXME]");
		}
		else
			print_esc (dlwrap_file, pure_msgstr, "msgstr");
		fflush (dlwrap_file);
	}
	asprintf (mod_msgstr, "[%d]%s", ref, pure_msgstr);

	g_free (ctxt);
}

#undef dlwrap_macro_3_nonvoid
#define dlwrap_macro_3_nonvoid(return_type, name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3) \
	return_type##_decl return_code; \
	return_type##_decl modreturn_code; \
	return_code = dlwrap_orig_3_nonvoid(return_type, name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3); \
	wrap_gettext (&modreturn_code, #name, arg1, arg2, NULL, return_code); \
	return modreturn_code;

#undef dlwrap_macro_5_nonvoid
#define dlwrap_macro_5_nonvoid(return_type, name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4, arg5_type, arg5) \
	return_type##_decl return_code; \
	return_type##_decl modreturn_code; \
	return_code = dlwrap_orig_5_nonvoid(return_type, name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4, arg5_type, arg5); \
	wrap_gettext (&modreturn_code, #name, arg1, arg2, arg3, return_code); \
	return modreturn_code;

dlwrap_install_3 (char_pointer, dcgettext, constant_char_pointer, constant_char_pointer, int_category);
dlwrap_install_5 (char_pointer, dcngettext, constant_char_pointer, constant_char_pointer, constant_char_pointer, unsigned_long_int, int);
