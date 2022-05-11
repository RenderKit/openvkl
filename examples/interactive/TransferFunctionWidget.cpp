// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TransferFunctionWidget.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace openvkl {
  namespace examples {

    namespace help {

      template <typename T>
      int find_idx(const std::vector<T> &A, float p)
      {
        auto found = std::upper_bound(
            A.begin(), A.end(), T(p), [](const T &a, const T &b) {
              return a.x < b.x;
            });
        return std::distance(A.begin(), found);
      }

      float lerp(const float &l,
                 const float &r,
                 const float &pl,
                 const float &pr,
                 const float &p)
      {
        const float dl =
            std::abs(pr - pl) > 0.0001f ? (p - pl) / (pr - pl) : 0.f;
        const float dr = 1.f - dl;
        return l * dr + r * dl;
      }

    }  // namespace help

    TransferFunctionWidget::TransferFunctionWidget(
        const range1f &_valueRange, const std::string &_widgetName)
        : valueRange(_valueRange), widgetName(_widgetName)
    {
      loadDefaultMaps();

      tfnColorPoints   = &(tfnsColorPoints[currentMap]);
      tfnOpacityPoints = &(tfnsOpacityPoints[currentMap]);
      tfnEditable      = tfnsEditable[currentMap];
    }

    TransferFunctionWidget::~TransferFunctionWidget()
    {
#if defined(VKL_HAVE_IMGUI)
      if (tfnPaletteTexture) {
        glDeleteTextures(1, &tfnPaletteTexture);
      }
#endif // defined(VKL_HAVE_IMGUI)
    }

    bool TransferFunctionWidget::updateUI()
    {
      bool changed = tfnChanged;

#if defined(VKL_HAVE_IMGUI)

      // set ImGui double click time to 1s, so it also works for slower frame
      // rates
      ImGuiIO &io             = ImGui::GetIO();
      io.MouseDoubleClickTime = 1.f;

      if (tfnChanged) {
        updateTfnPaletteTexture();
        tfnChanged = false;
      }

      // need a unique ImGui group name per widget
      ImGui::PushID(widgetName.c_str());

      ImGui::Text("Linear Transfer Function");

      ImGui::Separator();
      std::vector<const char *> names(tfnsNames.size(), nullptr);
      std::transform(tfnsNames.begin(),
                     tfnsNames.end(),
                     names.begin(),
                     [](const std::string &t) { return t.c_str(); });

      int newMap = currentMap;
      if (ImGui::ListBox("Color maps", &newMap, names.data(), names.size())) {
        setMap(newMap);
      }

      ImGui::Separator();

      ImGui::Text("Opacity scale");
      ImGui::SameLine();
      if (ImGui::SliderFloat(
              "##OpacityScale", &globalOpacityScale, 0.f, 10.f)) {
        tfnChanged = true;
      }

      ImGui::Separator();

      if (ImGui::DragFloatRange2("Value range",
                                 &valueRange.lower,
                                 &valueRange.upper,
                                 0.1f,
                                 -10000.f,
                                 10000.0f,
                                 "Min: %.7f",
                                 "Max: %.7f")) {
        tfnChanged = true;
      }

      drawEditor();

      ImGui::PopID();

#endif // defined(VKL_HAVE_IMGUI)

      return changed;
    }

    range1f TransferFunctionWidget::getValueRange() const
    {
      return valueRange;
    }

    void TransferFunctionWidget::setValueRange(const range1f &range)
    {
      valueRange = range;
    }

    std::vector<vec4f> TransferFunctionWidget::getSampledColorsAndOpacities(
        int numSamples) const
    {
      std::vector<vec4f> sampledColorsAndOpacities;

      const float dx = 1.f / (numSamples - 1);

      for (int i = 0; i < numSamples; i++) {
        sampledColorsAndOpacities.push_back(
            vec4f(interpolateColor(*tfnColorPoints, i * dx),
                  interpolateOpacity(*tfnOpacityPoints, i * dx) *
                      globalOpacityScale));
      }

      return sampledColorsAndOpacities;
    }

    void TransferFunctionWidget::loadDefaultMaps()
    {
      // same opacities for all maps
      std::vector<OpacityPoint> opacities;

      opacities.emplace_back(0.f, 0.f);
      opacities.emplace_back(1.f, 1.f);

      // Jet
      std::vector<ColorPoint> colors;

      colors.emplace_back(0.0f, 0.f, 0.f, 1.f);
      colors.emplace_back(0.3f, 0.f, 1.f, 1.f);
      colors.emplace_back(0.6f, 1.f, 1.f, 0.f);
      colors.emplace_back(1.0f, 1.f, 0.f, 0.f);

      tfnsColorPoints.push_back(colors);
      tfnsOpacityPoints.push_back(opacities);

      tfnsEditable.push_back(true);
      tfnsNames.push_back("Jet");

      // Ice Fire
      colors.clear();

      const float spacing = 1.f / 16;

      colors.emplace_back(0 * spacing, 0, 0, 0);
      colors.emplace_back(1 * spacing, 0, 0.120394, 0.302678);
      colors.emplace_back(2 * spacing, 0, 0.216587, 0.524575);
      colors.emplace_back(3 * spacing, 0.0552529, 0.345022, 0.659495);
      colors.emplace_back(4 * spacing, 0.128054, 0.492592, 0.720287);
      colors.emplace_back(5 * spacing, 0.188952, 0.641306, 0.792096);
      colors.emplace_back(6 * spacing, 0.327672, 0.784939, 0.873426);
      colors.emplace_back(7 * spacing, 0.60824, 0.892164, 0.935546);
      colors.emplace_back(8 * spacing, 0.881376, 0.912184, 0.818097);
      colors.emplace_back(9 * spacing, 0.9514, 0.835615, 0.449271);
      colors.emplace_back(10 * spacing, 0.904479, 0.690486, 0);
      colors.emplace_back(11 * spacing, 0.854063, 0.510857, 0);
      colors.emplace_back(12 * spacing, 0.777096, 0.330175, 0.000885023);
      colors.emplace_back(13 * spacing, 0.672862, 0.139086, 0.00270085);
      colors.emplace_back(14 * spacing, 0.508812, 0, 0);
      colors.emplace_back(15 * spacing, 0.299413, 0.000366217, 0.000549325);

      colors.emplace_back(1.f, 0.0157473, 0.00332647, 0);

      tfnsColorPoints.push_back(colors);
      tfnsOpacityPoints.push_back(opacities);

      tfnsEditable.push_back(true);
      tfnsNames.push_back("Ice Fire");

      // Grayscale
      colors.clear();

      colors.emplace_back(0.f, 1.f, 1.f, 1.f);
      colors.emplace_back(1.f, 1.f, 1.f, 1.f);

      tfnsColorPoints.push_back(colors);
      tfnsOpacityPoints.push_back(opacities);

      tfnsEditable.push_back(true);
      tfnsNames.push_back("Grayscale");
    };

    void TransferFunctionWidget::setMap(int selection)
    {
      if (currentMap != selection) {
        currentMap = selection;
        // Remember to update other constructors as well
        tfnColorPoints = &(tfnsColorPoints[selection]);
#if 1  // NOTE(jda) - this will use the first tf's opacities for all color maps
        tfnOpacityPoints = &(tfnsOpacityPoints[selection]);
#endif
        tfnEditable = tfnsEditable[selection];
        tfnChanged  = true;
      }
    }

    vec3f TransferFunctionWidget::interpolateColor(
        const std::vector<ColorPoint> &controlPoints, float x) const
    {
      auto first = controlPoints.front();
      if (x <= first.x)
        return vec3f(first.y, first.z, first.w);

      for (uint32_t i = 1; i < controlPoints.size(); i++) {
        auto current  = controlPoints[i];
        auto previous = controlPoints[i - 1];
        if (x <= current.x) {
          const float t = (x - previous.x) / (current.x - previous.x);
          return (1.0 - t) * vec3f(previous.y, previous.z, previous.w) +
                 t * vec3f(current.y, current.z, current.w);
        }
      }

      auto last = controlPoints.back();
      return vec3f(last.x, last.y, last.z);
    }

    float TransferFunctionWidget::interpolateOpacity(
        const std::vector<OpacityPoint> &controlPoints, float x) const

    {
      auto first = controlPoints.front();
      if (x <= first.x)
        return first.y;

      for (uint32_t i = 1; i < controlPoints.size(); i++) {
        auto current  = controlPoints[i];
        auto previous = controlPoints[i - 1];
        if (x <= current.x) {
          const float t = (x - previous.x) / (current.x - previous.x);
          return (1.0 - t) * previous.y + t * current.y;
        }
      }

      auto last = controlPoints.back();
      return last.y;
    }

    void TransferFunctionWidget::updateTfnPaletteTexture()
    {
#if defined(VKL_HAVE_IMGUI)
      const size_t textureWidth = 256, textureHeight = 1;

      // backup currently bound texture
      GLint prevBinding = 0;
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevBinding);

      // create transfer function palette texture if it doesn't exist
      if (!tfnPaletteTexture) {
        glGenTextures(1, &tfnPaletteTexture);
        glBindTexture(GL_TEXTURE_2D, tfnPaletteTexture);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA8,
                     textureWidth,
                     textureHeight,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      }

      // sample the palette then upload the data
      std::vector<vec4f> palette = getSampledColorsAndOpacities(textureWidth);

      // save palette to texture
      glBindTexture(GL_TEXTURE_2D, tfnPaletteTexture);
      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   GL_RGB,
                   textureWidth,
                   textureHeight,
                   0,
                   GL_RGBA,
                   GL_FLOAT,
                   static_cast<const void *>(palette.data()));

      // restore previously bound texture
      if (prevBinding) {
        glBindTexture(GL_TEXTURE_2D, prevBinding);
      }
