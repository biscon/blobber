#include "audio/policy.hpp"
#include <limits>
namespace blobber::audio {
bool unobstructed(GeometryView map, float x, float z, float tx, float tz) noexcept {
    if (!std::isfinite(x + z + tx + tz) || std::abs(tx - x) > 32 || std::abs(tz - z) > 32)
        return false;
    Cell cell{int(std::floor(x)), int(std::floor(z))}, end{int(std::floor(tx)), int(std::floor(tz))};
    const float dx = tx - x, dz = tz - z;
    const int sx = dx < 0 ? -1 : 1, sz = dz < 0 ? -1 : 1;
    const float ix = dx ? 1 / std::abs(dx) : std::numeric_limits<float>::infinity();
    const float iz = dz ? 1 / std::abs(dz) : std::numeric_limits<float>::infinity();
    float ax = dx ? ((sx > 0 ? cell.x + 1 - x : x - cell.x) * ix) : ix;
    float az = dz ? ((sz > 0 ? cell.y + 1 - z : z - cell.y) * iz) : iz;
    for (unsigned i = 0; i < 70; ++i) {
        if (cell == end)
            return true; // Endpoint may itself be a closing door.
        if (ax == az) {
            if (map.solid({cell.x + sx, cell.y}) || map.solid({cell.x, cell.y + sz}))
                return false;
            cell.x += sx;
            cell.y += sz;
            ax += ix;
            az += iz;
        } else if (ax < az) {
            cell.x += sx;
            ax += ix;
        } else {
            cell.y += sz;
            az += iz;
        }
        if (!(cell == end) && map.solid(cell))
            return false;
    }
    return false;
}
} // namespace blobber::audio
