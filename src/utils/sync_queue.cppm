module;

#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <utility>
#include <vector>

export module sync_linked_list;

export namespace sync {

  template <typename Element> //
  class SyncQueue
  {
    mutable std::shared_mutex mutex_;
    std::queue<Element>       data_;

  public:
    void push(const Element &value)
    {
      std::unique_lock lock(mutex_);
      data_.push(value);
    }

    void push(Element &&value)
    {
      std::unique_lock lock(mutex_);
      data_.push(std::move(value));
    }

    [[nodiscard]] std::optional<Element> pop()
    {
      std::unique_lock lock(mutex_);

      if (data_.empty()) {
        return std::nullopt;
      }

      Element value = std::move(data_.front());

      data_.pop();

      return value;
    }

    [[nodiscard]] std::optional<Element> front() const
    {
      std::shared_lock lock(mutex_);

      if (data_.empty()) {
        return std::nullopt;
      }

      return data_.front();
    }

    [[nodiscard]] bool empty() const
    {
      std::shared_lock lock(mutex_);
      return data_.empty();
    }

    [[nodiscard]] size_t size() const
    {
      std::shared_lock lock(mutex_);
      return data_.size();
    }
  };
} // namespace sync