#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <string>
#include <sstream>

typedef uint8_t BYTE;

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <fcntl.h>
#  include <tchar.h>
#  define strcasecmp _stricmp
#else
#  include <unistd.h>
   // On Unix, stdin/stdout are already in binary mode; these are no-ops.
#  define _setmode(fd, mode) (0)
#  define _fileno fileno
#  define _O_BINARY 0
#  define _O_TEXT   1
#  define _sntprintf snprintf
#  define _T(x) x
#endif
