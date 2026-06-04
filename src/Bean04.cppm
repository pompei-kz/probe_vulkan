module;
#include <iostream>
export module bean04;

export namespace bean04 {
  /// Bean
  class Bean04
  {
    int sin = 190;

  public:
    void hello() const
    {
      std::cout << "V2WdK0gru6 :: Hello from BEAN 03, sin=" << sin << std::endl;
    }
  };
} // namespace bean04
