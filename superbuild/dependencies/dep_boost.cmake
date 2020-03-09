## ======================================================================== ##
## Copyright 2020 Intel Corporation                                         ##
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

set(COMPONENT_NAME boost)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

set(BOOST_CONF "./bootstrap.sh")
set(BOOST_BUILD "./b2")
set(BOOST_LINK "shared")

ExternalProject_Add(${COMPONENT_NAME}
  PREFIX ${COMPONENT_NAME}
  DOWNLOAD_DIR ${COMPONENT_NAME}
  STAMP_DIR ${COMPONENT_NAME}/stamp
  SOURCE_DIR ${COMPONENT_NAME}/src
  BINARY_DIR ${COMPONENT_NAME}/src
  URL ${BOOST_URL}
  URL_HASH SHA256=${BOOST_HASH}
  CONFIGURE_COMMAND ${BOOST_CONF}
  BUILD_COMMAND ${BOOST_BUILD} -d0 --with-system --with-iostreams --with-regex --layout=system
    --prefix=${COMPONENT_PATH} variant=release threading=multi address-model=64
    link=${BOOST_LINK} architecture=x86 install
  INSTALL_COMMAND ""
  BUILD_ALWAYS OFF
)

mark_as_advanced(BOOST_CONF)
mark_as_advanced(BOOST_BUILD)
mark_as_advanced(BOOST_LINK)

set(BOOST_PATH ${COMPONENT_PATH})

