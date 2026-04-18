# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\EagleLibrary_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\EagleLibrary_autogen.dir\\ParseCache.txt"
  "EagleLibrary_autogen"
  )
endif()
