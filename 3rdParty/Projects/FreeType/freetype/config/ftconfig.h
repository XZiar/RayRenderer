/****************************************************************************
 *
 * ftconfig.h.in
 *
 *   UNIX-specific configuration file (specification only).
 *
 * Copyright (C) 1996-2021 by
 * David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 * This file is part of the FreeType project, and may only be used,
 * modified, and distributed under the terms of the FreeType project
 * license, LICENSE.TXT.  By continuing to use, modify, or distribute
 * this file you indicate that you have read the license and
 * understand and accept it fully.
 *
 */


  /**************************************************************************
   *
   * This header file contains a number of macro definitions that are used by
   * the rest of the engine.  Most of the macros here are automatically
   * determined at compile time, and you should not need to change it to port
   * FreeType, except to compile the library with a non-ANSI compiler.
   *
   * Note however that if some specific modifications are needed, we advise
   * you to place a modified copy in your build directory.
   *
   * The build directory is usually `builds/<system>`, and contains
   * system-specific files that are always included first when building the
   * library.
   *
   */

#ifndef FTCONFIG_H_
#define FTCONFIG_H_

#include <ft2build.h>
#include FT_CONFIG_OPTIONS_H
#include FT_CONFIG_STANDARD_LIBRARY_H

#ifdef __unix__ // for unix
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1

#undef FT_USE_AUTOCONF_SIZEOF_TYPES
#ifdef FT_USE_AUTOCONF_SIZEOF_TYPES

#undef SIZEOF_INT
#undef SIZEOF_LONG
#define FT_SIZEOF_INT  SIZEOF_INT
#define FT_SIZEOF_LONG SIZEOF_LONG

#endif /* FT_USE_AUTOCONF_SIZEOF_TYPES */
#endif

#include <freetype/config/integer-types.h>
#include <freetype/config/public-macros.h>
#include <freetype/config/mac-support.h>

#endif /* FTCONFIG_H_ */


/* END */
