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

#pragma once

#include "ManagedObject.h"
#include "openvkl/openvkl.h"

namespace openvkl {

  struct OPENVKL_CORE_INTERFACE Data : public ManagedObject
  {
    Data(size_t numItems,
         VKLDataType dataType,
         const void *source,
         VKLDataCreationFlags dataCreationFlags);

    virtual ~Data() override;

    virtual std::string toString() const override;

    size_t size() const;

    template <typename T>
    T *begin();

    template <typename T>
    T *end();

    template <typename T>
    const T *begin() const;

    template <typename T>
    const T *end() const;

    template <typename T>
    T &at(size_t i);

    template <typename T>
    const T &at(size_t i) const;

    size_t numItems;
    size_t numBytes;
    VKLDataType dataType;
    void *data;
    VKLDataCreationFlags dataCreationFlags;
  };

  template <typename T>
  inline T *Data::begin()
  {
    return static_cast<T *>(data);
  }

  template <typename T>
  inline T *Data::end()
  {
    return begin<T>() + numItems;
  }

  template <typename T>
  inline const T *Data::begin() const
  {
    return static_cast<const T *>(data);
  }

  template <typename T>
  inline const T *Data::end() const
  {
    return begin<const T>() + numItems;
  }

}  // namespace openvkl
