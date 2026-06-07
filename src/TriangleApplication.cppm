module;

#include <functional>
#include <memory>

export module triangle_application;
import settings;
import getter;
import cmd;

export namespace app {
  /// Bean application app::Settings
  class TriangleApplication
  {
  public:
    TriangleApplication(getter::Getter<Settings> &setting);
    ~TriangleApplication();

    TriangleApplication(const TriangleApplication &)            = delete;
    TriangleApplication &operator=(const TriangleApplication &) = delete;

    void run(const cmd::CmdFactory &&startCmdGetter);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace app
