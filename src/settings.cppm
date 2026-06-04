module;

export module settings;

export namespace app {
  /// Bean setting
  class Settings
  {
    int    argc_ = 0;
    char **argv_ = nullptr;

  public:
    void readApplicationArguments(const int argc, char **argv)
    {
      argc_ = argc;
      argv_ = argv;
    }
  };
} // namespace app
