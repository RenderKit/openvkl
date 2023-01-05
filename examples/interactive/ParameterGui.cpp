// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ParameterGui.h"
#include "TransferFunctionWidget.h"

#ifdef OPENVKL_TESTING_CPU
#include "renderer/density_path_tracer/DensityPathTracer.h"
#include "renderer/density_path_tracer/DensityPathTracerIspc.h"
#endif

#ifdef OPENVKL_TESTING_GPU
#include "renderer/density_path_tracer/DensityPathTracerGpu.h"
#endif

#include "renderer/HitIteratorRenderer.h"
#include "renderer/IntervalIteratorDebug.h"
#include "renderer/RayMarchIteratorRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/Scene.h"
#include "renderer/Scheduler.h"

#include <algorithm>
#include <cstring>  // memset
#include <string>
#include <vector>

namespace openvkl {
  namespace examples {

    namespace imgui {

      inline bool inputText(const std::string &label, std::string &text)
      {
        constexpr size_t bufSize = 1024;
        char buf[bufSize];
        std::memset(buf, 0, bufSize);
        std::strncpy(buf, text.c_str(), bufSize - 1);
        const bool changed = ImGui::InputText(label.c_str(), buf, bufSize);
        if (changed) {
          text = buf;
        }
        return changed;
      }

      template <class T, class MapToLabel>
      inline bool comboBox(const std::string &label,
                           T &output,
                           const std::vector<T> &opts,
                           MapToLabel mapToString)
      {
        bool changed = false;

        if (ImGui::BeginCombo(label.c_str(), mapToString(output).c_str())) {
          for (const T &opt : opts) {
            bool isSelected = (output == opt);
            if (ImGui::Selectable(mapToString(opt).c_str(), &isSelected)) {
              changed = true;
              output  = opt;
            }
            // Scroll to item if selected!
            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }

        return changed;
      }

      inline bool comboBox(const std::string &label,
                           std::string &output,
                           const std::vector<std::string> &opts)
      {
        return comboBox<std::string>(
            label, output, opts, [](const std::string &s) { return s; });
      }

      inline bool floatVector(const std::string &id,
                              std::vector<float> &samples,
                              size_t minSize,
                              float minValue,
                              float maxValue)
      {
        bool changed = false;
        ImGui::PushID(id.c_str());
        {
          // First control: number of samples.
          int numSamplesInput = static_cast<int>(samples.size());
          if (ImGui::InputInt("##TS", &numSamplesInput)) {
            numSamplesInput = std::max<int>(0, numSamplesInput);
            const size_t numSamples =
                std::max<size_t>(static_cast<size_t>(numSamplesInput), minSize);
            changed = (samples.size() != numSamples);
            samples.resize(numSamples);
          }
          ImGui::PushID("Sliders");
          {
            const ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            std::vector<bool> mustDelete(samples.size(), false);
            size_t numDelete = 0;
            for (size_t i = 0; i < samples.size(); ++i) {
              ImGui::PushID(i);
              changed |= ImGui::SliderFloat("##timeSample",
                                            &samples[i],
                                            minValue,
                                            maxValue,
                                            "%.3f",
                                            flags);
              ImGui::PopID();
            }
            std::vector<float> newSamples;
            newSamples.reserve(samples.size());
            for (size_t i = 0; i < samples.size(); ++i) {
              if (!mustDelete[i]) {
                newSamples.push_back(samples[i]);
              } else {
                changed = true;
              }
            }
            using std::swap;
            swap(samples, newSamples);
          }
          ImGui::PopID();
        }
        ImGui::PopID();
        return changed;
      }

    }  // namespace imgui

    // -------------------------------------------------------------------------

    class DensityPathTracerGui : public ParameterGui
    {
     public:
      using P = DensityPathTracerParams;

      DensityPathTracerGui(const std::string &id, Versioned<P> *params)
          : id{id}, params{params}
      {
      }

      bool drawImpl(P &p)
      {
        bool changed = false;

        ImGui::PushID(id.c_str());

        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Motion")) {
          changed |= ImGui::Checkbox("Motion Blur", &p.motionBlur);
          ImGui::PushDisabled(!p.motionBlur);
          changed |= ImGui::SliderFloat("Shutter", &p.shutter, 0.f, 1.f);
          ImGui::PopDisabled();
          ImGui::TreePop();
        }
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Material")) {
          ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
          if (ImGui::TreeNode("Coefficients")) {
            changed |= ImGui::InputFloat("SigmaT Scale", &p.sigmaTScale);
            changed |= ImGui::InputFloat("SigmaS Scale", &p.sigmaSScale);
            ImGui::TreePop();
          }
          ImGui::TreePop();
        }
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Ambient Light")) {
          changed |= ImGui::InputFloat("Intensity", &p.ambientLightIntensity);
          ImGui::TreePop();
        }
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Light Transport")) {
          changed |=
              ImGui::SliderInt("Max Num Scatters", &p.maxNumScatters, 1, 128);
          changed |= ImGui::Checkbox("Show Bounding Box", &p.showBbox);
          ImGui::TreePop();
        }

