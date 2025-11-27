#include "event/EventService.h"
#include <algorithm>


namespace event {

void EventService::unsubscribe(int subscriptionId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    for (auto& pair : subscribers_) {
        auto& subscriptions = pair.second;
        auto it = std::remove_if(subscriptions.begin(), subscriptions.end(),
            [subscriptionId](const Subscription& sub) {
                return sub.id == subscriptionId;
            });

        if (it != subscriptions.end()) {
            subscriptions.erase(it, subscriptions.end());
            return;
        }
    }
}

void EventService::clear() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    subscribers_.clear();
}

} // namespace event

