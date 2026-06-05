module;

#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

export module sync_map;

namespace {
  template <typename H, typename K>
  concept HashFunction = requires(H h, K k) {
    { h(k) } -> std::convertible_to<std::size_t>;
  };

  template <typename Eq, typename K>
  concept EqualityComparator = requires(Eq eq, K a, K b) {
    { eq(a, b) } -> std::convertible_to<bool>;
  };
}

export namespace sync {

  template <typename Key, //
  typename Val, //
  typename Hash = std::hash<Key>, //
  typename KeyEqual = std::equal_to<Key>> //
  requires HashFunction<Hash, Key> && EqualityComparator<KeyEqual, Key>
  class SyncMap
  {
    mutable std::shared_mutex mutex_;
    std::unordered_map<Key, Val, Hash, KeyEqual> values_;

  public:
    SyncMap() = default;

    SyncMap(const SyncMap &) = delete;

    SyncMap &operator=(const SyncMap &) = delete;

    void put(Key key, Val value)
    {
      std::unique_lock lock(mutex_);
      values_.insert_or_assign(std::move(key), std::move(value));
    }

    [[nodiscard]] std::optional<Val> get(const Key &key) const
    {
      std::shared_lock lock(mutex_);
      const auto value = values_.find(key);
      if (value == values_.end())
      {
        return std::nullopt;
      }
      return value->second;
    }

    [[nodiscard]] Val valueOr(const Key &key, Val fallback) const
    {
      std::shared_lock lock(mutex_);
      const auto value = values_.find(key);
      if (value == values_.end())
      {
        return fallback;
      }
      return value->second;
    }

    [[nodiscard]] bool contains(const Key &key) const
    {
      std::shared_lock lock(mutex_);
      return values_.contains(key);
    }

    bool remove(const Key &key)
    {
      std::unique_lock lock(mutex_);
      return values_.erase(key) > 0;
    }

    void clear()
    {
      std::unique_lock lock(mutex_);
      values_.clear();
    }

    [[nodiscard]] bool empty() const
    {
      std::shared_lock lock(mutex_);
      return values_.empty();
    }

    [[nodiscard]] std::size_t size() const
    {
      std::shared_lock lock(mutex_);
      return values_.size();
    }

    [[nodiscard]] std::unordered_map<Key, Val, Hash, KeyEqual> snapshot() const
    {
      std::shared_lock lock(mutex_);
      return values_;
    }
  };
}
