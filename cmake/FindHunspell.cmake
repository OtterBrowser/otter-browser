# - Try to find Hunspell
# Once done this will define
#
#  Hunspell_FOUND - system has Hunspell
#  Hunspell_INCLUDE_DIR - the Hunspell include directory
#  Hunspell_LIBRARIES - The libraries needed to use Hunspell
#  Hunspell_DEFINITIONS - Compiler switches required for using Hunspell

IF (Hunspell_INCLUDE_DIR AND Hunspell_LIBRARIES)
  # Already in cache, be silent
  SET(Hunspell_FIND_QUIETLY TRUE)
ENDIF (Hunspell_INCLUDE_DIR AND Hunspell_LIBRARIES)

FIND_PATH(Hunspell_INCLUDE_DIR hunspell/hunspell.hxx)
FIND_LIBRARY(Hunspell_LIBRARIES NAMES hunspell-1.7 hunspell-1.6 hunspell-1.5 hunspell)

# handle the QUIETLY and REQUIRED arguments and set Hunspell_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Hunspell DEFAULT_MSG Hunspell_LIBRARIES Hunspell_INCLUDE_DIR)

MARK_AS_ADVANCED(Hunspell_INCLUDE_DIR Hunspell_LIBRARIES)
