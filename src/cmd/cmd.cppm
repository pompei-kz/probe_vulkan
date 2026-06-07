module;
#include <functional>
#include <glm/vec3.hpp>
#include <memory>
#include <string>

export module cmd;
import utils;

export namespace cmd {

  struct Cmd
  {
    virtual ~Cmd() = default;
  };

  using CmdPtr     = std::shared_ptr<Cmd>;
  using CmdFactory = std::function<CmdPtr()>;

  struct CmdPrintToConsole : Cmd
  {
    std::string message;
  };

  struct CmdSequence : Cmd
  {
    bool                sync = true;
    std::vector<CmdPtr> sequence;
  };

  // This command changes camera parameters.
  // Each parameter has boolean flag
  struct CmdChangeCamera : Cmd
  {
    // apply position
    bool positionApply = false;
    // move camera to this position
    glm::vec3 position{0.0F, 0.0F, 1.0F};

    // apply forward vector
    bool forwardApply = false;
    // forward vector - the vector in which the camera is looking
    glm::vec3 forward{0.0F, 0.0F, -1.0F};

    // apply up vector
    bool upApply = false;
    // up vector - vector pointing upward when the camera is looking
    glm::vec3 up{0.0F, 1.0F, 0.0F};

    // apply plan parameters - camera sees only between these plans
    bool planeApply = false;
    // near plan parameter - distance alongside forward vector to near plan
    float nearPlane = 0.1F;
    // far plan parameter - distance alongside forward vector to far plan
    float farPlane  = 100.0F;

    // apply Field of View
    bool fovApply    = false;
    // Field of View in degrees of camera
    float fovDegrees = 45.0F;
  };

} // namespace cmd
