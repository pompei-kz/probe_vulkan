module;
#include <iostream>

export module bean01;
import getter;
import bean02;

export namespace bean01 {
  class Bean01
  {
    getter::Getter<bean02::Bean02> &bean02_;

    int asd;

  public:
    explicit Bean01(getter::Getter<bean02::Bean02> &bean02)
        : bean02_(bean02)
        , asd(0)
    {}

    Bean01(const Bean01 &)            = delete;
    Bean01 &operator=(const Bean01 &) = delete;
    Bean01(Bean01 &&)                 = delete;
    Bean01 &operator=(Bean01 &&)      = delete;

    void initialize()
    {
      asd = 107;
      std::cout << "kniEDV7as2 :: Bean01 initializing" << std::endl;
    }

    void hello1() const
    {
      std::cout << "zOngRoGKjS :: Hello from Bean01, asd=" << asd << std::endl;
      bean02_->hello();
    }
  };
} // namespace bean01
