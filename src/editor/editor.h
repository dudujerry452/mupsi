#ifndef _EDITOR_H_
#define _EDITOR_H_

#include <string>
#include <functional>

namespace mupsi {

class Controller;

// Runs the interactive editor window.
// controller must already have a scene loaded (controller.load(...)).
// Returns when the window is closed.
int runEditor(Controller& controller, const std::string& windowTitle = "mupsi editor");

} // namespace mupsi

#endif
