module;
#include <iostream>

export module bean02;

export namespace bean02 {
  /// Bean
  class Bean02
  {
    int i = 103;

  public:
    void hello() const
    {
      std::cout << "ZQ2beZag9b :: Hello from BEAN 02, i=" << i << std::endl;
    }
  };
}; // namespace bean02
