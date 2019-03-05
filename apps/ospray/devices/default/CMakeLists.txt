
include(ispc)

# ISPC-side include directories
include_directories_ispc(${OSPRAY_INCLUDE_DIRS})
include_directories_ispc(${EMBREE_INCLUDE_DIRS})
include_directories_ispc(${PROJECT_SOURCE_DIR}/volley/include)

get_target_property(OSPRAY_MODULE_ISPC_INCLUDE_DIRECTORIES
                    ospray::ospray_module_ispc
                    INTERFACE_INCLUDE_DIRECTORIES)

include_directories_ispc(${OSPRAY_MODULE_ISPC_INCLUDE_DIRECTORIES})


ospray_add_library(ospray_module_ispc_volley SHARED
  render/SimpleNativeVolumeRenderer.cpp
  render/SimpleNativeVolumeRenderer.ispc

  render/VolleyRenderer.cpp
  render/VolleyRenderer.ispc
)

# TODO: hack to get proper ospcommon include directory (i.e. to support <ospcommon/...> paths)
get_target_property(OSPRAY_COMMON_INCLUDE_DIRECTORY
                    ospray::ospray_common
                    INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(ospray_module_ispc_volley PRIVATE
  "${OSPRAY_COMMON_INCLUDE_DIRECTORY}/../")

target_include_directories(ospray_module_ispc_volley PRIVATE
  ${EMBREE_INCLUDE_DIRS})

# TODO: hack for pulled in -lembree (with no full path)
target_link_directories(ospray_module_ispc_volley PUBLIC
                        ${EMBREE_ROOT_DIR}/lib)

target_link_libraries(ospray_module_ispc_volley
  PUBLIC
  ospray::ospray_module_ispc
  ospray::ospray_common
  volley
  ${EMBREE_LIBRARIES}
)
