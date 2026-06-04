import triangle_application;

#include <cstdlib>
#include <exception>
#include <iostream>
import context;

int main(const int argc, char **argv)
{
  try {
    context::Context context;
    context.get_setting()->readApplicationArguments(argc, argv);

    app::TriangleApplication *application = context.get_application();

    application->run();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
