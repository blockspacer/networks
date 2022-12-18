#pragma once

#include "assert.h"
#include "fwd.h"

#include <cstddef>
#include <iterator>

namespace core {

template <class T>
class IntrusiveListItem {
 private:
  using ListItem = IntrusiveListItem<T>;

 public:
  inline IntrusiveListItem() noexcept
      : next_(this)
      , prev_(next_) {}

  inline ~IntrusiveListItem() { unlink(); }

  core_warn_unused_result inline bool empty() const noexcept { return (prev_ == this) && (next_ == this); }

  inline void unlink() noexcept {
    if (empty()) {
      return;
    }

    prev_->setNext(next_);
    next_->setPrev(prev_);

    setEnd();
  }

  inline void linkBefore(ListItem* before) noexcept {
    unlink();
    linkBeforeNoUnlink(before);
  }

  inline void linkBeforeNoUnlink(ListItem* before) noexcept {
    ListItem* const after = before->prev();

    after->setNext(this);
    setPrev(after);
    setNext(before);
    before->setPrev(this);
  }

  inline void linkBefore(ListItem& before) noexcept { linkBefore(&before); }

  inline void linkAfter(ListItem* after) noexcept {
    unlink();
    linkBeforeNoUnlink(after->next());
  }

  inline void linkAfter(ListItem& after) noexcept { linkAfter(&after); }

  inline ListItem* prev() noexcept { return prev_; }

  inline const ListItem* prev() const noexcept { return prev_; }

  inline ListItem* next() noexcept { return next_; }

  inline const ListItem* next() const noexcept { return next_; }

  inline void setEnd() noexcept {
    prev_ = this;
    next_ = prev_;
  }

  inline void setNext(ListItem* item) noexcept { next_ = item; }

  inline void setPrev(ListItem* item) noexcept { prev_ = item; }

  inline T* node() noexcept { return static_cast<T*>(this); }

  inline const T* node() const noexcept { return static_cast<const T*>(this); }

 private:
  inline IntrusiveListItem(const IntrusiveListItem&) = delete;
  inline IntrusiveListItem& operator=(const IntrusiveListItem&) = delete;

 private:
  ListItem* next_;
  ListItem* prev_;
};

template <class T>
class IntrusiveList {
 private:
  using ListItem = IntrusiveListItem<T>;

  template <class LI, class Node>
  class IteratorBase {
   public:
    using Item = LI;
    using Reference = Node&;
    using Pointer = Node*;

    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;

    using value_type = Node;
    using reference = Reference;
    using pointer = Pointer;

    inline IteratorBase() noexcept
        : item_(nullptr) {}

    template <class OtherLI, class OtherNode>
    inline IteratorBase(const IteratorBase<OtherLI, OtherNode>& right) noexcept
        : item_(right.item()) {}

    inline IteratorBase(Item* item) noexcept
        : item_(item) {}

    inline Item* item() const noexcept { return item_; }

    inline void next() noexcept { item_ = item_->next(); }

    inline void prev() noexcept { item_ = item_->prev(); }

    template <class OtherLI, class OtherNode>
    inline bool operator==(const IteratorBase<OtherLI, OtherNode>& right) const noexcept {
      return item() == right.item();
    }

    template <class OtherLI, class OtherNode>
    inline bool operator!=(const IteratorBase<OtherLI, OtherNode>& right) const noexcept {
      return item() != right.item();
    }

    inline IteratorBase& operator++() noexcept {
      next();

      return *this;
    }

    inline IteratorBase operator++(int) noexcept {
      IteratorBase ret(*this);

      next();

      return ret;
    }

    inline IteratorBase& operator--() noexcept {
      prev();

      return *this;
    }

    inline IteratorBase operator--(int) noexcept {
      IteratorBase ret(*this);

      prev();

      return ret;
    }

    inline Reference operator*() const noexcept { return *item_->node(); }

    inline Pointer operator->() const noexcept { return item_->node(); }

   private:
    Item* item_;
  };

  template <class Iterator>
  class ReverseIteratorBase {
   public:
    using Item = typename Iterator::Item;
    using Reference = typename Iterator::Reference;
    using Pointer = typename Iterator::Pointer;

