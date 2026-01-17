#pragma once
/**
 * @file
 * @author  Rongbo Zhang
 * @date    2025-04-23
 * @brief   This file contains the definition of the LazyPopUniquePriorityQueue class.
 *
 *  The class LazyPopUniquePriorityQueue is a priority queue that allows for lazy deletion of elements.
 *  The elements are pair of key and sort-value. The key is a unique value to identify the item, and the sort-value is used to sort the item.
 *  It is implemented using a vector and 2 sets, one set keeps track of the elements in the queue, and the other set keeps track of the elements that are pending deletion, 
 *  so that they can be removed from the queue when they are popped.
 * 
 *  Currently, the class supports the following functions:
 *      LazyPopUniquePriorityQueue::push(): Pushes a key-sort-value (K-SV) pair into the priority queue and adds the key to the tracking set.
 *      LazyPopUniquePriorityQueue::pop(): Returns the K-SV pair with the highest SV whose key is not pending deletion.
 *      LazyPopUniquePriorityQueue::remove(): Removes an element from the priority queue immediately.
 *      LazyPopUniquePriorityQueue::remove_at_pop_time(): Removes an element from the priority queue when it is popped.
 *      LazyPopUniquePriorityQueue::empty(): Returns whether the queue is empty.
 *      LazyPopUniquePriorityQueue::clear(): Clears the priority queue vector and the tracking sets.
 *      LazyPopUniquePriorityQueue::size(): Returns the number of elements in the queue.
 *      LazyPopUniquePriorityQueue::contains(): Returns true if the key is in the queue, false otherwise.
 */

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

template<typename T_key, typename T_sort>
class LazyPopUniquePriorityQueue {
  public:
    /**  @brief The custom comparison struct for sorting the items in the priority queue.
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

    /// @brief The set to store the delete pending item from the queue referred by the key.
    std::unordered_set<T_key> delete_pending_set;

    /**
     * @brief Push the key and the sort value as a pair into the priority queue.
     *
     *  @param key
     *              The unique key for the item that will be pushed onto the queue.
     *  @param value
     *              The sort value used for sorting the item.
     */
    void push(T_key key, T_sort value) {
        // Insert the key and sort value pair into the queue if it is not already present
        if (content_set.find(key) != content_set.end()) {
            // If the key is already in the queue, do nothing
            return;
        }
        // Insert the key and sort value pair into the heap and track the key
        // The new item is added to the end of the vector and then the push_heap function is call
        // to push the item to the correct position in the heap structure.
        heap.emplace_back(key, value);
        std::push_heap(heap.begin(), heap.end(), LazyPopUniquePriorityQueueCompare());
        content_set.insert(key);
    }

    /**
     * @brief Pop the top item from the priority queue.
     *
     *  @return The key and sort value pair.
     */
    std::pair<T_key, T_sort> pop() {
        std::pair<T_key, T_sort> top_pair;
        while (heap.size() > 0) {
            top_pair = heap.front();
            // Remove the key from the heap and the tracking set.
            // The pop_heap function will move the top item in the heap structure to the end of the vector container.
            // Then the pop_back function will remove the last item.
            std::pop_heap(heap.begin(), heap.end(), LazyPopUniquePriorityQueueCompare());
            heap.pop_back();
            content_set.erase(top_pair.first);

            // Checking if the key with the highest sort value is in the delete pending set.
            // If it is, ignore the current top item and remove the key from the delete pending set. Then get the next top item.
            // Otherwise, the top item found, break the loop.
            if (delete_pending_set.find(top_pair.first) != delete_pending_set.end()) {
                delete_pending_set.erase(top_pair.first);
                top_pair = std::pair<T_key, T_sort>();
            } else {
                break;
            }
        }

        // If there is zero non-pending-delete item, clear the queue.
        if (empty()) {
            clear();
        }

        return top_pair;
    }

    /**
     * @brief Remove the item with matching key value from the priority queue
     *        This will immediately remove the item and re-heapify the queue.
     *        
     *        This function is expensive, as it requires a full re-heapify of the queue.
     *        The time complexity is O(n log n) for the re-heapify, where n is the size of the queue.
     *        It is recommended to use remove_at_pop_time() instead.
     *  @param key
     *             The key of the item to be delected from the queue.
     */
    void remove(T_key key) {
        // If the key is in the priority queue, remove it from the heap and reheapify.
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

            // If this delete caused the queue to have zero non-pending-delete item, clear the queue.
            if (empty()) {
                clear();
                // Otherwise re-heapify the queue
            } else {
                std::make_heap(heap.begin(), heap.end(), LazyPopUniquePriorityQueueCompare());
            }
        }
    }

    /**
     * @brief Remove the item with matching key value from the priority queue at pop time.
     *        Add the key to the delete pending set for tracking, 
     *        and it will be deleted when it is popped.
     *      
     *        This function will not immediately delete the key from the
     *        priority queue. It will be deleted when it is popped. Thus do not
     *        expect a size reduction in the priority queue immediately.
     *  @param key
     *             The key of the item to be delected from the queue at pop time.
     */
    void remove_at_pop_time(T_key key) {
        // If the key is in the list, start tracking it in the delete pending list.
        // Otherwise, do nothing.
        if (content_set.find(key) != content_set.end()) {
            delete_pending_set.insert(key);

            // If this marks the last non-pending-delete item as to-be-deleted, clear the queue
            if (empty()) {
                clear();
            }
        }
    }

    /**
     * @brief Check if the priority queue is empty, i.e. there is zero non-pending-delete item.
     *
     *  @return True if the priority queue is empty, false otherwise.
     */
    bool empty() {
        return size() == 0;
    }

    /**
     * @brief Clears the priority queue and the tracking sets.
     *
     *  @return None
     */
    void clear() {
        heap.clear();
        content_set.clear();
        delete_pending_set.clear();
    }

    /**
     * @brief Get the number of non-pending-delete items in the priority queue.
     *
     *  @return The number of non-pending-delete items in the priority queue.
     */
    size_t size() {
        return heap.size() - delete_pending_set.size();
    }

    /**
     * @brief Check if the item referred to the key is in the priority queue.
     *
     *  @param key
     *              The key of the item.
     *  @return True if the key is in the priority queue, false otherwise.
     */
    bool contains(T_key key) {
        return content_set.find(key) != content_set.end();
    }
};
