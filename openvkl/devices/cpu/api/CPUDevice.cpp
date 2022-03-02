// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CPUDevice.h"
#include "../common/Data.h"
#include "../common/export_util.h"
#include "../common/ispc_isa.h"
#include "../iterator/Iterator.h"
#include "../observer/Observer.h"
#include "../sampler/Sampler.h"
#include "../volume/Volume.h"
#include "CPUDevice_ispc.h"

namespace openvkl {
  namespace cpu_device {

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
    // CPUDevice //////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    bool CPUDevice<W>::supportsWidth(int width)
    {
      return width == W || width == 4 || width == 8 || width == 16;
    }

    template <int W>
    int CPUDevice<W>::getNativeSIMDWidth()
    {
      return CALL_ISPC(ISPC_getProgramCount);
    }

    template <int W>
    void CPUDevice<W>::commit()
    {
      Device::commit();

      VKLISPCTarget target =
          static_cast<VKLISPCTarget>(CALL_ISPC(ISPC_getTarget));

      postLogMessage(this, VKL_LOG_DEBUG)
          << "CPU device instantiated with width: " << getNativeSIMDWidth()
          << ", ISA: " << stringForVKLISPCTarget(target);
    }

    template <int W>
    void CPUDevice<W>::commit(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->commit();
    }

    template <int W>
    void CPUDevice<W>::release(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->refDec();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Data ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLData CPUDevice<W>::newData(size_t numItems,
                                  VKLDataType dataType,
                                  const void *source,
                                  VKLDataCreationFlags dataCreationFlags,
                                  size_t byteStride)
    {
      Data *data =
          new Data(numItems, dataType, source, dataCreationFlags, byteStride);
      return (VKLData)data;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Observer ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLObserver CPUDevice<W>::newObserver(VKLVolume volume, const char *type)
    {
      auto &object          = referenceFromHandle<Volume<W>>(volume);
      Observer<W> *observer = object.newObserver(type);
      return (VKLObserver)observer;
    }

    template <int W>
    VKLObserver CPUDevice<W>::newObserver(VKLSampler sampler, const char *type)
    {
      auto &object          = referenceFromHandle<Sampler<W>>(sampler);
      Observer<W> *observer = object.newObserver(type);
      return (VKLObserver)observer;
    }

    template <int W>
    const void *CPUDevice<W>::mapObserver(VKLObserver observer)
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      return observerObject.map();
    }

    template <int W>
    void CPUDevice<W>::unmapObserver(VKLObserver observer)
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      observerObject.unmap();
    }

    template <int W>
    VKLDataType CPUDevice<W>::getObserverElementType(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      return observerObject.getElementType();
    }

    template <int W>
    size_t CPUDevice<W>::getObserverElementSize(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      return observerObject.getElementSize();
    }

    template <int W>
    size_t CPUDevice<W>::getObserverNumElements(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      return observerObject.getNumElements();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Interval iterator //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLIntervalIteratorContext CPUDevice<W>::newIntervalIteratorContext(
        VKLSampler sampler)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      return (VKLIntervalIteratorContext)samplerObject
          .getIntervalIteratorFactory()
          .newContext(samplerObject);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLHitIteratorContext CPUDevice<W>::newHitIteratorContext(
        VKLSampler sampler)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      return (VKLHitIteratorContext)samplerObject.getHitIteratorFactory()
          .newContext(samplerObject);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parameters /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    void CPUDevice<W>::setBool(VKLObject object, const char *name, const bool b)
    {
      setObjectParam(object, name, VKL_BOOL, &b);
    }

    template <int W>
    void CPUDevice<W>::set1f(VKLObject object, const char *name, const float x)
    {
      setObjectParam(object, name, VKL_FLOAT, &x);
    }

    template <int W>
    void CPUDevice<W>::set1i(VKLObject object, const char *name, const int x)
    {
      setObjectParam(object, name, VKL_INT, &x);
    }

