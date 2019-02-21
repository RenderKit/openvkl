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

#pragma once

// ospray API
#include "ospray/ospray.h"
// scalar_volley_device
#include "ManagedObject.h"

namespace ospray {
  namespace scalar_volley_device {

    struct Data : public ManagedObject
    {
      Data(size_t numItems, OSPDataType type);
      virtual ~Data() override = default;

      virtual size_t size() const = 0;
      virtual void *data() const  = 0;

      template <typename T>
      T &valueAt(size_t index);

      size_t numItems;  /*!< number of items */
      OSPDataType type; /*!< element type */
    };

    // Inlined definitions //

    template <typename T>
    inline T &Data::valueAt(size_t index)
    {
      auto *ptr = static_cast<T*>(data());
      return ptr[index];
    }

    ///////////////////////////////////////////////////////////////////////////

    struct RawData : public Data
    {
      RawData(size_t numItems, OSPDataType type, int flags, const void *init);
      ~RawData() override;

      void *data() const override;
      size_t size() const override;

     private:
      void *memory;
      size_t numBytes; /*!< total num bytes (sizeof(type)*numItems) */
      int flags;
    };

    struct ObjectData : public Data
    {
      ObjectData(size_t numItems, OSPDataType type, const void *init);

      size_t size() const override;
      void *data() const override;

     private:
      std::vector<ManagedPtr> t;
    };

  }  // namespace scalar_volley_device
}  // namespace ospray
