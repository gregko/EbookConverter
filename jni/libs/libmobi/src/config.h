/** @file src/config.h
 *
 * Copyright (c) 2014 Bartek Fabiszewski
 * http://www.fabiszewski.net
 *
 * This file is part of libmobi.
 * Licensed under LGPL, either version 3, or any later.
 * See <http://www.gnu.org/licenses/>
 */

#ifndef mobi_config_h
#define mobi_config_h

#ifdef _WIN32
# include <direct.h>
# include <malloc.h>
# define _ALLOCA _malloca
#else
# include <alloca.h>
# define _ALLOCA(size) alloca(size)
#endif

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#endif
