import triangle_application;

#include <cstdlib>
#include <exception>
#include <iostream>
import context;

int main(const int argc, char **argv)
{
  (void)argc;
  (void)argv;

  try {
    context::Context context;

    app::TriangleApplication *application = context.get_application();
    application->run();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
