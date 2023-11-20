#include <iostream>
#include <mutex>

template <class T, int CAPACITY>
class circular_queue
{
private:
    T array_[CAPACITY] = {};
    size_t size_ = 0;

    T* begin_of_memory_;
    T* end_of_memory_;

    T* p_head_;
    T* p_tail_;

    std::mutex mutex_;

    std::condition_variable cv_;

    void move_head() { p_head_ + 1 < end_of_memory_ ? ++p_head_ : p_head_ = array_; }
    void move_tail() { p_tail_ + 1 < end_of_memory_ ? ++p_tail_ : p_tail_ = array_; }

public:
    circular_queue()
    {
        begin_of_memory_ = array_;
        end_of_memory_ = begin_of_memory_ + CAPACITY;

        p_head_ = array_;
        p_tail_ = array_;
    }

    circular_queue(std::initializer_list<T> init_list) : circular_queue()
    {
        set_size(init_list.size());

        int8_t index = 0;
        for (T element : init_list)
        {
            array_[index] = element;
            p_tail_ = &array_[index];

            ++index;
        }
    }

    circular_queue(const circular_queue& new_queue) : circular_queue()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        set_size(new_queue.size());

        int index;
        for (index = 0; index < new_queue.size(); ++index)
        {
            array_[index] = new_queue.array_[index];
        }

        p_tail_ = &array_[new_queue.p_head_ - new_queue.begin_of_memory_ - 1];
        p_tail_ + 1 < end_of_memory_ ? p_head_ = p_tail_ + 1 : p_head_ = array_;
    }

    circular_queue& operator=(circular_queue& other_queue)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (this == &other_queue) { return *this; }

        set_size(other_queue.size());

        T* p_head_other_queue = other_queue.p_head_;

        int i = 0;
        do
        {
            array_[i++] = other_queue.read_front();

            other_queue.move_head();
            other_queue.move_tail();
        }
        while (other_queue.p_head_ != p_head_other_queue);

        return *this;
    }

    T operator[](int index)
    {
        return array_[index];
    }

    size_t size()
    {
        return size_;
    }

    void set_size(size_t size)
    {
        size_ = size;
    }

    void push_back(T&& pushable)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock);

        if (size_ < CAPACITY)
        {
            ++size_;
            move_tail();
            *p_tail_ = pushable;

            return;
        }

        move_tail();
        *p_tail_ = pushable;
    }

    T read_front()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!size_) { return T(); }

        T head_element = *p_head_;
        move_head();

        lock.unlock();
        cv_.notify_one();
        
        return head_element;
    }
};

template <class T, int CAPACITY>
std::ostream& operator<<(std::ostream& stream, circular_queue<T, CAPACITY>& queue)
{
    for (size_t i = 0; i < queue.size(); ++i) { stream << queue[i] << "\t"; }
    stream << std::endl;

    return stream;
}