// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "ospcommon/memory/malloc.h"

namespace openvkl {

  Data::Data(size_t numItems,
             VKLDataType dataType,
             const void *source,
             VKLDataCreationFlags dataCreationFlags)
      : numItems(numItems),
        numBytes(numItems * sizeOf(dataType)),
        dataType(dataType),
        dataCreationFlags(dataCreationFlags)
  {
    if (dataCreationFlags & VKL_DATA_SHARED_BUFFER) {
      if (source == nullptr)
        throw std::runtime_error("shared buffer is NULL");
      data = const_cast<void *>(source);
    } else {
      data = ospcommon::memory::alignedMalloc(numBytes + 16);
      if (data == nullptr)
        throw std::runtime_error("data is NULL");
      if (source)
        memcpy(data, source, numBytes);
      else if (dataType == VKL_OBJECT)
        memset(data, 0, numBytes);
    }

    managedObjectType = VKL_DATA;

    if (isManagedObject(dataType)) {
      ManagedObject **child = (ManagedObject **)data;
      for (uint32_t i = 0; i < numItems; i++) {
        if (child[i])
          child[i]->refInc();
      }
    }
  }

  Data::~Data()
  {
    if (isManagedObject(dataType)) {
      Data **child = (Data **)data;
      for (uint32_t i = 0; i < numItems; i++) {
        if (child[i])
          child[i]->refDec();
      }
    }

    if (!(dataCreationFlags & VKL_DATA_SHARED_BUFFER))
      ospcommon::memory::alignedFree(data);
  }

  std::string Data::toString() const
  {
    return "openvkl::Data";
  }

  size_t openvkl::Data::size() const
  {
    return numItems;
  }

}  // namespace openvkl