    template <int W>
    void CPUDevice<W>::setVec3f(VKLObject object,
                                const char *name,
                                const vec3f &v)
    {
      setObjectParam(object, name, VKL_VEC3F, &v);
    }

    template <int W>
    void CPUDevice<W>::setVec3i(VKLObject object,
                                const char *name,
                                const vec3i &v)
    {
      setObjectParam(object, name, VKL_VEC3I, &v);
    }

    template <int W>
    void CPUDevice<W>::setObject(VKLObject object,
                                 const char *name,
                                 VKLObject setObject)
    {
      setObjectParam(object, name, VKL_OBJECT, &setObject);
    }

    template <int W>
    void CPUDevice<W>::setString(VKLObject object,
                                 const char *name,
                                 const std::string &s)
    {
      setObjectParam(object, name, VKL_STRING, s.c_str());
    }

    template <int W>
    void CPUDevice<W>::setVoidPtr(VKLObject object, const char *name, void *v)
    {
      setObjectParam(object, name, VKL_VOID_PTR, &v);
    }

    template <int W>
    void CPUDevice<W>::setObjectParam(VKLObject object,
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

    ///////////////////////////////////////////////////////////////////////////
    // Sampler ////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLSampler CPUDevice<W>::newSampler(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return (VKLSampler)volumeObject.newSampler();
    }

#define __define_computeSampleN(WIDTH)                                      \
  template <int W>                                                          \
  void CPUDevice<W>::computeSample##WIDTH(                                  \
      const int *valid,                                                     \
      VKLSampler sampler,                                                   \
      const vvec3fn<WIDTH> &objectCoordinates,                              \
      float *samples,                                                       \
      unsigned int attributeIndex,                                          \
      const float *times)                                                   \
  {                                                                         \
    computeSampleAnyWidth<WIDTH>(                                           \
        valid, sampler, objectCoordinates, samples, attributeIndex, times); \
  }

    __define_computeSampleN(4);
    __define_computeSampleN(8);
    __define_computeSampleN(16);

#undef __define_computeSampleN

    // support a fast path for scalar sampling
    template <int W>
    void CPUDevice<W>::computeSample1(const int *valid,
                                      VKLSampler sampler,
                                      const vvec3fn<1> &objectCoordinates,
                                      float *sample,
                                      unsigned int attributeIndex,
                                      const float *time)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      vfloatn<1> timeW(time, 1);
      vfloatn<1> sampleW;
      samplerObject.computeSample(
          objectCoordinates, sampleW, attributeIndex, timeW);
      *sample = sampleW[0];
    }

    template <int W>
    void CPUDevice<W>::computeSampleN(VKLSampler sampler,
                                      unsigned int N,
                                      const vvec3fn<1> *objectCoordinates,
                                      float *samples,
                                      unsigned int attributeIndex,
                                      const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      samplerObject.computeSampleN(
          N, objectCoordinates, samples, attributeIndex, times);
    }

#define __define_computeSampleMN(WIDTH)              \
  template <int W>                                   \
  void CPUDevice<W>::computeSampleM##WIDTH(          \
      const int *valid,                              \
      VKLSampler sampler,                            \
      const vvec3fn<WIDTH> &objectCoordinates,       \
      float *samples,                                \
      unsigned int M,                                \
      const unsigned int *attributeIndices,          \
      const float *times)                            \
  {                                                  \
    computeSampleMAnyWidth<WIDTH>(valid,             \
                                  sampler,           \
                                  objectCoordinates, \
                                  samples,           \
                                  M,                 \
                                  attributeIndices,  \
                                  times);            \
  }

    __define_computeSampleMN(4);
    __define_computeSampleMN(8);
    __define_computeSampleMN(16);

