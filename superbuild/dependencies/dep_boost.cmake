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


if(WIN32)
  set(BOOST_CONF "bootstrap")
  set(BOOST_URL  "https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.zip")
  set(BOOST_HASH SHA256=8c20440aaba21dd963c0f7149517445f50c62ce4eb689df2b5544cc89e6e621e)
else()
  set(BOOST_CONF "./bootstrap.sh")
  set(BOOST_URL  "https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.gz")
  set(BOOST_HASH SHA256=c66e88d5786f2ca4dbebb14e06b566fb642a1a6947ad8cc9091f9f445134143f)
endif()

ExternalProject_Add(${COMPONENT_NAME}
  PREFIX ${COMPONENT_NAME}
  DOWNLOAD_DIR ${COMPONENT_NAME}
  STAMP_DIR ${COMPONENT_NAME}/stamp
  SOURCE_DIR ${COMPONENT_NAME}/src
  BINARY_DIR ${COMPONENT_NAME}/src
  URL ${BOOST_URL}
  URL_HASH ${BOOST_HASH}
  CONFIGURE_COMMAND ${BOOST_CONF} --with-libraries=system,iostreams,regex --prefix=${COMPONENT_PATH}
  BUILD_COMMAND ./b2
  INSTALL_COMMAND ./b2 install
  BUILD_ALWAYS OFF
)

set(BOOST_ROOT ${COMPONENT_PATH})

