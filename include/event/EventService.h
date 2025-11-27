#ifndef EVENT_EVENTSERVICE_H
#define EVENT_EVENTSERVICE_H

#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>


namespace event {

/**
 * Type-erased event holder (C++11 compatible replacement for std::any)
 */
class EventHolder {
public:
    EventHolder() : content_(nullptr) {}

    template<typename T>
    EventHolder(const T& value) : content_(new Holder<T>(value)) {}

    EventHolder(const EventHolder& other)
        : content_(other.content_ ? other.content_->clone() : nullptr) {}

    EventHolder& operator=(const EventHolder& other) {
        if (this != &other) {
            EventHolder tmp(other);
            std::swap(content_, tmp.content_);
        }
        return *this;
    }

    ~EventHolder() { delete content_; }

    template<typename T>
    const T* cast() const {
        if (content_ && content_->type() == typeid(T)) {
            return &static_cast<Holder<T>*>(content_)->value_;
        }
        return nullptr;
    }

private:
    struct HolderBase {
        virtual ~HolderBase() {}
        virtual HolderBase* clone() const = 0;
        virtual const std::type_info& type() const = 0;
    };

    template<typename T>
    struct Holder : public HolderBase {
        T value_;
        Holder(const T& value) : value_(value) {}
        HolderBase* clone() const override { return new Holder(value_); }
        const std::type_info& type() const override { return typeid(T); }
    };

    HolderBase* content_;
};

/**
 * Event subscriber callback type
 * Receives the event as EventHolder and must cast to appropriate type
 */
using EventSubscriber = std::function<void(const EventHolder&)>;

/**
 * EventService provides a publish-subscribe event bus
 * This is the C++ port of the Java EventService
 */
class EventService {
public:
    EventService() = default;
    ~EventService() = default;

    /**
     * Subscribe to events of a specific type
     * @tparam T The event type to subscribe to
     * @param callback The callback function to invoke when event is published
     * @return Subscription ID for unsubscribing
     */
    template<typename T>
    int subscribe(std::function<void(const T&)> callback) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        auto typeIndex = std::type_index(typeid(T));
        int subscriptionId = nextSubscriptionId_++;

        // Wrap the typed callback in a generic one
        auto wrapper = [callback](const EventHolder& event) {
            const T* typedEvent = event.cast<T>();
            if (typedEvent) {
                callback(*typedEvent);
            }
        };

        subscribers_[typeIndex].push_back({subscriptionId, wrapper});
        return subscriptionId;
    }

    /**
     * Publish an event to all subscribers
     * @tparam T The event type
     * @param event The event object to publish
     */
    template<typename T>
    void publish(const T& event) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        auto typeIndex = std::type_index(typeid(T));
        auto it = subscribers_.find(typeIndex);

        if (it != subscribers_.end()) {
            EventHolder eventHolder(event);
            for (const auto& subscriber : it->second) {
                subscriber.callback(eventHolder);
            }
        }
    }

    /**
     * Unsubscribe from events
     * @param subscriptionId The subscription ID returned from subscribe()
     */
    void unsubscribe(int subscriptionId);

    /**
     * Clear all subscriptions
     */
    void clear();

private:
    struct Subscription {
        int id;
        EventSubscriber callback;
    };

    std::unordered_map<std::type_index, std::vector<Subscription>> subscribers_;
    std::recursive_mutex mutex_;
    int nextSubscriptionId_ = 0;
};

} // namespace event


#endif // EVENT_EVENTSERVICE_H
