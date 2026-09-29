## Copyright 2026 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# Installs an unpacked pre-built package into DST, keeping files another
# component installed there already: the packages bundle their dependencies,
# which the superbuild installs first, thus first writer wins. Files of the
# previous install (MANIFEST) are removed first, so version bumps replace them.
#
# cmake -DSRC=<dir> -DDST=<dir> -DMANIFEST=<file> -P <this file>

cmake_minimum_required(VERSION 3.10)

if (EXISTS "${MANIFEST}")
  file(STRINGS "${MANIFEST}" PREVIOUS)
  foreach (FILE IN LISTS PREVIOUS)
    file(REMOVE "${DST}/${FILE}")
  endforeach()
endif()

file(GLOB_RECURSE FILES RELATIVE "${SRC}" "${SRC}/*")
set(INSTALLED "")
foreach (FILE IN LISTS FILES)
  if (NOT EXISTS "${DST}/${FILE}" AND NOT IS_SYMLINK "${DST}/${FILE}")
    get_filename_component(DIR "${FILE}" DIRECTORY)
    file(COPY "${SRC}/${FILE}" DESTINATION "${DST}/${DIR}")
    list(APPEND INSTALLED "${FILE}")
  endif()
endforeach()

string(REPLACE ";" "\n" INSTALLED "${INSTALLED}")
file(WRITE "${MANIFEST}" "${INSTALLED}\n")
