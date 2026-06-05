module;

#include <iostream>
#include <memory>

export module server;
import cmd;
import utils;
import pipeline;

export namespace server {
  /// Bean server
  class Server
  {
    float number = 8.09F;

  public:
    cmd::CmdPtr start()
    {
      auto ret = std::make_shared<cmd::CmdPipeline_ShapeGroup_Materials>();
      ret->id  = "fp7c4pmXp1 :: super  pipeline";
      return ret;
    }

    ~Server()
    {
      std::cout << nowStr() << " ZZzYOtuBsD :: Server are destroying..." << std::endl;
    }
  };
} // namespace server
