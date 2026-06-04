module;
#include <iostream>

export module bean01;
import getter;
import bean02;
import bean03;

export namespace bean01 {
  ///Bean bean02::Bean02 ; bean03::Bean03
  class Bean01
  {
    getter::Getter<bean02::Bean02> &bean02_;
    getter::Getter<bean03::Bean03> &bean03_;

    int asd;

  public:
    explicit Bean01(getter::Getter<bean02::Bean02> &bean02, getter::Getter<bean03::Bean03> &bean03)
        : bean02_(bean02)
        , bean03_(bean03)
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
      bean03_->hello();
    }
  };
} // namespace bean01
