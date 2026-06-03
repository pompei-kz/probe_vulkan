module;

#include <memory>

export module triangle_application;

export class TriangleApplication
{
public:
  TriangleApplication();
  ~TriangleApplication();

  TriangleApplication(const TriangleApplication &)            = delete;
  TriangleApplication &operator=(const TriangleApplication &) = delete;

  void run();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
