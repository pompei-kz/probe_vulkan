module;

#include <glm/vec3.hpp>
#include <iostream>
#include <memory>
#include <vector>

export module server;
import cmd;
import util;
import cmd_pipeline;
import cmd_light;
import generator;

export namespace server {
  /// Bean server
  class Server
  {
    float number = 8.09F;

  public:
    cmd::CmdPtr start()
    {
      auto ret = std::make_shared<cmd::CmdSequence>();

      {
        const auto camera = std::make_shared<cmd::CmdChangeCamera>();

        camera->position   = glm::vec3(0.0F, 0.0F, -10.0F);
        camera->forward    = glm::vec3(0.0F, 0.0F, 1.0F);
        camera->up         = glm::vec3(0.0F, -1.0F, 0.0F);
        camera->planes     = cmd::Planes{0.01, 100};
        camera->fovDegrees = 45;

        camera->positionApply   = true;
        camera->forwardApply    = true;
        camera->upApply         = true;
        camera->planesApply     = true;
        camera->fovDegreesApply = true;

        ret->sequence.push_back(camera);
      }

      {
        const auto sun = std::make_shared<cmd::CmdSetLight_Sun>();
        sun->id        = "a9Kp2nVqL4";
        sun->force     = 1.0F;
        sun->direction = glm::vec3(0.25F, 0.5F, 1.0F);
        sun->color     = glm::vec3(1.0F, 1.0F, 0.95F);

        ret->sequence.push_back(sun);
      }

      {
        const auto sphera = std::make_shared<cmd::CmdSetPipeline_ShapeGroup>();
        sphera->id        = "fp7c4pmXp1";

        sphera->meshes.resize(1);
        gen::populateWithSphera(&sphera->meshes[0], 1, 16, 16);

        sphera->materials.resize(2);
        sphera->materials[0].color = glm::vec3(0.0F, 0.5F, 1.0F);
        sphera->materials[1].color = glm::vec3(0.0F, 1.0F, 0.0F);

        sphera->shapeCountFn = [] { return 3; };

        sphera->populateShapesFn = [](std::vector<cmd::Shape> &shapes) {
          for (auto &shape : shapes) {
            shape.meshIndex      = 0;
            shape.materialIndex  = 0;
            shape.rotationVector = glm::vec3(0.0F, 0.0F, 0.0F);
            shape.scale          = glm::vec3(1.0F, 1.0F, 1.0F);
          }

          shapes[0].position = glm::vec3(0.0F, 0.0F, 0.0F);
          shapes[1].position = glm::vec3(2.0F, 0.0F, 0.0F);
          shapes[2].position = glm::vec3(0.0F, 2.0F, 0.0F);

          shapes[1].materialIndex = 1;
        };

        ret->sequence.push_back(sphera);
      }
      return ret;
    }

    ~Server()
    {
      std::cout << util::nowStr() << " ZZzYOtuBsD :: Server are destroying..." << std::endl;
    }
  };
} // namespace server
