#pragma once
// Readback of the last frame handed to the sim LED driver. Sim-only:
// lets the preview server grab pixels without knowing the driver.
//
// out must hold MATRIX_WIDTH * MATRIX_HEIGHT * 3 bytes; triplets go in strip
// order (the lamp's pixel buffer order). Brightness is already applied, the
// same scaling the hardware output would do. Before the first show() the
// snapshot is black.
#include <stdint.h>

namespace hal
{
    void simSnapshot(uint8_t* out);
}
