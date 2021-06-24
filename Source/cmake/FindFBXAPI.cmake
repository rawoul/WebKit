# - Try to find FBXAPI.
# Once done, this will define
#
#  FBXAPI_FOUND - system has all required freebox libraries

#  FBXBUS_FOUND - system has fbxbus and fbxevent
#  FBXBUS_INCLUDE_DIRS - the fbxbus include directories
#  FBXBUS_LIBRARIES - link these to use fbxbus
#
#  FBXCONNMAN_FOUND - system has fbxconnman
#  FBXCONNMAN_INCLUDE_DIRS - the fbxconnman include directories
#  FBXCONNMAN_LIBRARIES - link these to use fbxconnman
#

find_package(PkgConfig)
include(FindPackageHandleStandardArgs)

pkg_check_modules(FBXBUS fbxbus fbxevent fbxevent-gsource)
find_package_handle_standard_args(FBXBUS
    REQUIRED_VARS FBXBUS_FOUND
)

pkg_check_modules(FBXCONNMAN fbxconnman)
find_package_handle_standard_args(FBXCONNMAN
    REQUIRED_VARS FBXCONNMAN_FOUND
)

if (FBXBUS_FOUND AND FBXCONNMAN_FOUND)
    set(FBXAPI_FOUND TRUE)
else ()
    set(FBXAPI_FOUND FALSE)
endif ()
