# Taken from: https://github.com/BIC-MNI/minc-toolkit/blob/master/cmake-modules/FindNETPBM.cmake
# License: GPLv3
#
# - Find NETPBM
# Find the native NETPBM includes and library
# This module defines
#  NETPBM_INCLUDE_DIR, where to find jpeglib.h, etc.
#  NETPBM_LIBRARIES, the libraries needed to use NETPBM.
#  NETPBM_FOUND, If false, do not try to use NETPBM.
# also defined, but not for general use are
#  NETPBM_LIBRARY, where to find the NETPBM library.

FIND_PATH(NETPBM_INCLUDE_DIR pam.h
/usr/local/include/netpbm
/usr/local/include
/usr/include
)

SET(NETPBM_NAMES ${NETPBM_NAMES} netpbm)
FIND_LIBRARY(NETPBM_LIBRARY
  NAMES ${NETPBM_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

IF (NETPBM_LIBRARY AND NETPBM_INCLUDE_DIR)
    SET(NETPBM_LIBRARIES ${NETPBM_LIBRARY})
    SET(NETPBM_FOUND "YES")
ELSE (NETPBM_LIBRARY AND NETPBM_INCLUDE_DIR)
  SET(NETPBM_FOUND "NO")
ENDIF (NETPBM_LIBRARY AND NETPBM_INCLUDE_DIR)


IF (NETPBM_FOUND)
   IF (NOT NETPBM_FIND_QUIETLY)
      MESSAGE(STATUS "Found NETPBM: ${NETPBM_LIBRARIES}")
   ENDIF (NOT NETPBM_FIND_QUIETLY)
ELSE (NETPBM_FOUND)
   IF (NETPBM_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find NETPBM library")
   ENDIF (NETPBM_FIND_REQUIRED)
ENDIF (NETPBM_FOUND)

# Deprecated declarations.
SET (NATIVE_NETPBM_INCLUDE_PATH ${NETPBM_INCLUDE_DIR} )
GET_FILENAME_COMPONENT (NATIVE_NETPBM_LIB_PATH ${NETPBM_LIBRARY} PATH)

MARK_AS_ADVANCED(
  NETPBM_LIBRARY
  NETPBM_INCLUDE_DIR
)