#undef __define_computeSampleMN

    // support a fast path for scalar sampling
    template <int W>
    void CPUDevice<W>::computeSampleM1(const int *valid,
                                       VKLSampler sampler,
                                       const vvec3fn<1> &objectCoordinates,
                                       float *samples,
                                       unsigned int M,
                                       const unsigned int *attributeIndices,
                                       const float *time)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      vfloatn<1> timeW(time, 1);
      samplerObject.computeSampleM(
          objectCoordinates, samples, M, attributeIndices, timeW);
    }

    template <int W>
    void CPUDevice<W>::computeSampleMN(VKLSampler sampler,
                                       unsigned int N,
                                       const vvec3fn<1> *objectCoordinates,
                                       float *samples,
                                       unsigned int M,
                                       const unsigned int *attributeIndices,
                                       const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      samplerObject.computeSampleMN(
          N, objectCoordinates, samples, M, attributeIndices, times);
    }

#define __define_computeGradientN(WIDTH)                                      \
  template <int W>                                                            \
  void CPUDevice<W>::computeGradient##WIDTH(                                  \
      const int *valid,                                                       \
      VKLSampler sampler,                                                     \
      const vvec3fn<WIDTH> &objectCoordinates,                                \
      vvec3fn<WIDTH> &gradients,                                              \
      unsigned int attributeIndex,                                            \
      const float *times)                                                     \
  {                                                                           \
    computeGradientAnyWidth<WIDTH>(                                           \
        valid, sampler, objectCoordinates, gradients, attributeIndex, times); \
  }

    __define_computeGradientN(1);
    __define_computeGradientN(4);
    __define_computeGradientN(8);
    __define_computeGradientN(16);

