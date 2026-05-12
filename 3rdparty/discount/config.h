
//第三分依赖
/*
 * configuration for markdown (discount)
 * Cross-platform config for use with qmake build
 */
#ifndef __AC_MARKDOWN_D
#define __AC_MARKDOWN_D 1

#ifdef _WIN32
#define OS_WIN32 1
#include <Windows.h>
#else
#define OS_DARWIN 1
#endif

/* Discount link types */
#define USE_DISCOUNT_DL 1

/* DWORD/WORD/BYTE types used throughout discount */
#ifndef DWORD
#define DWORD unsigned long
#endif

#ifndef WORD
#define WORD unsigned short
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

/* Version */
#define VERSION "2.1.8"

/* Tab stop width */
#define TABSTOP 4

/* String functions */
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1

/* System functions */
#define HAVE_FCHDIR 1
#define HAVE_STAT 1
#define HAVE_PWD_H 1

/* Random number support */
#ifdef _WIN32
#define INITRNG(x) srand((unsigned int)(x))
#define COINTOSS() (rand()&1)
#define HAVE_RANDOM 0
#else
#define INITRNG(x) srandom((unsigned int)(x))
#define COINTOSS() (random()&1)
#define HAVE_RANDOM 1
#endif

/* malloc.h availability */
#ifdef _WIN32
#define HAVE_MALLOC_H 1
#else
#define HAVE_MALLOC_H 0
#endif

#endif /* __AC_MARKDOWN_D */
