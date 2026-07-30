#ifndef _RENDER_CONTEXT_H_
#define _RENDER_CONTEXT_H_

#include "camera.h"
#include "trace.h"              // for PathTracerSettings
#include "gp/gpnoise.h"        // for GPSettings
#include "geometry/scene.h"
#include <memory>
#include <utility>

namespace mupsi {

// Immutable snapshot of everything a progressive render needs.
// Built at render-start time; the render thread reads ONLY from this,
// never touching the mutable globals or the editable scene's camera.
struct RenderContext {
    Camera               camera; // deep-copied value
    PathTracerSettings   pts;    // snapshot of g_pathTracerSettings
    GPSettings           gp;     // snapshot of g_gpSettings
    std::shared_ptr<Scene> scene; // shared immutable geometry

    RenderContext(const Camera& cam,
                  const PathTracerSettings& p,
                  const GPSettings& g,
                  std::shared_ptr<Scene> s)
        : camera(cam), pts(p), gp(g), scene(std::move(s)) {}
};

} // namespace mupsi

#endif
