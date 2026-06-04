// ReSharper disable CppMemberFunctionMayBeStatic
module;

#include <shared_mutex>

export module context;
import getter;
import bean01;
import bean02;
import bean03;
import bean04;

// TODO MAIN Using cmake you need to generate class like this with name `Context`, in namespace `context`, in module `context`.
// TODO put generating code to separate cmake-file in folder `cmake` in root of this project

export namespace context {
  class Context1
  {
    mutable std::shared_mutex mutex_;

    // TODO  For each class with comment ///Bean make here fields like there
    getter::Getter<bean01::Bean01> bean01_{&mutex_, [this] { return create_bean01(); }};
    getter::Getter<bean02::Bean02> bean02_{&mutex_, [this] { return create_bean02(); }};
    getter::Getter<bean03::Bean03> bean03_{&mutex_, [this] { return create_bean03(); }};
    getter::Getter<bean04::Bean04> bean04_{&mutex_, [this] { return create_bean04(); }};

  public:
    Context1() = default;

    bean01::Bean01 *get_bean01()
    {
      return bean01_.get();
    }

  private:

    /*
     * TODO Scan this class for member-fields with type: getter::Getter<TYPE> (let following $name is name of this field)
     * TODO  For each class in comment ///Bean $name do:
     * TODO    Create private member-function without arguments:
     * TODO      name: create_$name
     * TODO      return type: TYPE*
     * TODO    Implement these functions as example following (look comments too)
     */

    bean01::Bean01 *create_bean01()
    {
      bean01::Bean01 *result = new bean01::Bean01(bean02_, bean03_); // TODO Use constructor with argument types: getter::Getter<AnyType>

      // TODO If no such constructor, use empty constructor
      // TODO If no suitable constructor found - compile error

      result->initialize(); // TODO call method `initialize` without arguments if it is existed. If no - do not call

      return result;
    }

    bean02::Bean02 *create_bean02()
    {
      bean02::Bean02 *result = new bean02::Bean02();

      return result;
    }

    bean03::Bean03 *create_bean03()
    {
      bean03::Bean03 *result = new bean03::Bean03(bean04_);

      return result;
    }

    bean04::Bean04 *create_bean04()
    {
      bean04::Bean04 *result = new bean04::Bean04();

      return result;
    }
  };
} // namespace context
