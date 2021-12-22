# - Try to find FBXAPI.
# Once done, this will define
#
#  FBXAPI_FOUND - system has all required freebox libraries

#  FBXBUS_FOUND - system has fbxbus and fbxevent
#  FBXBUS_INCLUDE_DIRS - the fbxbus include directories
#  FBXBUS_LIBRARIES - link these to use fbxbus
#

find_package(PkgConfig)
include(FindPackageHandleStandardArgs)

pkg_check_modules(FBXBUS fbxbus fbxevent fbxevent-gsource)
find_package_handle_standard_args(FBXBUS
    REQUIRED_VARS FBXBUS_FOUND
)

if (FBXBUS_FOUND)
    set(FBXAPI_FOUND TRUE)
else ()
    set(FBXAPI_FOUND FALSE)
endif ()