        ImGui::PopID();

        return changed;
      }

      bool draw(const Scheduler &scheduler) override final
      {
        bool changed = false;

        scheduler.locked(*params, [&]() {
          if (drawImpl(**params)) {
            params->incrementVersion();
            changed = true;
          }
        });

        return changed;
      }

     private:
      std::string id;
      Versioned<P> *params{nullptr};
    };

    // -------------------------------------------------------------------------

    class HitIteratorRendererGui : public ParameterGui
    {
     public:
      using P = HitIteratorRendererParams;

      HitIteratorRendererGui(const std::string &id, Versioned<P> *params)
          : id{id}, params{params}
      {
      }

      bool drawImpl(P &p)
      {
        bool changed = false;

        ImGui::PushID(id.c_str());

        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Isovalues")) {
          constexpr size_t minSize = 1;
          constexpr float minValue = -1.f;
          constexpr float maxValue = 1.f;
          changed |= imgui::floatVector(
              "isovalues", p.isoValues, minSize, minValue, maxValue);
          ImGui::TreePop();
        }

        ImGui::PopID();

        return changed;
      }

      bool draw(const Scheduler &scheduler) override final
      {
        bool changed = false;

        scheduler.locked(*params, [&]() {
          if (drawImpl(**params)) {
            params->incrementVersion();
            changed = true;
          }
        });

        return changed;
      }

     private:
      std::string id;
      Versioned<P> *params{nullptr};
    };

    // -------------------------------------------------------------------------

    class RayMarchIteratorRendererGui : public ParameterGui
    {
     public:
      using P = RayMarchIteratorRendererParams;

      RayMarchIteratorRendererGui(const std::string &id, Versioned<P> *params)
          : id{id}, params{params}
      {
      }

      bool drawImpl(P &p)
      {
        bool changed = false;

        ImGui::PushID(id.c_str());

        changed |= ImGui::SliderFloat(
            "intervalResolutionHint", &p.intervalResolutionHint, 0.f, 1.f);
        changed |=
            ImGui::SliderFloat("Sampling rate", &p.samplingRate, 0.01f, 4.f);

        ImGui::PopID();

        return changed;
      }

      bool draw(const Scheduler &scheduler) override final
      {
        bool changed = false;

        scheduler.locked(*params, [&]() {
          if (drawImpl(**params)) {
            params->incrementVersion();
            changed = true;
          }
        });

        return changed;
      }

     private:
      std::string id;
      Versioned<P> *params{nullptr};
    };

    // -------------------------------------------------------------------------

    class IntervalIteratorDebugGui : public ParameterGui
    {
     public:
      using P = IntervalIteratorDebugParams;

      IntervalIteratorDebugGui(const std::string &id, Versioned<P> *params)
          : id{id}, params{params}
      {
      }

      bool drawImpl(P &p)
      {
        bool changed = false;

        ImGui::PushID(id.c_str());

        changed |= ImGui::SliderFloat(
            "intervalResolutionHint", &p.intervalResolutionHint, 0.f, 1.f);
        changed |=
            ImGui::SliderFloat("Color scale", &p.intervalColorScale, 1.f, 32.f);
        changed |=
            ImGui::SliderFloat("Opacity", &p.intervalOpacity, 0.01f, 1.f);
        changed |= ImGui::Checkbox("First interval only", &p.firstIntervalOnly);
        changed |=
            ImGui::Checkbox("Show interval borders", &p.showIntervalBorders);

        ImGui::PopID();

        return changed;
      }

      bool draw(const Scheduler &scheduler) override final
      {
        bool changed = false;

        scheduler.locked(*params, [&]() {
          if (drawImpl(**params)) {
            params->incrementVersion();
            changed = true;
          }
        });

        return changed;
      }

     private:
      std::string id;
      Versioned<P> *params{nullptr};
    };

    // -------------------------------------------------------------------------

    ParameterGui::~ParameterGui() {}

    template <class G, class R>
    inline void mrg(const std::string &id,
                    Renderer *renderer,
                    std::unique_ptr<ParameterGui> &gui)
    {
      if (!gui) {
        R *r = dynamic_cast<R *>(renderer);
        if (r) {
          gui = rkcommon::make_unique<G>(id, &r->getGuiParams());
        }
      }
    }

    std::unique_ptr<ParameterGui> ParameterGui::makeRendererGui(
        Renderer *renderer)
    {
      std::unique_ptr<ParameterGui> gui;

#ifdef OPENVKL_TESTING_GPU
      mrg<DensityPathTracerGui, DensityPathTracerGpu>(
          "density_pathtracer_gpu", renderer, gui);
#endif

#ifdef OPENVKL_TESTING_CPU
      mrg<DensityPathTracerGui, DensityPathTracer>(
          "density_pathtracer", renderer, gui);

      mrg<DensityPathTracerGui, DensityPathTracerIspc>(
          "density_pathtracer_ispc", renderer, gui);

      mrg<HitIteratorRendererGui, HitIteratorRenderer>(
          "hit_iterator", renderer, gui);
      mrg<HitIteratorRendererGui, HitIteratorRendererIspc>(
          "hit_iterator_ispc", renderer, gui);

      mrg<RayMarchIteratorRendererGui, RayMarchIteratorRenderer>(
          "ray_march_iterator", renderer, gui);
      mrg<RayMarchIteratorRendererGui, RayMarchIteratorRendererIspc>(
          "ray_march_iterator_ispc", renderer, gui);

      mrg<IntervalIteratorDebugGui, IntervalIteratorDebug>(
          "interval_iterator_debug", renderer, gui);
      mrg<IntervalIteratorDebugGui, IntervalIteratorDebugIspc>(
          "interval_iterator_debug_ispc", renderer, gui);
#endif
      return gui;
    }

    // -------------------------------------------------------------------------

    class RendererParamsGui : public ParameterGui
    {
     public:
      using P = RendererParams;

      RendererParamsGui(Scene *scene)
          : scene{scene},
            tfWidget{scene->rendererParams->transferFunction.valueRange}
      {
      }

      bool drawImpl(P &p)
      {
        bool changed = false;

        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::CollapsingHeader("Volume", 0)) {
          changed |= ImGui::SliderFloat("Time", &p.time, 0.f, 1.f);
          const unsigned numAttributes = scene->volume.getNumAttributes();
          const unsigned maxAttribute =
              std::max<unsigned>(numAttributes, 1) - 1;
          p.attributeIndex =
              std::min<int>(p.attributeIndex, static_cast<int>(maxAttribute));
          ImGui::PushDisabled(numAttributes <= 1);
          changed |= ImGui::SliderInt(
              "Attribute Index", &p.attributeIndex, 0, maxAttribute);
          ImGui::PopDisabled();

          if (tfWidget.updateUI()) {
            p.transferFunction =
                TransferFunction(tfWidget.getValueRange(),
                                 tfWidget.getSampledColorsAndOpacities());
            changed = true;
          }
        }

        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::CollapsingHeader("Framebuffer", 0)) {
          changed |= ImGui::Checkbox("Fixed Size", &p.fixedFramebufferSize);
          ImGui::PushDisabled(!p.fixedFramebufferSize);
          changed |= ImGui::InputInt2("##Size", &p.framebufferSize.x);

          changed |=
              ImGui::Checkbox("Restrict Pixel Range", &p.restrictPixelRange);
          ImGui::PushDisabled(!p.restrictPixelRange);
          changed |= ImGui::DragIntRange2("X",
                                          &p.pixelRange.lower.x,
                                          &p.pixelRange.upper.x,
                                          1.f,
                                          0,
                                          INT_MAX);
          changed |= ImGui::DragIntRange2("Y",
                                          &p.pixelRange.lower.y,
                                          &p.pixelRange.upper.y,
                                          1.f,
                                          0,
                                          INT_MAX);
          ImGui::PopDisabled();
          ImGui::PopDisabled();
        }
        return changed;
      }

      bool draw(const Scheduler &scheduler) override final
      {
        bool changed = false;

        Versioned<P> &params = scene->rendererParams;

        scheduler.locked(params, [&]() {
          if (drawImpl(*params)) {
            params.incrementVersion();
            changed = true;
          }
        });

        return changed;
      }

     private:
      Scene *scene{nullptr};
      TransferFunctionWidget tfWidget;
    };

    std::unique_ptr<ParameterGui> ParameterGui::makeRendererParamsGui(
        Scene *scene)
    {
      return rkcommon::make_unique<RendererParamsGui>(scene);
    }

    // -------------------------------------------------------------------------

    class SceneParamsGui : public ParameterGui
    {
     public:
      SceneParamsGui(Scene *scene) : scene{scene} {}

      bool drawImpl(VolumeParams &p)
      {
        bool needsUpdate =
            false;  // indicates that the user clicked the button.
        bool &changed = volumeChanged;

        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::CollapsingHeader("Volume", 0)) {
          ImGui::PushDisabled(!changed);
          needsUpdate = ImGui::Button(
              "Rebuild Volume", ImVec2(ImGui::GetContentRegionAvail().x, 0));
          ImGui::PopDisabled();

          ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
          if (ImGui::TreeNode("Type")) {
            const bool wasSpherical = (p.volumeType == "structuredSpherical");
            changed |=
                imgui::comboBox("##T", p.volumeType, p.supportedVolumeTypes());
            const bool isSpherical = (p.volumeType == "structuredSpherical");
            ImGui::TreePop();

            // Structured spherical volumes have their own way of parametrizing
            // the volume! Make sure changing this in the GUI works.
            if (wasSpherical != isSpherical) {
              p.gridOrigin.x = rkcommon::math::nan;
              p.generateGridTransform();
            }
          }

          ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
          if (ImGui::TreeNode("Source")) {
            const std::vector<std::string> &supSources =
                p.supportedSources(p.volumeType);
            // Not all volume types support the same sources, so reset if an
            // invalid source is currently selected.
            if (std::find(supSources.begin(), supSources.end(), p.source) ==
                supSources.end()) {
              p.source = supSources[0];
            }
            changed |= imgui::comboBox(
                "Field Source##FieldSource", p.source, supSources);

            const bool isFile = (p.source == "file");
            ImGui::PushDisabled(!isFile);
            changed |= imgui::inputText("Filename", p.filename);
            ImGui::PopDisabled();

            if (isFile) {
              ImGui::PushDisabled(p.volumeType != "vdb");
              changed |= imgui::inputText("Field##FieldInFile", p.fieldInFile);
              ImGui::PopDisabled();
            } else {
              const std::vector<std::string> &supFields =
                  p.supportedFields(p.volumeType);
              // Not all volume types support the same sources, so reset if an
              // invalid source is currently selected.
              if (std::find(supFields.begin(), supFields.end(), p.field) ==
                  supFields.end()) {
                p.field = supFields[0];
              }
              changed |=
                  imgui::comboBox("Field##ProcField", p.field, supFields);
            }

            const std::vector<VKLDataType> &voxelTypes =
                p.supportedVoxelTypes(p.volumeType);
            ImGui::PushDisabled(voxelTypes.empty() || p.source == "file");
            {
              if (!voxelTypes.empty()) {
                // Fix voxel type if needed.
                if (std::find(voxelTypes.begin(),
                              voxelTypes.end(),
                              p.voxelType) == voxelTypes.end()) {
                  p.voxelType = voxelTypes[0];
                }
              }

              changed |= imgui::comboBox<VKLDataType>(
                  "Voxel Type", p.voxelType, voxelTypes, voxelTypeToString);
            }
            ImGui::PopDisabled();

            ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
            if (ImGui::TreeNode("Transform")) {
              ImGui::PushDisabled(p.volumeType == "particle" ||
                                  p.source == "file");
              {
                bool transformChanged = false;
                transformChanged |=
                    ImGui::InputInt3("Grid Dimensions", &(p.dimensions.x));
                transformChanged |=
                    ImGui::InputFloat3("Grid Origin", &(p.gridOrigin.x));
                transformChanged |=
                    ImGui::InputFloat3("Grid Spacing", &(p.gridSpacing.x));
                if (transformChanged) {
                  p.generateGridTransform();
                }
                changed |= transformChanged;
              }
              ImGui::PopDisabled();
              ImGui::TreePop();
            }

            ImGui::PushDisabled(p.volumeType != "particle");
            {
              changed |= ImGui::InputInt("Particles", &p.numParticles);
            }
            ImGui::PopDisabled();

            ImGui::PushDisabled(!(p.volumeType == "structuredRegular" ||
                                  p.volumeType == "vdb"));
            {
              changed |=
                  ImGui::Checkbox("Multiple Attributes", &p.multiAttribute);
            }
            ImGui::PopDisabled();

            ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
            if (ImGui::TreeNode("Motion")) {
              ImGui::PushDisabled(!(p.volumeType == "structuredRegular" ||
                                    p.volumeType == "vdb"));
              {
                if (ImGui::RadioButton("Temporally Constant",
                                       !(p.motionBlurStructured ||
                                         p.motionBlurUnstructured))) {
                  changed                  = true;
                  p.motionBlurStructured   = false;
                  p.motionBlurUnstructured = false;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Structured", p.motionBlurStructured)) {
                  changed                  = true;
                  p.motionBlurStructured   = true;
                  p.motionBlurUnstructured = false;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Unstructured",
                                       p.motionBlurUnstructured)) {
                  changed                  = true;
                  p.motionBlurStructured   = false;
                  p.motionBlurUnstructured = true;
                }

                ImGui::SetNextItemOpen(p.motionBlurStructured,
                                       ImGuiCond_FirstUseEver);
                if (ImGui::TreeNode("Structured Timesteps")) {
                  ImGui::PushDisabled(!p.motionBlurStructured);
                  {
                    int timesteps = p.motionBlurStructuredNumTimesteps;
                    if (ImGui::InputInt("##TS", &timesteps)) {
                      p.motionBlurStructuredNumTimesteps = static_cast<uint8_t>(
                          std::min<int>(std::max<int>(1, timesteps), 0xFF));
                      changed = true;
                    }
                  }
                  ImGui::PopDisabled();
                  ImGui::TreePop();
                }

                ImGui::SetNextItemOpen(p.motionBlurUnstructured,
                                       ImGuiCond_FirstUseEver);
                if (ImGui::TreeNode("Unstructured Timesteps")) {
                  ImGui::PushDisabled(!p.motionBlurUnstructured);
                  constexpr size_t minSize = 2;
                  constexpr float minValue = 0;
                  constexpr float maxValue = 1.f;
                  imgui::floatVector("unstructuredTimeSteps",
                                     p.motionBlurUnstructuredTimeSamples,
                                     minSize,
                                     minValue,
                                     maxValue);
                  ImGui::PopDisabled();
                  ImGui::TreePop();
                }
              }
              ImGui::PopDisabled();
              ImGui::TreePop();
            }

            ImGui::TreePop();
          }

          ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
          if (ImGui::TreeNode("Background")) {
            // Make sure we use the correct NaN.
            bool bgIsUndefined = std::isnan(p.background);
            if (ImGui::Checkbox("Background Undefined", &bgIsUndefined)) {
              changed = true;
              if (bgIsUndefined) {
                p.background = VKL_BACKGROUND_UNDEFINED;
              } else {
                p.background = 0.f;
              }
            }
            ImGui::PushDisabled(bgIsUndefined);
            {
              changed |= ImGui::InputFloat("Background Value", &p.background);
            }
            ImGui::PopDisabled();
            ImGui::TreePop();
          }
        }

        return changed && needsUpdate;
      }

      bool drawImpl(const VolumeParams &volumeParams, SamplerParams &p)
      {
        bool changed = false;

        const std::vector<VKLFilter> &supFilters =
            p.supportedFilters(volumeParams.volumeType);

        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::CollapsingHeader("Sampler", 0)) {
          ImGui::PushDisabled(volumeParams.volumeType != "vdb");
          changed |= ImGui::SliderInt("Maximum Sampling Depth##maxSamplingDepth",
                                     &p.maxSamplingDepth,
                                     0,
                                     VKL_VDB_NUM_LEVELS-1);
          ImGui::PopDisabled();
          ImGui::PushDisabled(supFilters.empty());
          changed |= imgui::comboBox("Sampling Filter##samplingFilter",
                                     p.filter,
                                     supFilters,
                                     filterToString);
          changed |= imgui::comboBox("Gradient Filter##samplingFilter",
                                     p.gradientFilter,
                                     supFilters,
                                     filterToString);
          ImGui::PopDisabled();
        }

        return changed;
      }

      bool draw(const Scheduler & /*scheduler*/) override final
      {
        bool changed = false;

        Volume &volume             = scene->volume;
        VolumeParams &volumeParams = volume.getVolumeParams();

        if (drawImpl(volumeParams)) {
          volume.setVolumeDirty();
          changed = true;
        }

        SamplerParams &samplerParams = volume.getSamplerParams();
        if (drawImpl(volumeParams, samplerParams)) {
          volume.setSamplerDirty();
          changed = true;
        }

        return changed;
      }

     private:
      Scene *scene{nullptr};
      bool volumeChanged{false};
    };

    std::unique_ptr<ParameterGui> ParameterGui::makeSceneParamsGui(Scene *scene)
    {
      return rkcommon::make_unique<SceneParamsGui>(scene);
    }

  }  // namespace examples
}  // namespace openvkl
