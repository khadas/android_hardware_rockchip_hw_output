/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_DRM_PROPERTY_H_
#define ANDROID_DRM_PROPERTY_H_

#include <stdint.h>
#include <string>
#include <xf86drmMode.h>
#include <vector>
#include <log/log.h>

namespace android {

enum DrmPropertyType {
  DRM_PROPERTY_TYPE_INT,
  DRM_PROPERTY_TYPE_ENUM,
  DRM_PROPERTY_TYPE_OBJECT,
  DRM_PROPERTY_TYPE_BLOB,
  DRM_PROPERTY_TYPE_BITMASK,
  DRM_PROPERTY_TYPE_INVALID,
};

struct hdr_static_metadata
{
  uint16_t eotf;
  uint16_t type;
  uint16_t display_primaries_x[3];
  uint16_t display_primaries_y[3];
  uint16_t white_point_x;
  uint16_t white_point_y;
  uint16_t max_mastering_display_luminance;
  uint16_t min_mastering_display_luminance;
  uint16_t max_fall;
  uint16_t max_cll;
  uint16_t min_cll;
};

class DrmProperty {
 public:
  DrmProperty() = default;
  DrmProperty(drmModePropertyPtr p, uint64_t value);
  DrmProperty(const DrmProperty &) = delete;
  DrmProperty &operator=(const DrmProperty &) = delete;

  void Init(drmModePropertyPtr p, uint64_t value);

  uint32_t id() const;
  std::string name() const;

  int value(uint64_t *value) const;

  void set_feature(const char* pcFeature)const;

  drmModePropertyPtr get_raw_property()const { return p_; }

 private:
  class DrmPropertyEnum {
   public:
    DrmPropertyEnum(drm_mode_property_enum *e);
    ~DrmPropertyEnum();

    uint64_t value_;
    std::string name_;
  };

  uint32_t id_ = 0;

  DrmPropertyType type_ = DRM_PROPERTY_TYPE_INVALID;
  uint32_t flags_ = 0;
  std::string name_;
  mutable const char* feature_name_;
  uint64_t value_ = 0;

  std::vector<uint64_t> values_;
  std::vector<DrmPropertyEnum> enums_;
  std::vector<uint32_t> blob_ids_;
  drmModePropertyPtr p_;
};
}

#endif  // ANDROID_DRM_PROPERTY_H_
