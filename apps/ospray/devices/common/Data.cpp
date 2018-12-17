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

#include "Data.h"
// core ospray
#include "ospray/SDK/common/OSPCommon.h"

namespace ospray {
  namespace scalar_volley_device {

    // Data definitions ///////////////////////////////////////////////////////

    Data::Data(size_t numItems, OSPDataType type)
        : numItems(numItems), type(type)
    {
    }

    // RawData definitions ////////////////////////////////////////////////////

    RawData::RawData(size_t numItems,
                     OSPDataType type,
                     int flags,
                     const void *init)
        : Data(numItems, type), numBytes(numItems * sizeOf(type)), flags(flags)
    {
      if (flags & OSP_DATA_SHARED_BUFFER)
        memory = const_cast<void *>(init);
      else {
        memory = alignedMalloc(numBytes + 16);
        if (init)
          memcpy(memory, init, numBytes);
        else if (type == OSP_OBJECT)
          memset(memory, 0, numBytes);
      }
    }

    RawData::~RawData()
    {
      if (!(flags & OSP_DATA_SHARED_BUFFER))
        alignedFree(memory);
    }

    void *RawData::data() const
    {
      return memory;
    }

    size_t RawData::size() const
    {
      return numItems;
    }

    // ObjectData definitions /////////////////////////////////////////////////

    ObjectData::ObjectData(size_t numItems, OSPDataType type, const void *init)
        : Data(numItems, type)
    {
      t.resize(numItems);
      if (init) {
        auto handles = static_cast<const ManagedPtr *>(init);
        for (size_t i = 0; i < numItems; i++)
          t[i] = handles[i]->shared_from_this();
      }
    }

    size_t ObjectData::size() const
    {
      return t.size();
    }

    void *ObjectData::data() const
    {
      return (void *)t.data();
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
