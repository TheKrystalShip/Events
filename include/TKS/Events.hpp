#pragma once

#include <functional>
#include <list>
#include <algorithm>
#include <utility>
#include <atomic>
#include <mutex>
#include <future>

namespace TKS::Events
{
    template <typename... Args>

    class EventHandler
    {
    public:
        typedef std::function<void(Args...)> handler_func_type;
        typedef unsigned int handler_id_type;

        EventHandler()
        {
        }

        explicit EventHandler(const handler_func_type &handlerFunc) : _handlerFunc(handlerFunc)
        {
            _handlerId = ++_handlerIdCounter;
        }

        // Copy constructor
        EventHandler(const EventHandler &src) : _handlerFunc(src._handlerFunc), _handlerId(src._handlerId)
        {
        }

        // Move constructor
        EventHandler(EventHandler &&src) : _handlerFunc(std::move(src._handlerFunc)), _handlerId(src._handlerId)
        {
        }

        // Copy assignment operator
        EventHandler &operator=(const EventHandler &src)
        {
            _handlerFunc = src._handlerFunc;
            _handlerId = src._handlerId;

            return *this;
        }

        // Move assignment operator
        EventHandler &operator=(EventHandler &&src)
        {
            std::swap(_handlerFunc, src._handlerFunc);
            _handlerId = src._handlerId;

            return *this;
        }

        // Function call operator
        void operator()(Args... params) const
        {
            if (_handlerFunc)
                _handlerFunc(params...);
        }

        bool operator==(const EventHandler &other) const
        {
            return _handlerId == other._handlerId;
        }

        operator bool() const
        {
            return _handlerFunc;
        }

        handler_id_type id() const
        {
            return _handlerId;
        }

    private:
        handler_func_type _handlerFunc;
        handler_id_type _handlerId;
        static std::atomic_uint _handlerIdCounter;
    };

    template <typename... Args>
    std::atomic_uint EventHandler<Args...>::_handlerIdCounter(0);

    template <typename... Args>
    class Event
    {
    public:
        typedef EventHandler<Args...> handler_type;

        Event()
        {
        }

        // Copy constructor
        Event(const Event &src)
        {
            std::lock_guard<std::mutex> lock(src._handlersLocker);
            _handlers = src._handlers;
        }

        // Move constructor
        Event(Event &&src)
        {
            std::lock_guard<std::mutex> lock(src._handlersLocker);
            _handlers = std::move(src._handlers);
        }

        // Copy assignment operator
        Event &operator=(const Event &src)
        {
            std::lock_guard<std::mutex> lock(_handlersLocker);
            std::lock_guard<std::mutex> lock2(src._handlersLocker);
            _handlers = src._handlers;

            return *this;
        }

        // Move assignment operator
        Event &operator=(Event &&src)
        {
            std::lock_guard<std::mutex> lock(_handlersLocker);
            std::lock_guard<std::mutex> lock2(src._handlersLocker);
            std::swap(_handlers, src._handlers);

            return *this;
        }

        typename handler_type::handler_id_type add(const handler_type &handler)
        {
            std::lock_guard<std::mutex> lock(_handlersLocker);
            _handlers.push_back(handler);

            return handler.id();
        }

        inline typename handler_type::handler_id_type add(const typename handler_type::handler_func_type &handler)
        {
            return add(handler_type(handler));
        }

        bool remove(const handler_type &handler)
        {
            std::lock_guard<std::mutex> lock(_handlersLocker);

            auto it = std::find(_handlers.begin(), _handlers.end(), handler);
            if (it != _handlers.end())
            {
                _handlers.erase(it);
                return true;
            }

            return false;
        }

        bool remove_id(const typename handler_type::handler_id_type &handlerId)
        {
            std::lock_guard<std::mutex> lock(_handlersLocker);

            auto it = std::find_if(_handlers.begin(), _handlers.end(),
                                   [handlerId](const handler_type &handler)
                                   { return handler.id() == handlerId; });
            if (it != _handlers.end())
            {
                _handlers.erase(it);
                return true;
            }

            return false;
        }

        void call(Args... params) const
        {
            handler_collection_type handlersCopy = getHandlersCopy();
            callImplicit(handlersCopy, params...);
        }

        std::future<void> callAsync(Args... params) const
        {
            return std::async(
                std::launch::async, [this](Args... asyncParams)
                { call(asyncParams...); },
                params...);
        }

        inline void operator()(Args... params) const
        {
            call(params...);
        }

        inline typename handler_type::handler_id_type operator+=(const handler_type &handler)
        {
            return add(handler);
        }

        inline typename handler_type::handler_id_type operator+=(const typename handler_type::handler_func_type &handler)
        {
            return add(handler);
        }

        inline bool operator-=(const handler_type &handler)
        {
            return remove(handler);
        }

    protected:
        typedef std::list<handler_type> handler_collection_type;

        void callImplicit(const handler_collection_type &handlers, Args... params) const
        {
            for (const auto &handler : handlers)
            {
                handler(params...);
            }
        }

        handler_collection_type getHandlersCopy() const
        {
            std::lock_guard<std::mutex> lock(_handlersLocker);

            // Since the function return value is by copy,
            // before the function returns (and destruct the lock_guard object),
            // it creates a copy of the _handlers container.

            return _handlers;
        }

    private:
        handler_collection_type _handlers;
        mutable std::mutex _handlersLocker;
    };
} // namespace TKS::Events
