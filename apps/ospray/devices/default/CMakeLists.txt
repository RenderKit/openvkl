
include(ispc)

# ISPC-side include directories
include_directories_ispc(${OSPRAY_INCLUDE_DIRS})
include_directories_ispc(${EMBREE_INCLUDE_DIRS})
include_directories_ispc(${PROJECT_SOURCE_DIR}/openvkl/include)

get_target_property(OSPRAY_MODULE_ISPC_INCLUDE_DIRECTORIES
                    ospray::ospray_module_ispc
                    INTERFACE_INCLUDE_DIRECTORIES)

include_directories_ispc(${OSPRAY_MODULE_ISPC_INCLUDE_DIRECTORIES})


ospray_add_library(ospray_module_ispc_openvkl SHARED
  render/SimpleNativeVolumeRenderer.cpp
  render/SimpleNativeVolumeRenderer.ispc

  render/SimpleVKLRenderer.cpp
  render/SimpleVKLRenderer.ispc

  render/VKLRayIteratorRenderer.cpp
  render/VKLRayIteratorRenderer.ispc

  render/VKLRayIteratorSurfaceRenderer.cpp
  render/VKLRayIteratorSurfaceRenderer.ispc

  render/VKLRayIteratorVolumeRenderer.cpp
  render/VKLRayIteratorVolumeRenderer.ispc

  render/VKLPathTracer.cpp
  render/VKLPathTracer.ispc
)

# TODO: hack to get proper ospcommon include directory (i.e. to support <ospcommon/...> paths)
get_target_property(OSPRAY_COMMON_INCLUDE_DIRECTORY
                    ospray::ospray_common
                    INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(ospray_module_ispc_openvkl PRIVATE
  "${OSPRAY_COMMON_INCLUDE_DIRECTORY}/../")

target_include_directories(ospray_module_ispc_openvkl PRIVATE
  ${EMBREE_INCLUDE_DIRS})

# TODO: hack for pulled in -lembree (with no full path)
target_link_directories(ospray_module_ispc_openvkl PUBLIC
                        ${EMBREE_ROOT_DIR}/lib)

target_link_libraries(ospray_module_ispc_openvkl
  PUBLIC
  ospray::ospray_module_ispc
  ospray::ospray_common
  openvkl
  ${EMBREE_LIBRARIES}
)
