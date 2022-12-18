#pragma once

#include <functional>

namespace core {

template <class T>
class IntrusiveList;

template <class T, class D>
class IntrusiveListWithAutoDelete;

class Deleter;

template <class T, class C, class D = Deleter>
class RefCounted;

template <class T>
class DefaultIntrusivePtrOps;

template <class T, class Ops = DefaultIntrusivePtrOps<T>>
class IntrusivePtr;

template <class T, class Ops = DefaultIntrusivePtrOps<T>>
class IntrusiveConstPtr;

class IThreadFactoru;

struct IObjectInQueue;
class ThreadFactoryHolder;

using ThreadFunction = std::function<void()>;

class IThreadPool;
class FakeThreadPool;
class AdaptiveThreadPool;
class SimpleThreadPool;

template <class Queue, class Slave>
class ThreadPoolBinder;

struct FutureException;

template <class T>
class Future;

template <class T>
class Promise;

}  // namespace core