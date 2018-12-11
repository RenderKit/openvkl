## ======================================================================== ##
## Copyright 2018 Intel Corporation                                         ##
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

option(BUILD_SCALAR_DRIVER "Build scalar driver" ON)
if (BUILD_SCALAR_DRIVER)

  ospray_create_library(volley_module_scalar_driver
    api/ScalarDriver.cpp
  LINK
    volley
  COMPONENT
    lib
  )

  target_include_directories(volley_module_scalar_driver
  PRIVATE
    ${VOLLEY_INCLUDE_DIRS}
  )

endif (BUILD_SCALAR_DRIVER)
