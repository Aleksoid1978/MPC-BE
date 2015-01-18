/* opj_config.h.  Generated from opj_config.h.in by configure.  */
/* opj_config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* define to 1 if you have lcms version 1.x */
/* #undef HAVE_LIBLCMS1 */

/* define to 1 if you have lcms version 2.x */
/* #undef HAVE_LIBLCMS2 */

/* define to 1 if you have libpng */
/* #undef HAVE_LIBPNG */

/* define to 1 if you have libtiff */
/* #undef HAVE_LIBTIFF */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "openjpeg"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "openjpeg@googlegroups.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "OpenJPEG"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "OpenJPEG 2.0.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "openjpeg"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.openjpeg.org"

/* Define to the version of this package. */
#define OPJ_PACKAGE_VERSION "2.0.0"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* define to 1 if you use jpip */
/* #undef USE_JPIP */

/* define to 1 if you use jpip server */
/* #undef USE_JPIP_SERVER */

/* define to 1 if you use mj2 */
/* #undef USE_MJ2 */

/* Version number of package */
#define VERSION "2.0.0"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif
