#pragma once

// Disable checked iterators and secure STL iterator debugging to avoid
// debug-CRT-only references in mixed build configurations.
#ifndef _ITERATOR_DEBUG_LEVEL
#define _ITERATOR_DEBUG_LEVEL 0
#endif

#ifndef _SECURE_SCL
#define _SECURE_SCL 0
#endif

#ifndef _HAS_ITERATOR_DEBUGGING
#define _HAS_ITERATOR_DEBUGGING 0
#endif
