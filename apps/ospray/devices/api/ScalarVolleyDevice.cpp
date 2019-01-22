// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray core
#include <ospray/SDK/api/Device.h>

#include "../common/Data.h"
#include "../common/Model.h"
#include "../fb/FrameBuffer.h"
#include "../render/VolumeRenderer.h"
#include "../transferFunction/TransferFunction.h"
#include "../volume/BlockBrickedVolume.h"
#include "../volume/StructuredVolume.h"
#include "../volume/VolleyVolumeWrapper.h"

namespace ospray {
  namespace scalar_volley_device {

    struct ScalarDevice : public api::Device
    {
      ScalarDevice()
      {
        application_objects.reserve(1000);
      };

      void commit(OSPObject handle) override
      {
        referenceFromHandle(handle).commit();
      }

      OSPData newData(size_t nitems,
                      OSPDataType format,
                      const void *init,
                      int flags) override
      {
        if (format == OSP_OBJECT) {
          auto data = createRegisteredObject<ObjectData>(nitems, format, init);
          return getHandleForAPI<OSPData>(data);
        } else {
          auto data =
              createRegisteredObject<RawData>(nitems, format, flags, init);
          return getHandleForAPI<OSPData>(data);
        }
      }

      OSPLight newLight(const char *) override
      {
        return createPlaceholderObject<OSPLight>();
      }

      OSPTexture newTexture(const char *) override
      {
        return createPlaceholderObject<OSPTexture>();
      }

      OSPMaterial newMaterial(OSPRenderer, const char *) override
      {
        return createPlaceholderObject<OSPMaterial>();
      }

      OSPMaterial newMaterial(const char *renderer_type,
                              const char *material_type) override
      {
        return createPlaceholderObject<OSPMaterial>();
      }

      OSPCamera newCamera(const char *) override
      {
        auto camera = createRegisteredObject<Camera>();
        return getHandleForAPI<OSPCamera>(camera);
      }

      OSPPixelOp newPixelOp(const char *) override
      {
        return createPlaceholderObject<OSPPixelOp>();
      }

      OSPRenderer newRenderer(const char *) override
      {
        auto renderer = createRegisteredObject<VolumeRenderer>();
        return getHandleForAPI<OSPRenderer>(renderer);
      }

      OSPModel newModel() override
      {
        auto model = createRegisteredObject<Model>();
        return getHandleForAPI<OSPModel>(model);
      }

      OSPGeometry newGeometry(const char *) override
      {
        return createPlaceholderObject<OSPGeometry>();
      }

      void removeParam(OSPObject _object, const char *name) override
      {
        referenceFromHandle(_object).removeParam(name);
      }

      void setString(OSPObject _object,
                     const char *paramName,
                     const char *value) override
      {
        referenceFromHandle(_object).setParam(paramName, std::string(value));
      }

      void setVoidPtr(OSPObject _object,
                      const char *paramName,
                      void *value) override
      {
        referenceFromHandle(_object).setParam(paramName, value);
      }

      void setInt(OSPObject _object,
                  const char *paramName,
                  const int value) override
      {
        referenceFromHandle(_object).setParam(paramName, value);
      }

      void setBool(OSPObject _object,
                   const char *paramName,
                   const bool value) override
      {
        referenceFromHandle(_object).setParam(paramName, value);
      }

      void setFloat(OSPObject _object,
                    const char *paramName,
                    const float value) override
      {
        referenceFromHandle(_object).setParam(paramName, value);
      }

      void setVec2f(OSPObject _object,
                    const char *paramName,
                    const vec2f &value) override
      {
        referenceFromHandle(_object).setParam(paramName, value);
      }

      void setVec3f(OSPObject _object,
                    const char *paramName,
                    const vec3f &value) override
      {
        referenceFromHandle(_object).setParam(paramName, value);
      }

      void setVec4f(OSPObject _object,
                    const char *paramName,
                    const vec4f &value) override
      {
        referenceFromHandle(_object).setParam(paramName, value);
      }

      void setVec2i(OSPObject _object,
                    const char *paramName,
                    const vec2i &value) override
      {
        referenceFromHandle(_object).setParam(paramName, value);
      }

      void setVec3i(OSPObject _object,
                    const char *paramName,
                    const vec3i &value) override
      {
        referenceFromHandle(_object).setParam(paramName, value);
      }

      void setObject(OSPObject _object,
                     const char *paramName,
                     OSPObject _value) override
      {
        auto &obj = referenceFromHandle(_object);
        auto &val = referenceFromHandle(_value);
        obj.setParam(paramName, val.shared_from_this());
      }

      void setMaterial(OSPGeometry _geometry, OSPMaterial _material) override
      {
        // TODO
      }

