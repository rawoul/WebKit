#.rst
# FindFbxcdm
# -------------
#
# Finds the Fbxcdm library.
#
# This will define the following variables:
#
# ``FBXCDM_FOUND``
#     True if the requested version of Fbxcdm was found
# ``FBXCDM_INCLUDE_DIRS``
#     The Fbxcdm include directories
# ``FBXCDM_LIBRARIES``
#     The linker libraries needed to use the Fbxcdm library

find_package(PkgConfig)
pkg_check_modules(PC_FBXCDM fbxcdm fbxcdmdecrypt)

find_path(FBXCDM_INCLUDE_DIR
    NAMES fbxcdm.h
    HINTS ${PC_FBXCDM_INCLUDEDIR}
    ${PC_FBXCDM_INCLUDE_DIRS}
    PATH_SUFFIXES ""
)

find_path(FBXCDMDECRYPT_INCLUDE_DIR
    NAMES libfbxcdmdecrypt.h
    HINTS ${PC_FBXCDMDECRYPT_INCLUDEDIR}
    ${PC_FBXCDMDECRYPT_INCLUDE_DIRS}
    PATH_SUFFIXES ""
)

find_library(FBXCDM_LIBRARY
    NAMES fbxcdm
    HINTS ${PC_FBXCDM_LIBDIR}
    ${PC_FBXCDM_LIBRARY_DIRS}
)

find_library(FBXCDMDECRYPT_LIBRARY
    NAMES fbxcdmdecrypt
    HINTS ${PC_FBXCDMDECRYPT_LIBDIR}
    ${PC_FBXCDMDECRYPT_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Fbxcdm
  FOUND_VAR FBXCDM_FOUND
  REQUIRED_VARS FBXCDM_LIBRARY FBXCDM_INCLUDE_DIR FBXCDMDECRYPT_LIBRARY FBXCDMDECRYPT_INCLUDE_DIR
)

if (FBXCDM_FOUND)
  set(FBXCDM_LIBRARIES ${FBXCDM_LIBRARY} ${FBXCDMDECRYPT_LIBRARY})
  set(FBXCDM_INCLUDE_DIRS ${FBXCDM_INCLUDE_DIR} ${FBXCDMDECRYPT_INCLUDE_DIR})
endif ()

mark_as_advanced(FBXCDM_LIBRARY FBXCDMDECRYPT_LIBRARY FBXCDM_INCLUDE_DIR FBXCDMDECRYPT_INCLUDE_DIR)

include(FeatureSummary)
set_package_properties(Fbxcdm PROPERTIES
    DESCRIPTION "Fbxcdm DRM framework"
)
