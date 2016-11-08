/* create opj_config_private.h for CMake */
#define OPJ_HAVE_INTTYPES_H 	1

#define OPJ_PACKAGE_VERSION "2.1.0"

/* Not used by openjp2*/
/*#define HAVE_MEMORY_H 1*/
/*#define HAVE_STDLIB_H 1*/
/* #undef HAVE_STRINGS_H */
/*#define HAVE_STRING_H 1*/
/*#define HAVE_SYS_STAT_H 1*/
/*#define HAVE_SYS_TYPES_H 1 */
/* #undef HAVE_UNISTD_H */

/* #undef _LARGEFILE_SOURCE */
/* #undef _LARGE_FILES */
/* #undef _FILE_OFFSET_BITS */
/* #undef OPJ_HAVE_FSEEKO */

/* find whether or not have <malloc.h> */
#define OPJ_HAVE_MALLOC_H
/* check if function `aligned_alloc` exists */
/* #undef OPJ_HAVE_ALIGNED_ALLOC */
/* check if function `_aligned_malloc` exists */
#define OPJ_HAVE__ALIGNED_MALLOC
/* check if function `memalign` exists */
/* #undef OPJ_HAVE_MEMALIGN */
/* check if function `posix_memalign` exists */
/* #undef OPJ_HAVE_POSIX_MEMALIGN */

/* Byte order.  */
/* All compilers that support Mac OS X define either __BIG_ENDIAN__ or
__LITTLE_ENDIAN__ to match the endianness of the architecture being
compiled for. This is not necessarily the same as the architecture of the
machine doing the building. In order to support Universal Binaries on
Mac OS X, we prefer those defines to decide the endianness.
On other platforms we use the result of the TRY_RUN. */
#if !defined(__APPLE__)
/* #undef OPJ_BIG_ENDIAN */
#elif defined(__BIG_ENDIAN__)
# define OPJ_BIG_ENDIAN
#endif
