# Template from http://www.idryman.org/blog/2016/03/10/autoconf-tutorial-1/

# Must init the autoconf setup
# The first parameter is project name
# second is version number
# third is bug report address
AC_INIT([shv], [1.0])

# Safety checks in case user overwritten --srcdir
AC_CONFIG_SRCDIR([kernel.c])

# Store the auxiliary build tools (e.g., install-sh, config.sub, config.guess)
# in this dir (build-aux)
AC_CONFIG_AUX_DIR([build-aux])

# Init automake, and specify this program use relaxed structures.
# i.e. this program doesn't follow the gnu coding standards, and doesn't have
# ChangeLog, COPYING, AUTHORS, INSTALL, README etc. files.
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Check for C compiler
AC_PROG_CC
AM_PROG_AS
AC_CHECK_PROGS([GRUB_MKRESCUE], [grub2-mkrescue grub-mkrescue])
# We can add more checks in this section

AM_CONDITIONAL([I386], [test x"$host_alias" = x"i686-linux-gnu"])

# Tells automake to create a Makefile
# See https://www.gnu.org/software/automake/manual/html_node/Requirements.html
AC_CONFIG_FILES([Makefile])

# Generate the output
AC_OUTPUT
