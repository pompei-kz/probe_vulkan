module;

export module setting;

export namespace app {
  /// Bean setting
  class Setting
  {
    int    argc_;
    char **argv_;

  public:
    void readApplicationArguments(const int argc, char **argv)
    {
      argc_ = argc;
      argv_ = argv;
    }
  };
} // namespace app