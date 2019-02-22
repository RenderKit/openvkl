
include(ispc)

macro(add_executable_ispc name)
  set(ISPC_SOURCES "")
  set(OTHER_SOURCES "")
  foreach(src ${ARGN})
    get_filename_component(ext ${src} EXT)
    if (ext STREQUAL ".ispc")
      set(ISPC_SOURCES ${ISPC_SOURCES} ${src})
    else()
      set(OTHER_SOURCES ${OTHER_SOURCES} ${src})
    endif ()
  endforeach()
  ospray_ispc_compile(${ISPC_SOURCES})
  add_executable(${name} ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES})
endmacro()

include_directories_ispc(${PROJECT_SOURCE_DIR}/volley/include)

add_executable_ispc(vlyTutorialISPC
  vlyTutorialISPC.ispc
  vlyTutorialISPC.cpp
)

target_link_libraries(vlyTutorialISPC PRIVATE volley)

install(TARGETS vlyTutorialISPC
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
