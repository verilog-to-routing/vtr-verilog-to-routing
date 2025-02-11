#ifndef VPR_LAZY_POP_UNIQUE_PRIORITY_QUEUE_H
#define VPR_LAZY_POP_UNIQUE_PRIORITY_QUEUE_H

#include <unordered_set>
#include <vector>
#include <algorithm>

/**
 * @brief Lazy Pop Unique Priority Queue
 *
 * This is a priority queue that is used to sort items which are identified by the key  
 * and sorted by the sort value.
 * 
 * It uses a vector to store the key and sort value pair. 
 * It uses a set to store the keys that are in the vector for uniqueness checking
 * and a set to store the delete pending keys which will be removed at pop time.
 */
template <typename T_key, typename T_sort>
class LazyPopUniquePriorityQueue {
    public:
        /**  @brief The custom comparsion struct for sorting the items in the priority queue.
         *          A less than comparison will put the item with the highest sort value to the front of the queue.
         *          A greater than comparison will put the item with the lowest sort value to the front of the queue.
         */
        struct LazyPopUniquePriorityQueueCompare {
            bool operator()(const std::pair<T_key, T_sort>& a,
                            const std::pair<T_key, T_sort>& b) const {
                return a.second < b.second;
            }
        };

        /// @brief The vector maintained as heap to store the key and sort value pair.
        std::vector<std::pair<T_key, T_sort>> heap;

        /// @brief The set to store the keys that are in the queue. This is used to ensure uniqueness
        std::unordered_set<T_key> content_set;

        /// @brief The set to store the delete pending item from the queue refered by the key.
        std::unordered_set<T_key> delete_pending_set;

        /**
         * @brief Push the key and the sort value as a pair into the priority queue.
         *
         *  @param key
         *              The unique key for the item that will be pushed onto the queue.
         *  @param value
         *              The sort value used for sorting the item.
         */
        void push(T_key key, T_sort value){
            // Insert the key and sort value pair into the queue if it is not already present
            if (content_set.find(key) != content_set.end()) {
                // If the key is already in the queue, do nothing
                return;
            }
            heap.emplace_back(key, value);
            std::push_heap(heap.begin(),
                           heap.end(),
                           LazyPopUniquePriorityQueueCompare());
                           content_set.insert(key);
        }

        /**
         * @brief Pop the top item from the priority queue.
         *
         *  @return The key and sort value pair.
         */
        std::pair<T_key,T_sort> pop(){
            std::pair<T_key, T_sort> top_pair;
            while (heap.size() > 0) {
                top_pair = heap.front();
                // remove the key from the heap and the tracking set
                std::pop_heap(heap.begin(),
                              heap.end(),
                              LazyPopUniquePriorityQueueCompare());
                              heap.pop_back();
                              content_set.erase(top_pair.first);
        
                // checking if the highest value's key is in the delete pending set
                // if it is, remove it from the delete pending set and find the next best gain's key
                if (delete_pending_set.find(top_pair.first) != delete_pending_set.end()) {
                    delete_pending_set.erase(top_pair.first);
                    top_pair = std::pair<T_key, T_sort>();
                } else {
                    break;
                }
            }
            return top_pair;
        }

        /**
         * @brief Remove the item with matching key value from the priority queue
         *        This fill immediately remove the item and re-heapify the queue.
         *
         *  @param key
         *             The key of the item to be delected from the queue
         */
        void remove(T_key key){
            // If the key is in the priority queue, remove it from the heap and reheapify
            // Otherwise, do nothing.
            if (content_set.find(key) != content_set.end()) {
                content_set.erase(key);
                delete_pending_set.erase(key);
                for (int i = 0; i < heap.size(); i++) {
                    if (heap[i].first == key) {
                        heap.erase(heap.begin() + i);
                        break;
                    }
                }
                std::make_heap(heap.begin(), heap.end(), LazyPopUniquePriorityQueueCompare());
            }
        }
        

        /**
         * @brief Remove the item with matching key value from the priority queue at pop time.
         *        Add the key to the delete pending set for tracking, 
         *        and it will be deleted when it is popped.
         *      
         *       This function will not immediately delete the key from the
         *       priority queue. It will be deleted when it is popped. Thus do not
         *       expect a size reduction in the priority queue immediately.
         *  @param key
         *             The key of the item to be delected from the queue at pop time.
         */
        void remove_at_pop_time(T_key key){
            //if the key is in the list, start tracking it in the delete pending list.
            // Otherwise, do nothing.
            if (content_set.find(key) != content_set.end()) {
                delete_pending_set.insert(key);
            } 
        }

        /**
         * @brief Check if the priority queue is empty.
         *
         *  @return True if the priority queue is empty, false otherwise.
         */
        bool empty(){
            return heap.empty();
        }

        /**
         * @brief Clears the priority queue and the tracking sets.
         *
         *  @return None
         */
        void clear(){
            heap.clear();
            content_set.clear();
            delete_pending_set.clear();
        }

        /**
         * @brief Get the size of the priority queue.
         *
         *  @return The size of the priority queue.
         */
        size_t size(){
            return heap.size();
        }

        /**
         * @brief Check if the item refered by the key is in the priority queue.
         *
         *  @param key
         *              The key of the item.
         *  @return True if the key is in the priority queue, false otherwise.
         */
        bool contains(T_key key){
            return content_set.find(key) != content_set.end();
        }
};

#endif