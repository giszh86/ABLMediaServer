// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_TARGET_RAY_SPACE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_TARGET_RAY_SPACE_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/renderer/modules/xr/xr_space.h"

namespace blink {

class XRTargetRaySpace : public XRSpace {
 public:
  XRTargetRaySpace(XRSession* session, XRInputSource* input_space);

  absl::optional<TransformationMatrix> MojoFromNative() const override;
  bool EmulatedPosition() const override;

  device::mojom::blink::XRNativeOriginInformationPtr NativeOrigin()
      const override;

  bool IsStationary() const override;

  std::string ToString() const override;

  void Trace(Visitor*) const override;

 private:
  Member<XRInputSource> input_source_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_TARGET_RAY_SPACE_H_
