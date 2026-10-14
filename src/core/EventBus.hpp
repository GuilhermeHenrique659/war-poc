#pragma once

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace core {

// Synchronous, type-indexed event bus. Handlers run immediately on publish.
class EventBus {
public:
    template <typename Event>
    using Handler = std::function<void(const Event&)>;

    EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    template <typename Event>
    void subscribe(Handler<Event> handler)
    {
        if (!handler) {
            return;
        }

        handlers_[std::type_index(typeid(Event))].emplace_back(
            [handler = std::move(handler)](const void* event) {
                handler(*static_cast<const Event*>(event));
            });
    }

    template <typename Event>
    void publish(const Event& event) const
    {
        const auto it = handlers_.find(std::type_index(typeid(Event)));
        if (it == handlers_.end()) {
            return;
        }

        // Copy so handlers may subscribe while an event is being dispatched.
        const auto handlers = it->second;
        for (const auto& handler : handlers) {
            handler(&event);
        }
    }

private:
    using ErasedHandler = std::function<void(const void*)>;

    std::unordered_map<std::type_index, std::vector<ErasedHandler>> handlers_;
};

} // namespace core
