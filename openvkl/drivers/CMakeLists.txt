## ======================================================================== ##
## Copyright 2019 Intel Corporation                                         ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

# add all drivers
file(GLOB drivers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
foreach(driver ${drivers})
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${driver}/CMakeLists.txt)
    add_subdirectory(${driver})
  endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${driver}/CMakeLists.txt)
endforeach(driver ${drivers})
