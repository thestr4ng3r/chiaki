/* 
* Not any company's property but Public-Domain
* Do with source-code as you will. No requirement to keep this
* header if need to use it/change it/ or do whatever with it
*
* Note that there is No guarantee that this code will work 
* and I take no responsibility for this code and any problems you
* might get if using it. 
*
* Code & platform dependent issues with it was originally 
* published at http://www.kjellkod.cc/threadsafecircularqueue
* 2012-16-19  @author Kjell Hedström, hedstrom@kjellkod.cc */

// should be mentioned the thinking of what goes where
// it is a "controversy" whether what is tail and what is head
// http://en.wikipedia.org/wiki/FIFO#Head_or_tail_first

#ifndef CIRCULARFIFO_AQUIRE_RELEASE_H_
#define CIRCULARFIFO_AQUIRE_RELEASE_H_

#include <atomic>
#include <cstddef>
namespace memory_relaxed_aquire_release {
template<typename Element, size_t Size> 
class CircularFifo{
public:
  enum { Capacity = Size+1 };

  CircularFifo() : _tail(0), _head(0){}   
  virtual ~CircularFifo() {}

  bool push(const Element& item); // pushByMOve?
  bool pop(Element& item);

  bool wasEmpty() const;
  bool wasFull() const;
  bool isLockFree() const;

private:
  size_t increment(size_t idx) const; 

  std::atomic <size_t>  _tail;  // tail(input) index
  Element    _array[Capacity];
  std::atomic<size_t>   _head; // head(output) index
};

template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::push(const Element& item)
{	
  const auto current_tail = _tail.load(std::memory_order_relaxed); 
  const auto next_tail = increment(current_tail); 
  if(next_tail != _head.load(std::memory_order_acquire))                           
  {	
    _array[current_tail] = item;
    _tail.store(next_tail, std::memory_order_release); 
    return true;
  }
  
  return false; // full queue

}


// Pop by Consumer can only update the head (load with relaxed, store with release)
//     the tail must be accessed with at least aquire
template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::pop(Element& item)
{
  const auto current_head = _head.load(std::memory_order_relaxed);  
  if(current_head == _tail.load(std::memory_order_acquire)) 
    return false; // empty queue

  item = _array[current_head]; 
  _head.store(increment(current_head), std::memory_order_release); 
  return true;
}

template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::wasEmpty() const
{
  // snapshot with acceptance of that this comparison operation is not atomic
  return (_head.load() == _tail.load()); 
}


// snapshot with acceptance that this comparison is not atomic
template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::wasFull() const
{
  const auto next_tail = increment(_tail.load()); // aquire, we dont know who call
  return (next_tail == _head.load());
}


template<typename Element, size_t Size>
bool CircularFifo<Element, Size>::isLockFree() const
{
  return (_tail.is_lock_free() && _head.is_lock_free());
}

template<typename Element, size_t Size>
size_t CircularFifo<Element, Size>::increment(size_t idx) const
{
  return (idx + 1) % Capacity;
}

} // memory_relaxed_aquire_release
#endif /* CIRCULARFIFO_AQUIRE_RELEASE_H_ */
