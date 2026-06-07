module;

#include <glm/vec3.hpp>
#include <string>

export module cmd_light;
import cmd;

export namespace cmd {

  struct CmdSetLight : Cmd
  {
    /**
     * ID of pipeline
     */
    std::string id; // TODO this field is putting to key of `TriangleApplication::Impl.pipelines_`
  };

  struct CmdSetLight_Sun : CmdSetLight
  {
    /**
     * Force of Sun
     */
    float force = 1.0F;

    /**
     * Direction of sunshine
     */
    glm::vec3 direction = {0.0F, 0.0F, -1.0F};

    /**
     * Color of Sun
     */
    glm::vec3 color = {1.0F, 1.0F, 1.0F};
  };

} // namespace cmd
