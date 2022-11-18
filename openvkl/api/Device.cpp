// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Device.h"
#include <sstream>
#include "../common/Data.h"
#include "../common/ManagedObject.h"
#include "ispc_util_ispc.h"
#include "rkcommon/tasking/tasking_system_init.h"
#include "rkcommon/utility/StringManip.h"
#include "rkcommon/utility/getEnvVar.h"

namespace openvkl {

  static ObjectFactory<Device, VKL_DEVICE> g_devicesFactory;

  namespace api {

    // helper functions
    template <std::ostream &OS>
    static inline void installLogCallback(Device &device)
    {
      device.logUserData = nullptr;
      device.logCallback = [](void * /*usrData*/, const char *msg) {
        OS << msg;
      };
    }

    template <std::ostream &OS>
    static inline void installErrorCallback(Device &device)
    {
      device.errorUserData = nullptr;
      device.errorCallback =
          [](void * /*usrData*/, VKLError e, const char *msg) {
            OS << "OPENVKL ERROR [" << e << "]: " << msg << std::endl;
          };
    }

    // Device definitions

    Device::Device()
    {
      // setup default logging functions; after the device is instantiated, they
      // may be overridden via vklDeviceSet*Callback().
      installLogCallback<std::cout>(*this);
      installErrorCallback<std::cerr>(*this);
    }

    Device *Device::createDevice(const std::string &deviceName)
    {
      const std::string cpuDeviceName("cpu");

      // special case for CPU device selection
      if (deviceName.find(cpuDeviceName) != std::string::npos) {
        int requestedDeviceWidth = 0;

        const int nativeMaxIspcWidth = ispc::get_programCount();

        if (deviceName == cpuDeviceName) {
          // the generic "cpu" device was selected

          // update requested device width if the user set the env var
          auto OPENVKL_CPU_DEVICE_DEFAULT_WIDTH =
              utility::getEnvVar<int>("OPENVKL_CPU_DEVICE_DEFAULT_WIDTH");
          requestedDeviceWidth = OPENVKL_CPU_DEVICE_DEFAULT_WIDTH.value_or(0);

          if (requestedDeviceWidth) {
            LogMessageStream(nullptr, VKL_LOG_DEBUG)
                << "application requested CPU device width "
                << requestedDeviceWidth
                << " via OPENVKL_CPU_DEVICE_DEFAULT_WIDTH";
          } else {
            // the user did not set the env var, use the runtime native
            // (maximum) ISPC vector width
            requestedDeviceWidth = nativeMaxIspcWidth;

            LogMessageStream(nullptr, VKL_LOG_DEBUG)
                << "will use ISPC device native maximum width "
                << requestedDeviceWidth;
          }

        } else if (deviceName.find(cpuDeviceName + "_") != std::string::npos &&
                   deviceName.size() > cpuDeviceName.size() + 1) {
          // the user chose a specific width for the CPU device, e.g.
          // cpu_[4,8,16]. verify that device is legal on this system.
          std::string specifiedWidthStr =
              deviceName.substr(deviceName.find("_") + 1, deviceName.size());

          try {
            requestedDeviceWidth = std::stoi(specifiedWidthStr);

            LogMessageStream(nullptr, VKL_LOG_DEBUG)
                << "application requested ISPC device width "
                << requestedDeviceWidth << "via device name " << deviceName;
          } catch (const std::invalid_argument &ia) {
            LogMessageStream(nullptr, VKL_LOG_ERROR)
                << "could not parse requested ISPC device width for name: "
                << deviceName;
          }
        }

        // verify legality of the determined width
        if (requestedDeviceWidth <= 0 ||
            requestedDeviceWidth > nativeMaxIspcWidth) {
          std::stringstream ss;
          ss << "device " << deviceName
             << " cannot run on the system (native SIMD width: "
             << nativeMaxIspcWidth
             << ", requested SIMD width: " << requestedDeviceWidth << ")";

          throw std::runtime_error(ss.str().c_str());
        }

        std::stringstream ss;
        ss << cpuDeviceName << "_" << requestedDeviceWidth;

        return g_devicesFactory.createInstance(nullptr, ss.str());
      }
      return g_devicesFactory.createInstance(nullptr, deviceName);
    }

    void Device::registerDevice(const std::string &type, FactoryDeviceFcn<Device> f)
    {
      g_devicesFactory.registerDevice(type, f);
    }

