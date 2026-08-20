/**
* implement a container like std::map
*/
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
   class Key,
   class T,
   class Compare = std::less <Key>
   > class map {
  public:
   /**
   * the internal type of data.
   * it should have a default constructor, a copy constructor.
   * You can use sjtu::map as value_type by typedef.
    */
   typedef pair<const Key, T> value_type;

  private:
   // AVL Tree Node
   struct Node {
       value_type *data;
       Node *left;
       Node *right;
       Node *parent;
       int height;

       Node() : data(nullptr), left(nullptr), right(nullptr), parent(nullptr), height(1) {}

       Node(const value_type &val, Node *p = nullptr)
           : left(nullptr), right(nullptr), parent(p), height(1) {
           data = static_cast<value_type *>(operator new(sizeof(value_type)));
           new (data) value_type(val);
       }

       ~Node() {
           if (data) {
               data->~value_type();
               operator delete(data);
           }
       }
   };

   // Sentinel node for end()
   Node *root;
   Node *sentinel;
   Compare comp;
   size_t _size;

   // Helper functions
   int getHeight(Node *node) const {
       return node ? node->height : 0;
   }

   int getBalance(Node *node) const {
       return node ? getHeight(node->left) - getHeight(node->right) : 0;
   }

   void updateHeight(Node *node) {
       if (node) {
           int leftHeight = getHeight(node->left);
           int rightHeight = getHeight(node->right);
           node->height = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
       }
   }

   Node *rightRotate(Node *y) {
       Node *x = y->left;
       Node *T2 = x->right;

       // Update parents
       x->parent = y->parent;
       if (y->parent != nullptr) {
           if (y == y->parent->left)
               y->parent->left = x;
           else
               y->parent->right = x;
       }
       y->parent = x;
       if (T2 != nullptr)
           T2->parent = y;

       // Rotate
       x->right = y;
       y->left = T2;

       // Update heights
       updateHeight(y);
       updateHeight(x);

       return x;
   }

   Node *leftRotate(Node *x) {
       Node *y = x->right;
       Node *T2 = y->left;

       // Update parents
       y->parent = x->parent;
       if (x->parent != nullptr) {
           if (x == x->parent->left)
               x->parent->left = y;
           else
               x->parent->right = y;
       }
       x->parent = y;
       if (T2 != nullptr)
           T2->parent = x;

       // Rotate
       y->left = x;
       x->right = T2;

       // Update heights
       updateHeight(x);
       updateHeight(y);

       return y;
   }

   Node *rebalance(Node *node) {
       updateHeight(node);
       int balance = getBalance(node);

       // Left Left Case
       if (balance > 1 && getBalance(node->left) >= 0)
           return rightRotate(node);

       // Left Right Case
       if (balance > 1 && getBalance(node->left) < 0) {
           node->left = leftRotate(node->left);
           return rightRotate(node);
       }

       // Right Right Case
       if (balance < -1 && getBalance(node->right) <= 0)
           return leftRotate(node);

       // Right Left Case
       if (balance < -1 && getBalance(node->right) > 0) {
           node->right = rightRotate(node->right);
           return leftRotate(node);
       }

       return node;
   }

   Node *findMin(Node *node) const {
       while (node->left != nullptr)
           node = node->left;
       return node;
   }

   Node *findMax(Node *node) const {
       while (node->right != nullptr)
           node = node->right;
       return node;
   }

   Node *findNode(const Key &key, Node *node) const {
       while (node != nullptr) {
           if (comp(key, node->data->first))
               node = node->left;
           else if (comp(node->data->first, key))
               node = node->right;
           else
               return node;
       }
       return nullptr;
   }

   Node *insertHelper(Node *node, const value_type &val, Node *parent, bool &inserted) {
       if (node == nullptr) {
           inserted = true;
           _size++;
           return new Node(val, parent);
       }

       if (comp(val.first, node->data->first)) {
           node->left = insertHelper(node->left, val, node, inserted);
       } else if (comp(node->data->first, val.first)) {
           node->right = insertHelper(node->right, val, node, inserted);
       } else {
           inserted = false;
           return node;
       }

       return rebalance(node);
   }

   Node *removeNode(Node *node, const Key &key, bool &removed, bool decrementSize = true) {
       if (node == nullptr)
           return nullptr;

       if (comp(key, node->data->first)) {
           node->left = removeNode(node->left, key, removed, decrementSize);
           if (node->left) node->left->parent = node;
       } else if (comp(node->data->first, key)) {
           node->right = removeNode(node->right, key, removed, decrementSize);
           if (node->right) node->right->parent = node;
       } else {
           if (decrementSize) {
               removed = true;
               _size--;
           }

           if (node->left == nullptr || node->right == nullptr) {
               Node *temp = node->left ? node->left : node->right;

               if (temp == nullptr) {
                   // No children
                   delete node;
                   return nullptr;
               } else {
                   // One child
                   Node *parent = node->parent;
                   temp->parent = parent;
                   delete node;
                   return temp;
               }
           } else {
               // Two children: get inorder successor
               Node *temp = findMin(node->right);

               // Swap data pointers
               value_type *tempData = node->data;
               node->data = temp->data;
               temp->data = tempData;

               // Remove the successor (don't decrement size again)
               node->right = removeNode(node->right, temp->data->first, removed, false);
               if (node->right) node->right->parent = node;
           }
       }

       return rebalance(node);
   }

   void destroyTree(Node *node) {
       if (node == nullptr)
           return;
       destroyTree(node->left);
       destroyTree(node->right);
       delete node;
   }

   Node *copyTree(Node *node, Node *parent) {
       if (node == nullptr)
           return nullptr;

       Node *newNode = new Node(*(node->data), parent);
       newNode->height = node->height;
       newNode->left = copyTree(node->left, newNode);
       newNode->right = copyTree(node->right, newNode);
       return newNode;
   }

  public:
   /**
   * see BidirectionalIterator at CppReference for help.
   *
   * if there is anything wrong throw invalid_iterator.
   *     like it = map.begin(); --it;
   *       or it = map.end(); ++end();
    */
   class const_iterator;
   class iterator {
      private:
       Node *node;
       const map *container;

       friend class map;
       friend class const_iterator;

       bool isSentinel() const {
           return node == container->sentinel;
       }

      public:
       iterator() : node(nullptr), container(nullptr) {}

       iterator(Node *n, const map *m) : node(n), container(m) {}

       iterator(const iterator &other) : node(other.node), container(other.container) {}

       /**
       * TODO iter++
        */
       iterator operator++(int) {
           iterator temp = *this;
           ++(*this);
           return temp;
       }

       /**
       * TODO ++iter
        */
       iterator &operator++() {
           if (node == nullptr)
               throw invalid_iterator();

           if (isSentinel())
               throw invalid_iterator();

           if (node->right != nullptr) {
               node = container->findMin(node->right);
           } else {
               Node *p = node->parent;
               while (p != nullptr && node == p->right) {
                   node = p;
                   p = p->parent;
               }
               if (p == nullptr) {
                   node = container->sentinel;
               } else {
                   node = p;
               }
           }
           return *this;
       }

       /**
       * TODO iter--
        */
       iterator operator--(int) {
           iterator temp = *this;
           --(*this);
           return temp;
       }

       /**
       * TODO --iter
        */
       iterator &operator--() {
           if (node == nullptr)
               throw invalid_iterator();

           if (isSentinel()) {
               if (container->root == nullptr)
                   throw invalid_iterator();
               node = container->findMax(container->root);
           } else if (node->left != nullptr) {
               node = container->findMax(node->left);
           } else {
               Node *p = node->parent;
               while (p != nullptr && node == p->left) {
                   node = p;
                   p = p->parent;
               }
               if (p == nullptr)
                   throw invalid_iterator();
               node = p;
           }
           return *this;
       }

       /**
       * a operator to check whether two iterators are same (pointing to the same memory).
        */
       value_type &operator*() const {
           if (node == nullptr || isSentinel())
               throw invalid_iterator();
           return *(node->data);
       }

       bool operator==(const iterator &rhs) const {
           return node == rhs.node && container == rhs.container;
       }

       bool operator==(const const_iterator &rhs) const {
           return node == rhs.node && container == rhs.container;
       }

       /**
       * some other operator for iterator.
        */
       bool operator!=(const iterator &rhs) const {
           return !(*this == rhs);
       }

       bool operator!=(const const_iterator &rhs) const {
           return !(*this == rhs);
       }

       /**
       * for the support of it->first.
       * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
        */
       value_type *operator->() const
           noexcept {
           if (node == nullptr || isSentinel())
               return nullptr;
           return node->data;
       }
   };

   class const_iterator {
       Node *node;
       const map *container;

       friend class map;
       friend class iterator;

       bool isSentinel() const {
           return node == container->sentinel;
       }

      public:
       const_iterator() : node(nullptr), container(nullptr) {}

       const_iterator(Node *n, const map *m) : node(n), container(m) {}

       const_iterator(const const_iterator &other) : node(other.node), container(other.container) {}

       const_iterator(const iterator &other) : node(other.node), container(other.container) {}

       const_iterator operator++(int) {
           const_iterator temp = *this;
           ++(*this);
           return temp;
       }

       const_iterator &operator++() {
           if (node == nullptr)
               throw invalid_iterator();

           if (isSentinel())
               throw invalid_iterator();

           if (node->right != nullptr) {
               node = container->findMin(node->right);
           } else {
               Node *p = node->parent;
               while (p != nullptr && node == p->right) {
                   node = p;
                   p = p->parent;
               }
               if (p == nullptr) {
                   node = container->sentinel;
               } else {
                   node = p;
               }
           }
           return *this;
       }

       const_iterator operator--(int) {
           const_iterator temp = *this;
           --(*this);
           return temp;
       }

       const_iterator &operator--() {
           if (node == nullptr)
               throw invalid_iterator();

           if (isSentinel()) {
               if (container->root == nullptr)
                   throw invalid_iterator();
               node = container->findMax(container->root);
           } else if (node->left != nullptr) {
               node = container->findMax(node->left);
           } else {
               Node *p = node->parent;
               while (p != nullptr && node == p->left) {
                   node = p;
                   p = p->parent;
               }
               if (p == nullptr)
                   throw invalid_iterator();
               node = p;
           }
           return *this;
       }

       const value_type &operator*() const {
           if (node == nullptr || isSentinel())
               throw invalid_iterator();
           return *(node->data);
       }

       bool operator==(const const_iterator &rhs) const {
           return node == rhs.node && container == rhs.container;
       }

       bool operator==(const iterator &rhs) const {
           return node == rhs.node && container == rhs.container;
       }

       bool operator!=(const const_iterator &rhs) const {
           return !(*this == rhs);
       }

       bool operator!=(const iterator &rhs) const {
           return !(*this == rhs);
       }

       const value_type *operator->() const
           noexcept {
           if (node == nullptr || isSentinel())
               return nullptr;
           return node->data;
       }
   };

   /**
   * TODO two constructors
    */
   map() : root(nullptr), _size(0) {
       sentinel = new Node();
   }

   map(const map &other) : root(nullptr), _size(0) {
       sentinel = new Node();
       root = copyTree(other.root, nullptr);
       _size = other._size;
   }

   /**
   * TODO assignment operator
    */
   map &operator=(const map &other) {
       if (this == &other)
           return *this;

       clear();
       destroyTree(root);
       root = copyTree(other.root, nullptr);
       _size = other._size;
       return *this;
   }

   /**
   * TODO Destructors
    */
   ~map() {
       destroyTree(root);
       delete sentinel;
   }

   /**
   * TODO
   * access specified element with bounds checking
   * Returns a reference to the mapped value of the element with key equivalent to key.
   * If no such element exists, an exception of type `index_out_of_bound'
    */
   T &at(const Key &key) {
       Node *node = findNode(key, root);
       if (node == nullptr)
           throw index_out_of_bound();
       return node->data->second;
   }

   const T &at(const Key &key) const {
       Node *node = findNode(key, root);
       if (node == nullptr)
           throw index_out_of_bound();
       return node->data->second;
   }

   /**
   * TODO
   * access specified element
   * Returns a reference to the value that is mapped to a key equivalent to key,
   *   performing an insertion if such key does not already exist.
    */
   T &operator[](const Key &key) {
       Node *node = findNode(key, root);
       if (node == nullptr) {
           // Use insert to create the element
           auto result = insert(value_type(key, T()));
           return result.first->second;
       }
       return node->data->second;
   }

   /**
   * behave like at() throw index_out_of_bound if such key does not exist.
    */
   const T &operator[](const Key &key) const {
       return at(key);
   }

   /**
   * return a iterator to the beginning
    */
   iterator begin() {
       if (root == nullptr)
           return end();
       return iterator(findMin(root), this);
   }

   const_iterator cbegin() const {
       if (root == nullptr)
           return cend();
       return const_iterator(findMin(root), this);
   }

   /**
   * return a iterator to the end
   * in fact, it returns past-the-end.
    */
   iterator end() {
       return iterator(sentinel, this);
   }

   const_iterator cend() const {
       return const_iterator(sentinel, this);
   }

   /**
   * checks whether the container is empty
   * return true if empty, otherwise false.
    */
   bool empty() const {
       return _size == 0;
   }

   /**
   * returns the number of elements.
    */
   size_t size() const {
       return _size;
   }

   /**
   * clears the contents
    */
   void clear() {
       destroyTree(root);
       root = nullptr;
       _size = 0;
   }

   /**
   * insert an element.
   * return a pair, the first of the pair is
   *   the iterator to the new element (or the element that prevented the insertion),
   *   the second one is true if insert successfully, or false.
    */
   pair<iterator, bool> insert(const value_type &value) {
       Node *existing = findNode(value.first, root);
       if (existing != nullptr) {
           return pair<iterator, bool>(iterator(existing, this), false);
       }

       bool inserted = false;
       root = insertHelper(root, value, nullptr, inserted);

       // Update root's parent if root changed
       if (root != nullptr)
           root->parent = nullptr;

       Node *newNode = findNode(value.first, root);
       return pair<iterator, bool>(iterator(newNode, this), inserted);
   }

   /**
   * erase the element at pos.
   *
   * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
    */
   void erase(iterator pos) {
       if (pos.node == nullptr || pos.node == sentinel || pos.container != this)
           throw invalid_iterator();

       // Verify the node is actually in this map
       Node *check = findNode(pos.node->data->first, root);
       if (check != pos.node)
           throw invalid_iterator();

       bool removed = false;
       root = removeNode(root, pos.node->data->first, removed);

       // Update root's parent if root changed
       if (root != nullptr)
           root->parent = nullptr;
   }

   /**
   * Returns the number of elements with key
   *   that compares equivalent to the specified argument,
   *   which is either 1 or 0
   *     since this container does not allow duplicates.
   * The default method of check the equivalence is !(a < b || b > a)
    */
   size_t count(const Key &key) const {
       return findNode(key, root) ? 1 : 0;
   }

   /**
   * Finds an element with key equivalent to key.
   * key value of the element to search for.
   * Iterator to an element with key equivalent to key.
   *   If no such element is found, past-the-end (see end()) iterator is returned.
    */
   iterator find(const Key &key) {
       Node *node = findNode(key, root);
       if (node == nullptr)
           return end();
       return iterator(node, this);
   }

   const_iterator find(const Key &key) const {
       Node *node = findNode(key, root);
       if (node == nullptr)
           return cend();
       return const_iterator(node, this);
   }
};

}

#endif
