import triangle_application;

#include <cstdlib>
#include <exception>
#include <iostream>
import context;
import bean01;

int main(const int argc, char **argv)
{
  (void)argc;
  (void)argv;

  try {
    context::Context context;

    const bean01::Bean01 *bean01 = context.get_bean01();

    bean01->hello1();

    TriangleApplication app;
    // app.run();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
