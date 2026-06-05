#ifndef VLC_USM_VLC_COMPAT_H
#define VLC_USM_VLC_COMPAT_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#ifndef MODULE_STRING
#define MODULE_STRING "usm"
#endif

#ifndef N_
#define N_(text) (text)
#endif

#endif