#endif //defined(VKL_HAVE_IMGUI)
    }

    void TransferFunctionWidget::drawEditor()
    {
#if defined(VKL_HAVE_IMGUI)
      // only God and me know what do they do ...
      ImDrawList *draw_list   = ImGui::GetWindowDrawList();
      float canvas_x          = ImGui::GetCursorScreenPos().x;
      float canvas_y          = ImGui::GetCursorScreenPos().y;
      float canvas_avail_x    = ImGui::GetContentRegionAvail().x;
      float canvas_avail_y    = ImGui::GetContentRegionAvail().y;
      const float mouse_x     = ImGui::GetMousePos().x;
      const float mouse_y     = ImGui::GetMousePos().y;
      const float scroll_x    = ImGui::GetScrollX();
      const float scroll_y    = ImGui::GetScrollY();
      const float margin      = 10.f;
      const float width       = canvas_avail_x - 2.f * margin;
      const float height      = 260.f;
      const float color_len   = 9.f;
      const float opacity_len = 7.f;

      // draw preview texture
      ImGui::SetCursorScreenPos(ImVec2(canvas_x + margin, canvas_y));
      ImGui::Image(reinterpret_cast<void *>(tfnPaletteTexture),
                   ImVec2(width, height));

      ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
      for (int i = 0; i < tfnOpacityPoints->size() - 1; ++i) {
        std::vector<ImVec2> polyline;
        polyline.emplace_back(
            canvas_x + margin + (*tfnOpacityPoints)[i].x * width,
            canvas_y + height);
        polyline.emplace_back(
            canvas_x + margin + (*tfnOpacityPoints)[i].x * width,
            canvas_y + height - (*tfnOpacityPoints)[i].y * height);
        polyline.emplace_back(
            canvas_x + margin + (*tfnOpacityPoints)[i + 1].x * width + 1,
            canvas_y + height - (*tfnOpacityPoints)[i + 1].y * height);
        polyline.emplace_back(
            canvas_x + margin + (*tfnOpacityPoints)[i + 1].x * width + 1,
            canvas_y + height);
        draw_list->AddConvexPolyFilled(
            polyline.data(), polyline.size(), 0xFFD8D8D8);
      }
      canvas_y += height + margin;
      canvas_avail_y -= height + margin;

      // draw color control points
      ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));

      if (tfnEditable) {
        // draw circle background
        draw_list->AddRectFilled(ImVec2(canvas_x + margin, canvas_y - margin),
                                 ImVec2(canvas_x + margin + width,
                                        canvas_y - margin + 2.5 * color_len),
                                 0xFF474646);

        // draw circles
        for (int i = tfnColorPoints->size() - 1; i >= 0; --i) {
          const ImVec2 pos(canvas_x + width * (*tfnColorPoints)[i].x + margin,
                           canvas_y);
          ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));

          // white background
          draw_list->AddTriangleFilled(ImVec2(pos.x - 0.5f * color_len, pos.y),
                                       ImVec2(pos.x + 0.5f * color_len, pos.y),
                                       ImVec2(pos.x, pos.y - color_len),
                                       0xFFD8D8D8);
          draw_list->AddCircleFilled(
              ImVec2(pos.x, pos.y + 0.5f * color_len), color_len, 0xFFD8D8D8);

          // draw picker
          ImVec4 picked_color = ImColor((*tfnColorPoints)[i].y,
                                        (*tfnColorPoints)[i].z,
                                        (*tfnColorPoints)[i].w,
                                        1.f);
          ImGui::SetCursorScreenPos(
              ImVec2(pos.x - color_len, pos.y + 1.5f * color_len));
          if (ImGui::ColorEdit4(("##ColorPicker" + std::to_string(i)).c_str(),
                                (float *)&picked_color,
                                ImGuiColorEditFlags_NoAlpha |
                                    ImGuiColorEditFlags_NoInputs |
                                    ImGuiColorEditFlags_NoLabel |
                                    ImGuiColorEditFlags_AlphaPreview |
                                    ImGuiColorEditFlags_NoOptions |
                                    ImGuiColorEditFlags_NoTooltip)) {
            (*tfnColorPoints)[i].y = picked_color.x;
            (*tfnColorPoints)[i].z = picked_color.y;
            (*tfnColorPoints)[i].w = picked_color.z;
            tfnChanged             = true;
          }
          if (ImGui::IsItemHovered()) {
            // convert float color to char
            int cr = static_cast<int>(picked_color.x * 255);
            int cg = static_cast<int>(picked_color.y * 255);
            int cb = static_cast<int>(picked_color.z * 255);

            // setup tooltip
            ImGui::BeginTooltip();
            ImVec2 sz(
                ImGui::GetFontSize() * 4 + ImGui::GetStyle().FramePadding.y * 2,
                ImGui::GetFontSize() * 4 +
                    ImGui::GetStyle().FramePadding.y * 2);
            ImGui::ColorButton(
                "##PreviewColor",
                picked_color,
                ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaPreview,
                sz);
            ImGui::SameLine();
            ImGui::Text(
                "Left click to edit\n"
                "HEX: #%02X%02X%02X\n"
                "RGB: [%3d,%3d,%3d]\n(%.2f, %.2f, %.2f)",
                cr,
                cg,
                cb,
                cr,
                cg,
                cb,
                picked_color.x,
                picked_color.y,
                picked_color.z);
            ImGui::EndTooltip();
          }
        }
        for (int i = 0; i < tfnColorPoints->size(); ++i) {
          const ImVec2 pos(canvas_x + width * (*tfnColorPoints)[i].x + margin,
                           canvas_y);

          // draw button
          ImGui::SetCursorScreenPos(
              ImVec2(pos.x - color_len, pos.y - 0.5 * color_len));
          ImGui::InvisibleButton(
              ("##ColorControl-" + std::to_string(i)).c_str(),
              ImVec2(2.f * color_len, 2.f * color_len));

          // dark highlight
          ImGui::SetCursorScreenPos(ImVec2(pos.x - color_len, pos.y));
          draw_list->AddCircleFilled(
              ImVec2(pos.x, pos.y + 0.5f * color_len),
              0.5f * color_len,
              ImGui::IsItemHovered() ? 0xFF051C33 : 0xFFBCBCBC);

          // delete color point
          if (ImGui::IsMouseDoubleClicked(1) && ImGui::IsItemHovered()) {
            if (i > 0 && i < tfnColorPoints->size() - 1) {
              tfnColorPoints->erase(tfnColorPoints->begin() + i);
              tfnChanged = true;
            }
          }

          // drag color control point
          else if (ImGui::IsItemActive()) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            if (i > 0 && i < tfnColorPoints->size() - 1) {
              (*tfnColorPoints)[i].x += delta.x / width;
              (*tfnColorPoints)[i].x = clamp((*tfnColorPoints)[i].x,
                                             (*tfnColorPoints)[i - 1].x,
                                             (*tfnColorPoints)[i + 1].x);
            }

            tfnChanged = true;
          }
        }
      }

      // draw opacity control points
      ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
      {
        // draw circles
        for (int i = 0; i < tfnOpacityPoints->size(); ++i) {
          const ImVec2 pos(
              canvas_x + width * (*tfnOpacityPoints)[i].x + margin,
              canvas_y - height * (*tfnOpacityPoints)[i].y - margin);
          ImGui::SetCursorScreenPos(
              ImVec2(pos.x - opacity_len, pos.y - opacity_len));
          ImGui::InvisibleButton(
              ("##OpacityControl-" + std::to_string(i)).c_str(),
              ImVec2(2.f * opacity_len, 2.f * opacity_len));
          ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));

          // dark bounding box
          draw_list->AddCircleFilled(pos, opacity_len, 0xFF565656);

          // white background
          draw_list->AddCircleFilled(pos, 0.8f * opacity_len, 0xFFD8D8D8);

          // highlight
          draw_list->AddCircleFilled(
              pos,
              0.6f * opacity_len,
              ImGui::IsItemHovered() ? 0xFF051c33 : 0xFFD8D8D8);

          // setup interaction

          // delete opacity point
          if (ImGui::IsMouseDoubleClicked(1) && ImGui::IsItemHovered()) {
            if (i > 0 && i < tfnOpacityPoints->size() - 1) {
              tfnOpacityPoints->erase(tfnOpacityPoints->begin() + i);
              tfnChanged = true;
            }
          } else if (ImGui::IsItemActive()) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            (*tfnOpacityPoints)[i].y -= delta.y / height;
            (*tfnOpacityPoints)[i].y =
                clamp((*tfnOpacityPoints)[i].y, 0.0f, 1.0f);
            if (i > 0 && i < tfnOpacityPoints->size() - 1) {
              (*tfnOpacityPoints)[i].x += delta.x / width;
              (*tfnOpacityPoints)[i].x = clamp((*tfnOpacityPoints)[i].x,
                                               (*tfnOpacityPoints)[i - 1].x,
                                               (*tfnOpacityPoints)[i + 1].x);
            }
            tfnChanged = true;
          } else if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Double right click button to delete point\n"
                "Left click and drag to move point");
          }
        }
      }

      // draw background interaction
      ImGui::SetCursorScreenPos(ImVec2(canvas_x + margin, canvas_y - margin));
      ImGui::InvisibleButton("##tfn_palette_color",
                             ImVec2(width, 2.5 * color_len));

      // add color point
      if (tfnEditable && ImGui::IsMouseDoubleClicked(0) &&
          ImGui::IsItemHovered()) {
        const float p = clamp(
            (mouse_x - canvas_x - margin - scroll_x) / (float)width, 0.f, 1.f);
        const int ir   = help::find_idx(*tfnColorPoints, p);
        const int il   = ir - 1;
        const float pr = (*tfnColorPoints)[ir].x;
        const float pl = (*tfnColorPoints)[il].x;
        const float r  = help::lerp(
            (*tfnColorPoints)[il].y, (*tfnColorPoints)[ir].y, pl, pr, p);
        const float g = help::lerp(
            (*tfnColorPoints)[il].z, (*tfnColorPoints)[ir].z, pl, pr, p);
        const float b = help::lerp(
            (*tfnColorPoints)[il].w, (*tfnColorPoints)[ir].w, pl, pr, p);
        ColorPoint pt(p, r, g, b);
        tfnColorPoints->insert(tfnColorPoints->begin() + ir, pt);
        tfnChanged = true;
      }

      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Double left click to add new color point");
      }

      // draw background interaction
      ImGui::SetCursorScreenPos(
          ImVec2(canvas_x + margin, canvas_y - height - margin));
      ImGui::InvisibleButton("##tfn_palette_opacity", ImVec2(width, height));

      // add opacity point
      if (ImGui::IsMouseDoubleClicked(0) && ImGui::IsItemHovered()) {
        const float x = clamp(
            (mouse_x - canvas_x - margin - scroll_x) / (float)width, 0.f, 1.f);
        const float y =
            clamp(-(mouse_y - canvas_y + margin - scroll_y) / (float)height,
                  0.f,
                  1.f);
        const int idx = help::find_idx(*tfnOpacityPoints, x);
        OpacityPoint pt(x, y);
        tfnOpacityPoints->insert(tfnOpacityPoints->begin() + idx, pt);
        tfnChanged = true;
      }

      // update cursors
      canvas_y += 4.f * color_len + margin;
      canvas_avail_y -= 4.f * color_len + margin;

      ImGui::SetCursorScreenPos(ImVec2(canvas_x, canvas_y));
#endif //defined(VKL_HAVE_IMGUI)
    }

  }  // namespace examples
}  // namespace openvkl
