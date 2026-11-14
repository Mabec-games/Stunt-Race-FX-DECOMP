# toolchain.cmake: the Linux game for one CPU against Ubuntu 22.04's C library
# (builder/linux/sysroot.py), whatever Linux builds it, so it runs on Linux
# from 2022 on (glibc 2.35).
#   -DCMAKE_TOOLCHAIN_FILE=builder/linux/toolchain.cmake -DSRFX_ARCH=amd64|arm64 -DSRFX_SYSROOT=DIR
#   -DSRFX_CFLAGS=... -DSRFX_LDFLAGS=...   (build-linux.sh: the sysroot's headers
#   and libraries ahead of the build machine's own, which gcc searches first)
set(CMAKE_SYSTEM_NAME Linux)
if(SRFX_ARCH STREQUAL "arm64")
  set(CMAKE_SYSTEM_PROCESSOR aarch64)
  set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
else()
  set(CMAKE_SYSTEM_PROCESSOR x86_64)
  set(CMAKE_C_COMPILER gcc)
endif()
set(CMAKE_SYSROOT ${SRFX_SYSROOT})
set(CMAKE_C_FLAGS_INIT "${SRFX_CFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${SRFX_LDFLAGS}")
set(CMAKE_FIND_ROOT_PATH ${SRFX_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# the variables pass on to try_compile
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES SRFX_ARCH SRFX_SYSROOT SRFX_CFLAGS SRFX_LDFLAGS)