    void Device::commit()
    {
      // log level
      logLevel = VKLLogLevel(getParam<int>("logLevel", LOG_LEVEL_DEFAULT));

      // environment variable takes precedence
      auto OPENVKL_LOG_LEVEL = utility::lowerCase(
          utility::getEnvVar<std::string>("OPENVKL_LOG_LEVEL")
              .value_or(std::string()));

      if (!OPENVKL_LOG_LEVEL.empty()) {
        if (OPENVKL_LOG_LEVEL == "debug") {
          logLevel = VKL_LOG_DEBUG;
        } else if (OPENVKL_LOG_LEVEL == "info") {
          logLevel = VKL_LOG_INFO;
        } else if (OPENVKL_LOG_LEVEL == "warning") {
          logLevel = VKL_LOG_WARNING;
        } else if (OPENVKL_LOG_LEVEL == "error") {
          logLevel = VKL_LOG_ERROR;
        } else if (OPENVKL_LOG_LEVEL == "none") {
          logLevel = VKL_LOG_NONE;
        } else {
          LogMessageStream(this, VKL_LOG_ERROR)
              << "unknown OPENVKL_LOG_LEVEL env value; must be debug, info, "
                 "warning, error or none";
        }
      }

      // log output
      auto OPENVKL_LOG_OUTPUT =
          utility::getEnvVar<std::string>("OPENVKL_LOG_OUTPUT");

      auto dst =
          OPENVKL_LOG_OUTPUT.value_or(getParam<std::string>("logOutput", ""));

      if (dst == "cout")
        installLogCallback<std::cout>(*this);
      else if (dst == "cerr")
        installLogCallback<std::cerr>(*this);
      else if (dst == "none") {
        logCallback = [](void *, const char *) {};
        logUserData = nullptr;
      }

      // error output
      auto OPENVKL_ERROR_OUTPUT =
          utility::getEnvVar<std::string>("OPENVKL_ERROR_OUTPUT");

      dst = OPENVKL_ERROR_OUTPUT.value_or(
          getParam<std::string>("errorOutput", ""));

      if (dst == "cout")
        installErrorCallback<std::cout>(*this);
      else if (dst == "cerr")
        installErrorCallback<std::cerr>(*this);
      else if (dst == "none") {
        errorCallback = [](void *, VKLError, const char *) {};
        errorUserData = nullptr;
      }

      // threads
      auto OPENVKL_THREADS = utility::getEnvVar<int>("OPENVKL_THREADS");
      int numThreads =
          OPENVKL_THREADS.value_or(getParam<int>("numThreads", -1));

      // flush to zero / denormals are zero
      auto OPENVKL_FLUSH_DENORMALS =
          utility::getEnvVar<int>("OPENVKL_FLUSH_DENORMALS");
      bool flushDenormals =
          OPENVKL_FLUSH_DENORMALS.value_or(getParam<int>("flushDenormals", 1));

      tasking::initTaskingSystem(numThreads, flushDenormals);

      committed = true;
    }