    using iterator_category = typename Iterator::iterator_category;
    using difference_type = typename Iterator::difference_type;

    using value_type = typename Iterator::value_type;
    using reference = typename Iterator::reference;
    using pointer = typename Iterator::pointer;

    inline ReverseIteratorBase() noexcept = default;

    template <class OtherIterator>
    inline ReverseIteratorBase(const ReverseIteratorBase<OtherIterator>& right) noexcept
        : current_(right.base()) {}

    inline explicit ReverseIteratorBase(Iterator item) noexcept
        : current_(item) {}

    inline Iterator base() const noexcept { return current_; }

    inline Item* item() const noexcept {
      Iterator ret = current_;

      return (--ret).Item();
    }

    inline void next() noexcept { current_.prev(); }

    inline void prev() noexcept { current_.next(); }

    template <class OtherIterator>
    inline bool operator==(const ReverseIteratorBase<OtherIterator>& right) const noexcept {
      return base() == right.base();
    }

    template <class OtherIterator>
    inline bool operator!=(const ReverseIteratorBase<OtherIterator>& right) const noexcept {
      return base() != right.base();
    }

    inline ReverseIteratorBase& operator++() noexcept {
      next();

      return *this;
    }

    inline ReverseIteratorBase operator++(int) noexcept {
      ReverseIteratorBase ret(*this);

      next();

      return ret;
    }

    inline ReverseIteratorBase& operator--() noexcept {
      prev();

      return *this;
    }

    inline ReverseIteratorBase operator--(int) noexcept {
      ReverseIteratorBase ret(*this);

      prev();

      return ret;
    }

    inline Reference operator*() const noexcept {
      Iterator ret = current_;

      return *--ret;
    }

    inline Pointer operator->() const noexcept {
      Iterator ret = current_;

      return &*--ret;
    }

   private:
    Iterator current_;
  };

 public:
  using Iterator = IteratorBase<ListItem, T>;
  using ConstIterator = IteratorBase<const ListItem, const T>;

  using ReverseIterator = ReverseIteratorBase<Iterator>;
  using ConstReverseIterator = ReverseIteratorBase<ConstIterator>;

  using iterator = Iterator;
  using const_iterator = ConstIterator;

  using reverse_iterator = ReverseIterator;
  using const_reverse_iterator = ConstReverseIterator;

  inline void swap(IntrusiveList& right) noexcept {
    IntrusiveList temp;

    temp.append(right);
    core_assert(right.empty());
    right.append(*this);
    core_assert(this->empty());
    this->append(temp);
    core_assert(temp.empty());
  }

 public:
  inline IntrusiveList() noexcept = default;

  inline ~IntrusiveList() = default;

  inline IntrusiveList(IntrusiveList&& right) noexcept { this->swap(right); }

  inline IntrusiveList& operator=(IntrusiveList&& rhs) noexcept {
    this->swap(rhs);
    return *this;
  }

  inline explicit operator bool() const noexcept { return !empty(); }

  core_warn_unused_result inline bool empty() const noexcept { return end_.empty(); }

  core_warn_unused_result inline size_t size() const noexcept { return std::distance(begin(), end()); }

  inline void clear() noexcept { end_.unlink(); }

  inline iterator begin() noexcept { return ++end(); }

  inline iterator end() noexcept { return Iterator(&end_); }

  inline const_iterator begin() const noexcept { return ++end(); }

  inline const_iterator end() const noexcept { return ConstIterator(&end_); }

  inline reverse_iterator rbegin() noexcept { return ReverseIterator(end()); }

  inline reverse_iterator rend() noexcept { return ReverseIterator(begin()); }

  inline const_reverse_iterator rbegin() const noexcept { return ConstReverseIterator(end()); }

  inline const_reverse_iterator rend() const noexcept { return ConstReverseIterator(begin()); }

  inline const_iterator cbegin() const noexcept { return begin(); }

  inline const_iterator cend() const noexcept { return end(); }

  inline const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  inline const_reverse_iterator crend() const noexcept { return rend(); }

