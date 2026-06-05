module;

#include <string>

export module pipeline;
import cmd;

export namespace cmd {

  struct CmdPipeline : Cmd
  {
    std::string hello;
  };

  struct CmdPipeline_ShapeGroup_Materials : Cmd
  {
    std::string hello;
  };

}
