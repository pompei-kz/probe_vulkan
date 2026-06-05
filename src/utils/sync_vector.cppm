module;

#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <utility>
#include <vector>

export module sync_vector;

export namespace sync {

  template <typename Element> //
  class SyncVector
  {
    mutable std::shared_mutex mutex_;
    std::vector<Element>      data_;

  public:
    void push_back(const Element &element)
    {
      std::unique_lock lock(mutex_);
      data_.push_back(element);
    }

    void push_back(Element &&element)
    {
      std::unique_lock lock(mutex_);
      data_.push_back(std::move(element));
    }

    template <typename... Args> void emplace_back(Args &&...args)
    {
      std::unique_lock lock(mutex_);
      data_.emplace_back(std::forward<Args>(args)...);
    }

    [[nodiscard]] std::optional<Element> get(size_t index) const
    {
      std::shared_lock lock(mutex_);

      if (index >= data_.size()) return std::nullopt;

      return data_[index];
    }

    [[nodiscard]] std::optional<Element> front() const
    {
      std::shared_lock lock(mutex_);

      if (data_.empty()) return std::nullopt;

      return data_.front();
    }

    [[nodiscard]] std::optional<Element> back() const
    {
      std::shared_lock lock(mutex_);

      if (data_.empty()) return std::nullopt;

      return data_.back();
    }

    [[nodiscard]] size_t size() const
    {
      std::shared_lock lock(mutex_);
      return data_.size();
    }

    [[nodiscard]] bool empty() const
    {
      std::shared_lock lock(mutex_);
      return data_.empty();
    }

    void clear()
    {
      std::unique_lock lock(mutex_);
      data_.clear();
    }

    void reserve(size_t capacity)
    {
      std::unique_lock lock(mutex_);
      data_.reserve(capacity);
    }

    void resize(size_t count)
    {
      std::unique_lock lock(mutex_);
      data_.resize(count);
    }

    void pop_back()
    {
      std::unique_lock lock(mutex_);

      if (!data_.empty()) data_.pop_back();
    }

    [[nodiscard]] std::vector<Element> snapshot() const
    {
      std::shared_lock lock(mutex_);
      return data_;
    }

    template <typename Func> void for_each(Func &&func) const
    {
      std::shared_lock lock(mutex_);

      for (const auto &item : data_) {
        func(item);
      }
    }
  };

} // namespace sync