    bool Device::isCommitted()
    {
      return committed;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parameter setting helpers //////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    using SetParamFcn = void(VKLObject, const char *, const void *);

    template <typename T>
    static void setParamOnObject(VKLObject _obj, const char *p, const T &v)
    {
      auto *obj = (ManagedObject *)_obj;
      obj->setParam(p, v);
    }

#define declare_param_setter(TYPE)                                           \
  {                                                                          \
    VKLTypeFor<TYPE>::value, [](VKLObject o, const char *p, const void *v) { \
      setParamOnObject(o, p, *(TYPE *)v);                                    \
    }                                                                        \
  }

#define declare_param_setter_object(TYPE)                                    \
  {                                                                          \
    VKLTypeFor<TYPE>::value, [](VKLObject o, const char *p, const void *v) { \
      ManagedObject *obj = *(TYPE *)v;                                       \
      setParamOnObject(o, p, obj);                                           \
    }                                                                        \
  }

#define declare_param_setter_string(TYPE)                                    \
  {                                                                          \
    VKLTypeFor<TYPE>::value, [](VKLObject o, const char *p, const void *v) { \
      const char *str = (const char *)v;                                     \
      setParamOnObject(o, p, std::string(str));                              \
    }                                                                        \
  }

    static std::map<VKLDataType, std::function<SetParamFcn>> setParamFcns = {
        declare_param_setter(void *),
        declare_param_setter(bool),
        declare_param_setter_object(openvkl::ManagedObject *),
        declare_param_setter_object(openvkl::Data *),
        declare_param_setter_string(char *),
        declare_param_setter_string(const char *),
        declare_param_setter_string(const char[]),
        declare_param_setter(char),
        declare_param_setter(unsigned char),
        declare_param_setter(rkcommon::math::vec2uc),
        declare_param_setter(rkcommon::math::vec3uc),
        declare_param_setter(rkcommon::math::vec4uc),
        declare_param_setter(short),
        declare_param_setter(unsigned short),
        declare_param_setter(int32_t),
        declare_param_setter(rkcommon::math::vec2i),
        declare_param_setter(rkcommon::math::vec3i),
        declare_param_setter(rkcommon::math::vec4i),
        declare_param_setter(uint32_t),
        declare_param_setter(rkcommon::math::vec2ui),
        declare_param_setter(rkcommon::math::vec3ui),
        declare_param_setter(rkcommon::math::vec4ui),
        declare_param_setter(int64_t),
        declare_param_setter(rkcommon::math::vec2l),
        declare_param_setter(rkcommon::math::vec3l),
        declare_param_setter(rkcommon::math::vec4l),
        declare_param_setter(uint64_t),
        declare_param_setter(rkcommon::math::vec2ul),
        declare_param_setter(rkcommon::math::vec3ul),
        declare_param_setter(rkcommon::math::vec4ul),
        declare_param_setter(float),
        declare_param_setter(rkcommon::math::vec2f),
        declare_param_setter(rkcommon::math::vec3f),
        declare_param_setter(rkcommon::math::vec4f),
        declare_param_setter(double),
        declare_param_setter(rkcommon::math::box1i),
        declare_param_setter(rkcommon::math::box2i),
        declare_param_setter(rkcommon::math::box3i),
        declare_param_setter(rkcommon::math::box4i),
        declare_param_setter(rkcommon::math::box1f),
        declare_param_setter(rkcommon::math::box2f),
        declare_param_setter(rkcommon::math::box3f),
        declare_param_setter(rkcommon::math::box4f),
        declare_param_setter(rkcommon::math::linear2f),
        declare_param_setter(rkcommon::math::linear3f),
        declare_param_setter(rkcommon::math::affine2f),
        declare_param_setter(rkcommon::math::affine3f),
    };

#undef declare_param_setter

    ///////////////////////////////////////////////////////////////////////////
    // Parameters /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void Device::setBool(VKLObject object, const char *name, const bool b)
    {
      setObjectParam(object, name, VKL_BOOL, &b);
    }

    void Device::set1f(VKLObject object, const char *name, const float x)
    {
      setObjectParam(object, name, VKL_FLOAT, &x);
    }

    void Device::set1i(VKLObject object, const char *name, const int x)
    {
      setObjectParam(object, name, VKL_INT, &x);
    }

    void Device::setVec3f(VKLObject object, const char *name, const vec3f &v)
    {
      setObjectParam(object, name, VKL_VEC3F, &v);
    }

    void Device::setVec3i(VKLObject object, const char *name, const vec3i &v)
    {
      setObjectParam(object, name, VKL_VEC3I, &v);
    }

    void Device::setObject(VKLObject object,
                           const char *name,
                           VKLObject setObject)
    {
      setObjectParam(object, name, VKL_OBJECT, &setObject);
    }

    void Device::setString(VKLObject object,
                           const char *name,
                           const std::string &s)
    {
      setObjectParam(object, name, VKL_STRING, s.c_str());
    }

    void Device::setVoidPtr(VKLObject object, const char *name, void *v)
    {
      setObjectParam(object, name, VKL_VOID_PTR, &v);
    }

    void Device::setObjectParam(VKLObject object,
                                const char *name,
                                VKLDataType dataType,
                                const void *mem)
    {
      if (!setParamFcns.count(dataType)) {
        throw std::runtime_error("cannot set parameter " + std::string(name) +
                                 " for given data type");
      }

      setParamFcns[dataType](object, name, mem);
    }

    // new param setters //////////////////////////////////////////////////////

    void Device::setBool(APIObject object2, const char *name, const bool b)
    {
      VKLObject object = static_cast<VKLObject>(object2.host);
      setObjectParam(object, name, VKL_BOOL, &b);
    }

    void Device::set1f(APIObject object2, const char *name, const float x)
    {
      VKLObject object = static_cast<VKLObject>(object2.host);
      setObjectParam(object, name, VKL_FLOAT, &x);
    }

    void Device::set1i(APIObject object2, const char *name, const int x)
    {
      VKLObject object = static_cast<VKLObject>(object2.host);
      setObjectParam(object, name, VKL_INT, &x);
    }

    void Device::setVec3f(APIObject object2, const char *name, const vec3f &v)
    {
      VKLObject object = static_cast<VKLObject>(object2.host);
      setObjectParam(object, name, VKL_VEC3F, &v);
    }

    void Device::setVec3i(APIObject object2, const char *name, const vec3i &v)
    {
      VKLObject object = static_cast<VKLObject>(object2.host);
      setObjectParam(object, name, VKL_VEC3I, &v);
    }

    void Device::setObject(APIObject object2,
                           const char *name,
                           VKLObject setObject)
    {
      VKLObject object = static_cast<VKLObject>(object2.host);
      setObjectParam(object, name, VKL_OBJECT, &setObject);
    }

    void Device::setString(APIObject object2,
                           const char *name,
                           const std::string &s)
    {
      VKLObject object = static_cast<VKLObject>(object2.host);
      setObjectParam(object, name, VKL_STRING, s.c_str());
    }

    void Device::setVoidPtr(APIObject object2, const char *name, void *v)
    {
      VKLObject object = static_cast<VKLObject>(object2.host);
      setObjectParam(object, name, VKL_VOID_PTR, &v);
    }

    void Device::setObjectParam(APIObject object2,
                                const char *name,
                                VKLDataType dataType,
                                const void *mem)
    {
      VKLObject object = static_cast<VKLObject>(object2.host);
      if (!setParamFcns.count(dataType)) {
        throw std::runtime_error("cannot set parameter " + std::string(name) +
                                 " for given data type");
      }

      setParamFcns[dataType](object, name, mem);
    }

  }  // namespace api
}  // namespace openvkl
