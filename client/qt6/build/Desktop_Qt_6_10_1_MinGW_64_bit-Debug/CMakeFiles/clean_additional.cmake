# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\MillionaireClient_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\MillionaireClient_autogen.dir\\ParseCache.txt"
  "MillionaireClient_autogen"
  )
endif()
