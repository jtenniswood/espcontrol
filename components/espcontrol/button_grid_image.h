#pragma once

// Compatibility adapter for the public Camera Card implementation.
//
// The reusable camera/image engine lives in button_grid_camera.h. Existing
// firmware code and guard scripts still include button_grid_image.h for the
// saved `type=image` Camera Card, so keep this header as the stable adapter.

#include "button_grid_camera.h"
