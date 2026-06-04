// ReSharper disable CppMemberFunctionMayBeStatic
module;

#include <shared_mutex>

export module context;
import getter;
import bean01;
import bean02;
import bean03;
import bean04;

export namespace context
{
  class Context
  {
    mutable std::shared_mutex mutex_;

    getter::Getter<bean01::Bean01> bean01_{&mutex_, [this] { return create_bean01(); }};
    getter::Getter<bean02::Bean02> bean02_{&mutex_, [this] { return create_bean02(); }};
    getter::Getter<bean03::Bean03> bean03_{&mutex_, [this] { return create_bean03(); }};
    getter::Getter<bean04::Bean04> bean04_{&mutex_, [this] { return create_bean04(); }};

  public:
    Context() = default;

    bean01::Bean01 *get_bean01() { return bean01_.get(); }

  private:
    bean01::Bean01 *create_bean01()
    {
      bean01::Bean01 *result = new bean01::Bean01(bean02_);
      result->initialize();
      return result;
    }

    bean02::Bean02 *create_bean02()
    {
      bean02::Bean02 *result = new bean02::Bean02();

      return result;
    }

    bean03::Bean03 *create_bean03()
    {
      bean03::Bean03 *result = new bean03::Bean03();

      return result;
    }

    bean04::Bean04 *create_bean04()
    {
      bean04::Bean04 *result = new bean04::Bean04();

      return result;
    }
  };
} // namespace context
