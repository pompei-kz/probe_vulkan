module;
#include <iostream>

export module bean03;
import getter;
import bean04;

export namespace bean03 {
  /// Bean three ; bean04::Bean04
  class Bean03
  {
    float wow;

    getter::Getter<bean04::Bean04> &bean04_;

  public:
    explicit Bean03(getter::Getter<bean04::Bean04> &bean04)
        : wow(17.9F)
        , bean04_(bean04)
    {}

    void hello() const
    {
      std::cout << "Vcy2wSBniR :: Hello from BEAN 03, wow=" << wow << std::endl;
      bean04_->hello();
    }
  };
} // namespace bean03
