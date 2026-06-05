module;
#include <functional>
#include <iostream>
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

    ~CmdPrintToConsole()
    {
      std::cout << nowStr() << " OJkZuoWa62 :: Destroy CmdPrintToConsole: message = " << message << std::endl;
    }
  };

} // namespace cmd
