module;

#include <memory>

export module triangle_application;
import settings;
import getter;

export namespace app {
  /// Bean application app::Settings
  class TriangleApplication
  {
  public:
    TriangleApplication(getter::Getter<Settings> &setting);
    ~TriangleApplication();

    TriangleApplication(const TriangleApplication &)            = delete;
    TriangleApplication &operator=(const TriangleApplication &) = delete;

    void run(const std::string &startPoint);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
} // namespace app