  inline T* back() noexcept { return end_.prev()->node(); }

  inline T* front() noexcept { return end_.next()->node(); }

  inline const T* back() const noexcept { return end_.prev()->node(); }

  inline const T* front() const noexcept { return end_.next()->node(); }

  inline void pushBack(ListItem* item) noexcept { item->linkBefore(end_); }

  inline void pushFront(ListItem* item) noexcept { item->linkAfter(end_); }

  inline T* popBack() noexcept {
    ListItem* const ret = end_.prev();

    ret->unlink();

    return ret->node();
  }

  inline T* popFront() noexcept {
    ListItem* const ret = end_.next();

    ret->unlink();

    return ret->node();
  }

  inline void append(IntrusiveList& list) noexcept { cut(list.begin(), list.end(), end()); }

  inline static void cut(Iterator begin, Iterator end, Iterator paste_before) noexcept {
    if (begin == end) {
      return;
    }

    ListItem* const cutFront = begin.item();
    ListItem* const gapBack = end.item();

    ListItem* const gapFront = cutFront->prev();
    ListItem* const cutBack = gapBack->prev();

    gapFront->setNext(gapBack);
    gapBack->setPrev(gapFront);

    ListItem* const pasteBack = paste_before.item();
    ListItem* const pasteFront = pasteBack->prev();

    pasteFront->setNext(cutFront);
    cutFront->setPrev(pasteFront);

    cutBack->setNext(pasteBack);
    pasteBack->setPrev(cutBack);
  }

  template <class Functor>
  inline void forEach(Functor&& functor) {
    Iterator i = begin();

    while (i != end()) {
      functor(&*(i++));
    }
  }

  template <class Functor>
  inline void forEach(Functor&& functor) const {
    ConstIterator i = begin();

    while (i != end()) {
      functor(&*(i++));
    }
  }

  template <class Comparator>
  inline void quickSort(Comparator&& comparator) {
    if (begin() == end() || ++begin() == end()) {
      return;
    }

    T* const pivot = popFront();
    IntrusiveList bigger;
    Iterator i = begin();

    while (i != end()) {
      if (comparator(*pivot, *i)) {
        bigger.pushBack(&*i++);
      } else {
        ++i;
      }
    }

    this->quickSort(comparator);
    bigger.quickSort(comparator);

    pushBack(pivot);
    append(bigger);
  }

 private:
  inline IntrusiveList(const IntrusiveList&) = delete;
  inline IntrusiveList& operator=(const IntrusiveList&) = delete;

 private:
  ListItem end_;
};

template <class T, class D>
class IntrusiveListWithAutoDelete : public IntrusiveList<T> {
 public:
  using Iterator = typename IntrusiveList<T>::Iterator;
  using ConstIterator = typename IntrusiveList<T>::ConstIterator;

  using ReverseIterator = typename IntrusiveList<T>::ReverseIterator;
  using ConstReverseIterator = typename IntrusiveList<T>::ConstReverseIterator;

  using iterator = Iterator;
  using const_iterator = ConstIterator;

  using reverse_iterator = ReverseIterator;
  using const_reverse_iterator = ConstReverseIterator;

  inline IntrusiveListWithAutoDelete() noexcept = default;

  inline IntrusiveListWithAutoDelete(IntrusiveListWithAutoDelete&& right) noexcept
      : IntrusiveList<T>(std::move(right)) {}

  inline ~IntrusiveListWithAutoDelete() { this->clear(); }

  IntrusiveListWithAutoDelete& operator=(IntrusiveListWithAutoDelete&& rhs) noexcept {
    IntrusiveList<T>::operator=(std::move(rhs));
    return *this;
  }

  inline void clear() noexcept {
    this->forEach([](auto* item) { D::destroy(item); });
  }

  inline static void cut(Iterator begin, Iterator end) noexcept {
    IntrusiveListWithAutoDelete<T, D> temp;
    cut(begin, end, temp.End());
  }

  inline static void cut(Iterator begin, Iterator end, Iterator paste_before) noexcept {
    IntrusiveList<T>::cut(begin, end, paste_before);
  }
};

}  // namespace core