      void addGeometry(OSPModel _model, OSPGeometry _geometry) override
      {
        // TODO
      }

      float renderFrame(OSPFrameBuffer _fb,
                        OSPRenderer _renderer,
                        const uint32 /*fbChannelFlags*/) override
      {
        auto &fb = referenceFromHandle<FrameBuffer>(_fb);
        auto &r  = referenceFromHandle<Renderer>(_renderer);
        fb.beginFrame();
        r.renderFrame(fb);
        fb.endFrame();
        return 0.f;
      }

      void frameBufferClear(OSPFrameBuffer _fb,
                            const uint32 /*fbChannelFlags*/) override
      {
        auto &fb = referenceFromHandle<FrameBuffer>(_fb);
        fb.clear(vec4f(0.f));
      }

      OSPFrameBuffer frameBufferCreate(const vec2i &size,
                                       const OSPFrameBufferFormat /*mode*/,
                                       const uint32 /*channels*/) override
      {
        auto fb = createRegisteredObject<FrameBuffer>(size, vec2i(32));
        return getHandleForAPI<OSPFrameBuffer>(fb);
      }

      const void *frameBufferMap(
          OSPFrameBuffer _fb, const OSPFrameBufferChannel /*channel*/) override
      {
        auto &fb = referenceFromHandle<FrameBuffer>(_fb);
        return fb.mapColorBuffer();
      }

      void frameBufferUnmap(const void *, OSPFrameBuffer) override
      {
        // no-op
      }

      int loadModule(const char *name) override
      {
        return loadLocalModule(name);
      }

      void removeGeometry(OSPModel /*_model*/,
                          OSPGeometry /*_geometry*/) override
      {
        // TODO
      }

      void addVolume(OSPModel _model, OSPVolume _volume) override
      {
        auto &model  = referenceFromHandle<Model>(_model);
        auto &volume = referenceFromHandle<Volume>(_volume);
        model.addVolume(volume);
      }

      void removeVolume(OSPModel _model, OSPVolume _volume) override
      {
        auto &model  = referenceFromHandle<Model>(_model);
        auto &volume = referenceFromHandle<Volume>(_volume);
        model.removeVolume(volume);
      }

      int setRegion(OSPVolume _volume,
                    const void *source,
                    const vec3i &index,
                    const vec3i &count) override
      {
        auto &volume = referenceFromHandle<Volume>(_volume);
        return volume.setRegion(source, index, count);
      }

      void setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op) override
      {
        // TODO
      }

      OSPVolume newVolume(const char *_type) override
      {
        std::string type = _type;
        bool useVolleyVolume   = type == "volley::simple_procedural_volume";

        if (useVolleyVolume) {
          auto volume = createRegisteredObject<VolleyVolumeWrapper>();
          return getHandleForAPI<OSPVolume>(volume);
        } else {
          throw std::runtime_error("only Volley volumes supported by this device");
          return nullptr;
        }
      }

      OSPTransferFunction newTransferFunction(const char *) override
      {
        auto tf = createRegisteredObject<TransferFunction>();
        return getHandleForAPI<OSPTransferFunction>(tf);
      }

      void release(OSPObject _obj) override
      {
        auto *addressOfObjectToRemove = &referenceFromHandle(_obj);

        auto handle_matches = [&](const ManagedPtr &m) {
          return m.get() == addressOfObjectToRemove;
        };

        auto &o = application_objects;
        o.erase(std::remove_if(o.begin(), o.end(), handle_matches), o.end());
      }

      virtual void commit() override
      {
        Device::commit();
      }

     private:
      // Helper functions //

      template <typename T, typename... Args>
      std::shared_ptr<T> createRegisteredObject(Args &&... args)
      {
        auto object = std::make_shared<T>(std::forward<Args>(args)...);
        application_objects.push_back(object);
        return object;
      }

      template <typename HANDLE_T, typename OBJECT_T>
      HANDLE_T getHandleForAPI(const std::shared_ptr<OBJECT_T> &object)
      {
        return (HANDLE_T)object.get();
      }

      template <typename HANDLE_T>
      HANDLE_T createPlaceholderObject()
      {
        return getHandleForAPI<HANDLE_T>(
            createRegisteredObject<ManagedObject>());
      }

      template <typename OBJECT_T = ManagedObject,
                typename HANDLE_T = OSPObject>
      OBJECT_T &referenceFromHandle(HANDLE_T handle)
      {
        return *((OBJECT_T *)handle);
      }

      // Data //

      std::vector<ManagedPtr> application_objects;
    };

    OSP_REGISTER_DEVICE(ScalarDevice, scalar_volley_device);

  }  // namespace scalar_volley_device
}  // namespace ospray
