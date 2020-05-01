#ifndef VTR_SET_H
#define VTR_SET_H

#include <vector>

namespace vtr {

template<typename T>
class vec_id_set {
    public:
        typedef typename std::vector<T>::const_iterator const_iterator;
        typedef const_iterator iterator;

        auto begin() const { return vec_.begin(); }
        auto end() const { return vec_.end(); }

        auto cbegin() const { return vec_.cbegin(); }
        auto cend() const { return vec_.cend(); }

        bool insert(T val) {
            if (count(val)) {
                return false;
            }

            vec_.push_back(val);
            if (size_t(val) >= contained_.size()) {
                contained_.resize(size_t(val)+1, false);
            }
            contained_[size_t(val)] = true;
            return true;
        }

        template<typename Iter>
        void insert(Iter first, Iter last) {
            size_t nelem = std::distance(first, last);
            vec_.reserve(size() + nelem);
            contained_.reserve(size() + nelem);

            for (Iter itr = first; itr != last; ++itr) {
                insert(*itr);
            }
        }


        size_t count(T val) const {
            if (size_t(val) < contained_.size()) {
                return contained_[size_t(val)];
            }
            return 0;
        }

        size_t size() const {
            return vec_.size();
        }

        void sort() {
            std::sort(vec_.begin(), vec_.end());
        }

        void clear() {
            vec_.clear();
            contained_.clear();
        }

    private:
        std::vector<T> vec_;
        std::vector<bool> contained_;
};

} //namespace

#endif
