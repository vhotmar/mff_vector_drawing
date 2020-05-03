#pragma once

#include <memory>
#include <functional>
#include <stack>

#include <mff/leaf.h>

namespace mff {

/**
 * Simple object pool (not thread safe, but should be easy to add).
 *
 * Based on https://stackoverflow.com/a/27837534/1725462
 *
 * @tparam Value
 */
template <class Value>
class ObjectPool {
private:
    struct ExternalDeleter;

public:
    using value_type = Value;
    using pool_type = ObjectPool<value_type>;

    using pool_ptr = std::unique_ptr<value_type, ExternalDeleter>;
    using value_ptr = std::unique_ptr<value_type>;

    using allocate_function = std::function<boost::leaf::result<value_ptr>()>;
    using recycle_function = std::function<void(value_type*)>;

public:
    /*template <class T = Value,
        typename std::enable_if<std::is_default_constructible<T>::value,
            uint8_t>::type = 0>
    ObjectPool() : this_ptr_(new pool_type*(this)), allocate_fn_(allocate_function(std::make_unique<value_type>)) {}*/

    ObjectPool() = delete; // TODO: make default constructable for default constructable objects
    ObjectPool(allocate_function allocate)
        : this_ptr_(new pool_type*(this)), allocate_fn_(allocate) {
    }

    ObjectPool(allocate_function allocate, recycle_function recycle)
        : this_ptr_(new pool_type*(this)), allocate_fn_(allocate), recycle_fn_(recycle) {
    }

    uint32_t unused_resources() const {
        return pool_.size();
    }

    boost::leaf::result<pool_ptr> acquire() {
        LEAF_CHECK(ensure_available());

        pool_ptr tmp(pool_.top().release(), ExternalDeleter{this_ptr_});
        pool_.pop();
        return std::move(tmp);
    }

    boost::leaf::result<std::vector<pool_ptr>> acquire(std::size_t count) {
        std::vector<pool_ptr> result;

        for (std::size_t i = 0; i < count; i++) {
            LEAF_AUTO(item, acquire());
            result.push_back(std::move(item));
        }

        return std::move(result);
    }

    void add(value_ptr value) {
        pool_.push(std::move(value));
    }

    void add(std::vector<value_ptr> new_values) {
        while (!new_values.empty()) {
            value_ptr value = std::move(new_values.back());
            new_values.pop_back();

            add(std::move(value));
        }
    }

private:
    boost::leaf::result<void> ensure_available() {
        if (!pool_.empty()) return {};

        allocated_count_++;
        LEAF_AUTO(res, allocate_fn_());
        pool_.push(std::move(res));

        return {};
    }

    void recycle(value_type* ptr) {
        if (recycle_fn_) {
            recycle_fn_(ptr);
        }

        pool_.push(value_ptr{ptr});
    }

    struct ExternalDeleter {
        ExternalDeleter() = default;

        ExternalDeleter(const std::weak_ptr<pool_type*>& pool)
            : pool_(pool) {
        }

        void operator()(value_type* ptr) {
            // if pool still exists, then just move the pointer back to the pool
            if (auto pool = pool_.lock()) {
                (*pool.get())->recycle(ptr);

                return;
            }

            // otherwise delete it
            std::default_delete<value_type>{}(ptr);
        }

        std::weak_ptr<pool_type*> pool_;
    };

private:
    std::shared_ptr<pool_type*> this_ptr_ = nullptr;
    std::stack<value_ptr> pool_ = {};

    allocate_function allocate_fn_ = {};
    recycle_function recycle_fn_ = {};

    std::size_t allocated_count_ = 0;
};

template <typename T>
using UniqueObjectPool = std::unique_ptr<ObjectPool<T>>;

}