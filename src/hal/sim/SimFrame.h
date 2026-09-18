#pragma once
// Readback of the last frame handed to the sim LED driver. Sim-only:
// lets the preview server grab pixels without knowing the driver.
//
// out must hold MATRIX_WIDTH * MATRIX_HEIGHT * 3 bytes; triplets go in matrix
// order - row by row from the bottom-left, the order Frame::at(x, y) uses -
// so the page never has to know how the strip is wired. indexMap is the
// lamp's own (x, y) -> strip table. Brightness is already applied, the same
// scaling the hardware output would do. Before the first show() the snapshot
// is black.
#include <stdint.h>

namespace hal
{
    void simSnapshot(uint8_t* out, const uint16_t* indexMap);
}
