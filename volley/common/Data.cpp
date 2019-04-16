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
#include <ospray/ospcommon/memory/malloc.h>

namespace volley {

  Data::Data(size_t numItems,
             VLYDataType dataType,
             const void *source,
             VLYDataCreationFlags dataCreationFlags)
      : numItems(numItems),
        numBytes(numItems * sizeOf(dataType)),
        dataType(dataType),
        dataCreationFlags(dataCreationFlags)
  {
    if (dataCreationFlags & VLY_DATA_SHARED_BUFFER) {
      Assert2(source != NULL, "shared buffer is NULL");
      data = const_cast<void *>(source);
    } else {
      data = ospcommon::memory::alignedMalloc(numBytes + 16);
      if (source)
        memcpy(data, source, numBytes);
      else if (dataType == VLY_OBJECT)
        memset(data, 0, numBytes);
    }

    managedObjectType = VLY_DATA;

    if (dataType == VLY_OBJECT) {
      ManagedObject **child = (ManagedObject **)data;
      for (uint32_t i = 0; i < numItems; i++) {
        if (child[i])
          child[i]->refInc();
      }
    }
  }

  Data::~Data()
  {
    if (dataType == VLY_OBJECT) {
      Data **child = (Data **)data;
      for (uint32_t i = 0; i < numItems; i++) {
        if (child[i])
          child[i]->refDec();
      }
    }

    if (!(dataCreationFlags & VLY_DATA_SHARED_BUFFER))
      ospcommon::memory::alignedFree(data);
  }

  std::string Data::toString() const
  {
    return "volley::Data";
  }

  size_t volley::Data::size() const
  {
    return numItems;
  }

}  // namespace volley
