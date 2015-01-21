#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include "gears/base/common/atomic_ops.h"


template <typename Ty_>
class AtomicNode {
 public:
  template<typename U>
  friend class AtomicList;
  typedef AtomicNode<Ty_> my_type;

  explicit AtomicNode(const Ty_ &value = Ty_()) : next_(NULL), value_(value) {
  }

  Ty_ &get() {
    return value_;
  }

  const Ty_ &get() const{
    return value_;
  }

 private:
  Ty_ value_;
  my_type *next_;
};

//serve as a template typedef, we need it so BADLY!
template <typename Ty_>
class LazyDefaultValue : public LazySingleton<Ty_> {
};

//lock-free single linked list, which is suitable for read-much-write-and-remove-little usage
template <typename Ty_>
class AtomicList {
 private:
  //we've some restriction here:
  //1)Ty_ must has the same size of AtomicWord, or it can NOT be use with atomic_ops
  //2)Ty_ must NOT has a non-trivial copy-constructor/operator=, or a CompareAndSwap will break it's internal state
  //3)Ty_ must be default constructable, and with a default value, which will be used as empty indicator
  BOOST_STATIC_ASSERT((sizeof(Ty_) == sizeof(AtomicWord)));
  BOOST_STATIC_ASSERT((boost::has_trivial_copy<Ty_>::value && boost::has_trivial_assign<Ty_>::value));

 public:
  typedef AtomicNode<Ty_> node_type;
  typedef AtomicList<Ty_> my_type;
  typedef Ty_ value_type;
  typedef value_type *pointer;
  typedef value_type &reference;
  typedef const value_type *const_pointer;
  typedef const value_type &const_reference;

  class iterator {
   public:
    explicit iterator(node_type* value = NULL) : opaque_(value) {}
    iterator(const iterator &rhs) : opaque_(rhs.opaque_) {}
    iterator& operator=(const iterator &rhs) {
      opaque_ = rhs.opaque_;
      return *this;
    }

    const_reference operator*() const{
      if(opaque_) {
        return opaque_->get();
      } else {
        throw std::exception("out of range");
      }
    }

    const_pointer operator->() const{
      return (&**this);
    }

    iterator& operator++() {
      if(opaque_) {
        opaque_ = opaque_->next_;
        return *this;
      } else {
        throw std::exception("out of range");
      }
    }

    iterator operator++(int) {
      iterator tmp(*this);
      ++*this;
      return tmp;
    }

    bool operator==(const iterator &rhs) {
      return opaque_ == rhs.opaque_;
    }

    bool operator!=(const iterator &rhs) {
      return opaque_ != rhs.opaque_;
    }

    value_type dismiss() {
      //make sure we won't overwrite other's value
      //XXX:dismissed twice in different thread, with a reusing put insert between them, will cause problem
      return (value_type)CompareAndSwap((AtomicWord*)&opaque_->value_, default_value(), opaque_->value_);
    }

   private:
    node_type *opaque_;
  };

  AtomicList() : head_(NULL) {
  }

  iterator begin() {
    return iterator(head_);
  }

  static AtomicWord conv(AtomicWord foo) {
    return foo;
  }

  iterator put(const_reference value) {
    //first, try to find an empty node for it
    node_type *head = head_;
    while(head) {
      if(head->value_ == default_value()
         && CompareAndSwap((AtomicWord*)&head->value_, default_value(), value) == default_value()) {
        return iterator(head);
      }
      head = head->next_;
    }

    //if we find none, push it into front
    return push_front(value);
  }

  //an iterator with NULL ptr indicate the end of an AtomicList, and can NOT be dereference
  iterator end() {
    return LazyDefaultValue<iterator>::GetInstance();
  }


 private:
  iterator push_front(const_reference value) {
    node_type *node = new node_type(value);
    node_type *old = NULL;
    do {
      old = head_;
      node->next_ = old;
    } while(CompareAndSwap((AtomicWord*)&head_, (AtomicWord)old, (AtomicWord)node) != (AtomicWord)old);
    return iterator(node);
  }


  template<typename trival_constructable>
  struct Default_value_ {};

  template<>
  struct Default_value_<boost::true_type>{
    static value_type get() {
      return Ty_();
    }
  };

  struct die{};
  template<>
  struct Default_value_<boost::false_type> {
    static const_reference get() {
      return LazyDefaultValue<Ty_>::GetInstance();
    }
  };

  //I need the DAMN auto!
  //select suitable return value type
  //for trivial constructable one, we use value type
  //for non-trivial constructable types, we use lazy singleton and const reference
  typedef typename boost::mpl::if_<boost::has_trivial_constructor<Ty_>,
          Ty_,
          const_reference>::type default_value_type;

  //select best default value generate method, for those who is trivial constructable
  static default_value_type default_value() {
    return Default_value_<boost::has_trivial_constructor<Ty_>::type >::get();
  }

  node_type *head_;
};
