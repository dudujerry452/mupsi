#include "boudingbox.h"
#include "geometry/ray.h"

namespace mupsi {

bool AABB::intersect(const Ray& ray) const {
    float tmin = -INFINITY;
    float tmax =  INFINITY;

    if (std::abs(ray.direction().x()) > 1e-8f) {
        float t1 = (minpos.x() - ray.origin().x()) / ray.direction().x();
        float t2 = (maxpos.x() - ray.origin().x()) / ray.direction().x();
        tmin = std::max(tmin, std::min(t1, t2));
        tmax = std::min(tmax, std::max(t1, t2));
    } else if (ray.origin().x() < minpos.x() || ray.origin().x() > maxpos.x()) {
        return false;
    }

    if (std::abs(ray.direction().y()) > 1e-8f) {
        float t1 = (minpos.y() - ray.origin().y()) / ray.direction().y();
        float t2 = (maxpos.y() - ray.origin().y()) / ray.direction().y();
        tmin = std::max(tmin, std::min(t1, t2));
        tmax = std::min(tmax, std::max(t1, t2));
    } else if (ray.origin().y() < minpos.y() || ray.origin().y() > maxpos.y()) {
        return false;
    }

    if (std::abs(ray.direction().z()) > 1e-8f) {
        float t1 = (minpos.z() - ray.origin().z()) / ray.direction().z();
        float t2 = (maxpos.z() - ray.origin().z()) / ray.direction().z();
        tmin = std::max(tmin, std::min(t1, t2));
        tmax = std::min(tmax, std::max(t1, t2));
    } else if (ray.origin().z() < minpos.z() || ray.origin().z() > maxpos.z()) {
        return false;
    }

    return tmax >= tmin && tmax >= ray.nearT() && tmin <= ray.farT();
}

}
