﻿set(CMAKE_SYSROOT /usr/i686-pc-cygwin/sys-root)
set(CMAKE_RC_COMPILER /bin/i686-pc-cygwin-windres)
set(CMAKE_C_COMPILER i686-pc-cygwin-gcc)
set(CMAKE_CXX_COMPILER i686-pc-cygwin-g++)
if(NOT EXISTS ${CMAKE_SYSROOT})
  unset(CMAKE_RC_COMPILER)
  unset(CMAKE_SYSROOT)
endif()