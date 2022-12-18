#include "tls.h"
#include "atomic.h"
#include "intrusive_list.h"
#include "intrusive_ptr.h"
#include "singleton.h"

#include "absl/container/flat_hash_map.h"

#include <vector>

#include <pthread.h>

namespace {

static inline core::AtomicType AcquireKey() {
  static core::Atomic cur;
  return core::atomics::Increment(cur) - 1;
}

class GenericTlsBase {
 public:
  using SmallKey = size_t;

  class PerThreadStorage {
   public:
    struct Key : public core::NonCopyable {
      inline Key(core::tls::Dtor dtor)
          : key(AcquireKey())
          , dtor(dtor) {}

      SmallKey key;
      core::tls::Dtor dtor;
    };

    class StoredValue final : public core::IntrusiveListItem<StoredValue> {
     public:
      inline StoredValue(const Key* key)
          : data_(nullptr)
          , dtor_(key->dtor) {}

      inline ~StoredValue() {
        if (dtor_ && data_) {
          dtor_(data_);
        }
      }

      inline void set(void* ptr) noexcept { data_ = ptr; }

      [[nodiscard]] inline void* get() const noexcept { return data_; }

     private:
      void* data_;
      core::tls::Dtor dtor_;
    };

    inline StoredValue* value(const Key* key) {
      StoredValue*& ret = *valuePtr(key->key);
      if (!ret) {
        std::unique_ptr<StoredValue> sv(new StoredValue(key));
        storage_.pushFront(sv.get());
        ret = sv.release();
      }
      return ret;
    }

    inline StoredValue** valuePtr(size_t idx) {
      if (idx < 10000) {
        if (idx >= values_.size()) {
          values_.resize(idx + 1);
        }
        return &values_[idx];
      }
      return &far_values_[idx];
    }

   private:
    std::vector<StoredValue*> values_;
    absl::flat_hash_map<size_t, StoredValue*> far_values_;
    core::IntrusiveListWithAutoDelete<StoredValue, core::Deleter> storage_;
  };

  inline PerThreadStorage* myStorage() {
#if defined(core_have_fast_pod_tls)
    core_pod_static_thread(PerThreadStorage*) my(nullptr);
    if (!my) {
      my = myStorageSlow();
    }
    return my;
#else
    return myStorageSlow();
#endif
  }

  virtual PerThreadStorage* myStorageSlow() = 0;

  virtual ~GenericTlsBase() = default;
};

class MasterTls final : public GenericTlsBase {
 public:
  inline MasterTls() { core_verify(!pthread_key_create(&key_, Dtor), "pthread_key_create failed"); }

  inline ~MasterTls() override {
    Dtor(pthread_getspecific(key_));
    core_verify(!pthread_key_delete(key_), "pthread_key_delete failed");
  }

  static inline MasterTls* instance() { return core::SingletonWithPriority<MasterTls, 1>(); }

 private:
  PerThreadStorage* myStorageSlow() override {
    void* ret = pthread_getspecific(key_);
    if (!ret) {
      ret = new PerThreadStorage();
      core_verify(!pthread_setspecific(key_, ret), "pthread_setspecific failed");
    }
    return static_cast<PerThreadStorage*>(ret);
  }

  static void Dtor(void* ptr) { delete (PerThreadStorage*)ptr; }

 private:
  pthread_key_t key_;
};

using KeyDescriptor = MasterTls::PerThreadStorage::Key;

}  // namespace

namespace core::tls {

class Key::Impl final : public KeyDescriptor {
 public:
  inline Impl(Dtor dtor)
      : KeyDescriptor(dtor) {}

  [[nodiscard]] inline void* get() const { return MasterTls::instance()->myStorage()->value(this)->get(); }

  inline void set(void* val) const { MasterTls::instance()->myStorage()->value(this)->set(val); }

  static inline void cleanup() {}
};

Key::Key(Dtor dtor)
    : impl_(new Impl(dtor)) {}

Key::Key(Key&& key) noexcept = default;

Key::~Key() = default;

void* Key::get() const { return impl_->get(); }

void Key::set(void* ptr) const { impl_->set(ptr); }

void Key::cleanup() noexcept { Impl::cleanup(); }

}  // namespace core::tls