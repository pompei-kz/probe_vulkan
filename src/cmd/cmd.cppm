module;
#include <functional>
#include <glm/vec3.hpp>
#include <memory>
#include <string>

export module cmd;
import util;

export namespace cmd {

  /**
   * Abstract class for all commands
   */
  struct Cmd
  {
    virtual ~Cmd() = default;
  };

  /**
   * Reference to command
   */
  using CmdPtr = std::shared_ptr<Cmd>;

  /**
   * Command factory function
   */
  using CmdFactory = std::function<CmdPtr()>;

  /**
   * Command to print log message into console.
   */
  struct CmdPrintToConsole : Cmd
  {
    std::string message;
  };

  /**
   * Command that groups multiple commands into a single command.
   *
   * Commands stored in `sequence` are executed in the order in which
   * they appear in the vector.
   *
   * This command is useful when several commands need to be represented,
   * stored, or executed as a single logical command.
   */
  struct CmdSequence : Cmd
  {
    std::vector<CmdPtr> sequence;
  };

  /**
   * Clipping planes of the camera view frustum.
   */
  struct Planes
  {
    /**
     * Distance from the camera to the near clipping plane.
     *
     * Geometry closer than this distance will not be rendered.
     */
    float near;

    /**
     * Distance from the camera to the far clipping plane.
     *
     * Geometry farther than this distance will not be rendered.
     */
    float far;
  };

  /**
   * Command that updates camera parameters.
   *
   * Each camera property has a corresponding to apply flag.
   * A property is updated only if its apply flag is set to true.
   */
  struct CmdChangeCamera : Cmd
  {
    /**
     * Apply camera position.
     */
    bool positionApply = false;

    /**
     * New camera position in world space.
     */
    glm::vec3 position{0.0F, 0.0F, 1.0F};

    /**
     * Apply camera forward direction.
     */
    bool forwardApply = false;

    /**
     * Forward direction of the camera.
     * This vector defines the direction the camera is looking toward.
     */
    glm::vec3 forward{0.0F, 0.0F, -1.0F};

    /**
     * Apply camera up direction.
     */
    bool upApply = false;

    /**
     * Up direction of the camera.
     *
     * This vector does not have to be normalized or perpendicular to
     * `forward`. The system will recalculate it so that the resulting
     * up vector is normalized and perpendicular to `forward`.
     *
     * The original `up`, `forward`, and recalculated `up` vectors must lie
     * in the same plane.
     */
    glm::vec3 up{0.0F, 1.0F, 0.0F};

    /**
     * Apply near and far clipping plane parameters.
     */
    bool planesApply = false;

    /**
     * Near and far clipping planes of the camera frustum.
     * Only geometry located between these planes is rendered.
     */
    Planes planes;

    /**
     * Apply field of view.
     */
    bool fovDegreesApply = false;

    /**
     * Vertical field of view in degrees.
     */
    float fovDegrees = 45.0F;
  };

} // namespace cmd