#undef __define_computeGradientN

    template <int W>
    void CPUDevice<W>::computeGradientN(VKLSampler sampler,
                                        unsigned int N,
                                        const vvec3fn<1> *objectCoordinates,
                                        vvec3fn<1> *gradients,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      samplerObject.computeGradientN(
          N, objectCoordinates, gradients, attributeIndex, times);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Volume /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLVolume CPUDevice<W>::newVolume(const char *type)
    {
      // warn for deprecated snake case volume types
      std::string typeStr(type);

      if (typeStr.find("_") != std::string::npos) {
        postLogMessage(this, VKL_LOG_WARNING)
            << "volume type name '" << typeStr
            << "' may be deprecated; volume type names are now camelCase (no "
               "underscores)";
      }

      std::stringstream ss;
      ss << type << "_" << W;

      return (VKLVolume)Volume<W>::createInstance(this, ss.str());
    }

    template <int W>
    box3f CPUDevice<W>::getBoundingBox(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getBoundingBox();
    }

    template <int W>
    unsigned int CPUDevice<W>::getNumAttributes(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getNumAttributes();
    }

    template <int W>
    range1f CPUDevice<W>::getValueRange(VKLVolume volume,
                                        unsigned int attributeIndex)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getValueRange(attributeIndex);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Private methods ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    CPUDevice<W>::computeSampleAnyWidth(const int *valid,
                                        VKLSampler sampler,
                                        const vvec3fn<OW> &objectCoordinates,
                                        float *samples,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);
      vfloatn<W> tW(times, OW);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      ocW.fill_inactive_lanes(validW);
      tW.fill_inactive_lanes(validW);

      vfloatn<W> samplesW;

      samplerObject.computeSampleV(validW, ocW, samplesW, attributeIndex, tW);

      for (int i = 0; i < OW; i++)
        samples[i] = samplesW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    CPUDevice<W>::computeSampleAnyWidth(const int *valid,
                                        VKLSampler sampler,
                                        const vvec3fn<OW> &objectCoordinates,
                                        float *samples,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<W> tW(times, W);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      vfloatn<W> samplesW;

      samplerObject.computeSampleV(
          validW, objectCoordinates, samplesW, attributeIndex, tW);

      for (int i = 0; i < W; i++)
        samples[i] = samplesW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    CPUDevice<W>::computeSampleAnyWidth(const int *valid,
                                        VKLSampler sampler,
                                        const vvec3fn<OW> &objectCoordinates,
                                        float *samples,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<OW> tOW(times, OW);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);
        vfloatn<W> tW  = tOW.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        ocW.fill_inactive_lanes(validW);
        tW.fill_inactive_lanes(validW);

        vfloatn<W> samplesW;

        samplerObject.computeSampleV(validW, ocW, samplesW, attributeIndex, tW);

        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          samples[i] = samplesW[i - packIndex * W];
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    CPUDevice<W>::computeSampleMAnyWidth(const int *valid,
                                         VKLSampler sampler,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);
      vfloatn<W> tW(times, OW);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      ocW.fill_inactive_lanes(validW);
      tW.fill_inactive_lanes(validW);

      float *samplesW = (float *)alloca(M * W * sizeof(float));

      samplerObject.computeSampleMV(
          validW, ocW, samplesW, M, attributeIndices, tW);

      for (unsigned int a = 0; a < M; a++) {
        for (int i = 0; i < OW; i++) {
          samples[a * OW + i] = samplesW[a * W + i];
        }
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    CPUDevice<W>::computeSampleMAnyWidth(const int *valid,
                                         VKLSampler sampler,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<W> timesW(times, W);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      samplerObject.computeSampleMV(
          validW, objectCoordinates, samples, M, attributeIndices, timesW);
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    CPUDevice<W>::computeSampleMAnyWidth(const int *valid,
                                         VKLSampler sampler,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<OW> tOW(times, OW);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);
        vfloatn<W> tW  = tOW.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        ocW.fill_inactive_lanes(validW);
        tW.fill_inactive_lanes(validW);

        float *samplesW = (float *)alloca(M * W * sizeof(float));

        samplerObject.computeSampleMV(
            validW, ocW, samplesW, M, attributeIndices, tW);

        for (unsigned int a = 0; a < M; a++) {
          for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
            samples[a * OW + i] = samplesW[a * W + (i - packIndex * W)];
        }
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    CPUDevice<W>::computeGradientAnyWidth(const int *valid,
                                          VKLSampler sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);
      vfloatn<W> tW(times, OW);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      ocW.fill_inactive_lanes(validW);
      tW.fill_inactive_lanes(validW);

      vvec3fn<W> gradientsW;

      samplerObject.computeGradientV(
          validW, ocW, gradientsW, attributeIndex, tW);

      for (int i = 0; i < OW; i++) {
        gradients.x[i] = gradientsW.x[i];
        gradients.y[i] = gradientsW.y[i];
        gradients.z[i] = gradientsW.z[i];
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    CPUDevice<W>::computeGradientAnyWidth(const int *valid,
                                          VKLSampler sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<W> tW(times, W);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      samplerObject.computeGradientV(
          validW, objectCoordinates, gradients, attributeIndex, tW);
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    CPUDevice<W>::computeGradientAnyWidth(const int *valid,
                                          VKLSampler sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<OW> tOW(times, OW);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);
        vfloatn<W> tW  = tOW.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        ocW.fill_inactive_lanes(validW);
        tW.fill_inactive_lanes(validW);

        vvec3fn<W> gradientsW;

        samplerObject.computeGradientV(
            validW, ocW, gradientsW, attributeIndex, tW);

        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++) {
          gradients.x[i] = gradientsW.x[i - packIndex * W];
          gradients.y[i] = gradientsW.y[i - packIndex * W];
          gradients.z[i] = gradientsW.z[i - packIndex * W];
        }
      }
    }

    VKL_REGISTER_DEVICE(CPUDevice<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_cpu_, VKL_TARGET_WIDTH))

  }  // namespace cpu_device
}  // namespace openvkl

extern "C" OPENVKL_DLLEXPORT void CONCAT1(openvkl_init_module_cpu_device_,
                                          VKL_TARGET_WIDTH)()
{
}
