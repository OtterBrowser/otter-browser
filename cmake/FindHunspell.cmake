# - Try to find Hunspell
# Once done this will define
#
#  Hunspell_FOUND - system has Hunspell
#  Hunspell_INCLUDE_DIR - the Hunspell include directory
#  Hunspell_LIBRARIES - The libraries needed to use Hunspell
#  Hunspell_DEFINITIONS - Compiler switches required for using Hunspell
#  Hunspell::Hunspell - Library target

if (Hunspell_INCLUDE_DIR AND Hunspell_LIBRARIES)
    # Already in cache, be silent
    set(Hunspell_FIND_QUIETLY TRUE)
endif (Hunspell_INCLUDE_DIR AND Hunspell_LIBRARIES)

find_path(Hunspell_INCLUDE_DIR hunspell/hunspell.hxx)
find_library(Hunspell_LIBRARIES NAMES hunspell-1.7 hunspell-1.6 hunspell-1.5 hunspell)

# handle the QUIETLY and REQUIRED arguments and set Hunspell_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Hunspell DEFAULT_MSG Hunspell_LIBRARIES Hunspell_INCLUDE_DIR)

mark_as_advanced(Hunspell_INCLUDE_DIR Hunspell_LIBRARIES)

if (Hunspell_FOUND)
    add_library(Hunspell::Hunspell INTERFACE IMPORTED)
    set_target_properties(Hunspell::Hunspell PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "${Hunspell_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${Hunspell_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${Hunspell_LIBRARIES}"
    )
endif()
