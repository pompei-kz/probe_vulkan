import triangle_application;

#include <cstdlib>
#include <exception>
#include <iostream>

int main(const int argc, char **argv)
{
  (void)argc;
  (void)argv;

  try
  {
    TriangleApplication app;
    app.run();
  }
  catch (const std::exception &error)
  {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
