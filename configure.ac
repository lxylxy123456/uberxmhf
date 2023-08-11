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

# https://stackoverflow.com/questions/71709907/
AC_CONFIG_HEADERS([include/config.h])

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

# Define macros that will be passed to C and assembly files, following:
# https://stackoverflow.com/questions/71709907/

shv_opt=0
AC_ARG_WITH([shv_opt],
            [AS_HELP_STRING([--with-shv-opt=SHV_OPT],
                            [specify SHV options, see shv.h])],
            [shv_opt="$withval"], [])
AC_DEFINE_UNQUOTED([SHV_OPT],[${shv_opt}],[SHV options])

# Tells automake to create a Makefile
# See https://www.gnu.org/software/automake/manual/html_node/Requirements.html
AC_CONFIG_FILES([Makefile])

# Generate the output
AC_OUTPUT
