import triangle_application;

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
import context;
import cmd;

int main(const int argc, char **argv)
{
  try {
    context::Context context;
    context.get_setting()->readApplicationArguments(argc, argv);

    app::TriangleApplication *application = context.get_application();

    application->run([&context] { return context.get_server()->start(); });

  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
