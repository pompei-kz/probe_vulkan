module;

#include <glm/vec3.hpp>
#include <iostream>
#include <memory>
#include <vector>

export module server;
import cmd;
import utils;
import cmd_pipeline;
import generator;

export namespace server {
  /// Bean server
  class Server
  {
    float number = 8.09F;

  public:
    cmd::CmdPtr start()
    {
      auto ret = std::make_shared<cmd::CmdSetPipeline_ShapeGroup>();
      ret->id  = "fp7c4pmXp1";

      ret->meshes.resize(1);
      gen::populateWithSphera(&ret->meshes[0], 1, 16, 16);

      ret->materials.resize(2);
      ret->materials[0].color = glm::vec3(0.0F, 0.5F, 1.0F);
      ret->materials[1].color = glm::vec3(0.0F, 1.0F, 0.0F);

      ret->shapeCountFn = [] { return 3; };

      ret->populateShapesFn = [](std::vector<cmd::Shape> &shapes) {
        for (auto shape : shapes) {
          shape.meshIndex     = 0;
          shape.materialIndex = 0;
          shape.rotateX       = 0;
          shape.rotateY       = 0;
          shape.rotateZ       = 0;
          shape.scaleX        = 1;
          shape.scaleY        = 1;
          shape.scaleZ        = 1;
        }

        shapes[0].position = glm::vec3(0.0F, 0.0F, 0.0F);
        shapes[1].position = glm::vec3(2.0F, 0.0F, 0.0F);
        shapes[2].position = glm::vec3(0.0F, 2.0F, 0.0F);
      };
      return ret;
    }

    ~Server()
    {
      std::cout << nowStr() << " ZZzYOtuBsD :: Server are destroying..." << std::endl;
    }
  };
} // namespace server
