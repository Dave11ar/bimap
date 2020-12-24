#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include "splay_tree.h"

template <typename Left, typename Right,
    typename CompareLeft = std::less<Left>, typename CompareRight = std::less<Right>>
struct bimap {
  using left_t = Left;
  using right_t = Right;

private:
  using splay_tree_t = splay_tree<left_t, right_t>;

  static constexpr node<left_tag, left_t>* (*get_node_l)(splay_tree_t*) =
      &get_node<left_tag, left_t, right_t, left_t>;
  static constexpr node<right_tag, right_t>* (*get_node_r)(splay_tree_t*) =
      &get_node<right_tag, left_t, right_t, right_t>;

  static constexpr splay_tree_t *(*get_splay_l)(node<left_tag, left_t >*) =
    &get_splay<left_tag, left_t, right_t>;
  static constexpr splay_tree_t *(*get_splay_r)(node<right_tag, right_t >*) =
    &get_splay<right_tag, left_t, right_t>;

  template <typename Tag, typename T>
  bool less(T const &a, T const &b) const {
    if constexpr (std::is_same_v<Tag, left_tag>) {
      return compare_left(a, b);
    } else {
      return compare_right(a, b);
    }
  }

  template <typename Tag, typename T>
  bool equal(T const &a, T const &b) const {
    return !less<Tag>(a, b) && !less<Tag>(b, a);
  }

  /**
   * invariant for all functions, except split: tree_left and tree_right stay correct,
   */


  /**
   * @return node with equal value if tree with root t, contains it, nullptr otherwise
   */
  template <typename Tag, typename T>
  node<Tag, T> *find(node<Tag, T> *t, T const &value) const {
    if (!t) {
      return t;
    }

    if (equal<Tag>(t->value, value)) {
      return set_tree_root(t);
    }

    if (less<Tag, T>(t->value, value)) {
      if (!t->right) {
        return nullptr;
      }
      return find(t->right, value);
    } else {
      if (!t->left) {
        return nullptr;
      }
      return find(t->left, value);
    }
  }

  /**
   * @return was element inserted or not
   */
  template <typename Tag, typename T>
  bool insert(node<Tag, T> *new_node) const {
    node<Tag, T> *t = get_root<Tag, T>();

    if (!t) {
      set_tree_root(new_node);
      return true;
    }

    std::pair<node<Tag, T>*, node<Tag, T>*> tmp = split<Tag>(new_node->value);

    if (tmp.first && equal<Tag>(tmp.first->value, new_node->value)) {
      merge(tmp.first, tmp.second);
      return false;
    }

    new_node->left = tmp.first;
    new_node->right = tmp.second;

    if (new_node->left) {
      new_node->left->parent = new_node;
    }
    if (new_node->right) {
      new_node->right->parent = new_node;
    }

    set_tree_root(new_node);
    return true;
  }

  /**
 * @return was element removed or not
 */
  template <typename Tag, typename T>
  bool remove(T const &value) const {
    node<Tag, T> *t = get_root<Tag, T>();

    if (!t) {
      return false;
    }

    t = find(t, value);
    if (!equal<Tag>(t->value, value)) {
      return false;
    }

    if (t->left) {
      t->left->parent = nullptr;
    }
    if (t->right) {
      t->right->parent = nullptr;
    }

    merge(t->left, t->right);
    return true;
  }

  /**
   * @return next element or nullptr if element is end()
   */
  template <typename Tag, typename T>
  node<Tag, T>* next(node<Tag, T> *t) const {
    if (!t) {
      return t;
    }

    t = set_tree_root(t);

    if (!t->right) {
      return nullptr;
    }

    node<Tag, T> *tmp = t->right;
    while (tmp->left) {
      tmp = tmp->left;
    }

    return set_tree_root(tmp);
  }

  /**
   * @return if there is node with node->value > value,
   * return it, nullptr otherwise
   */
  template <typename Tag, typename T>
  node<Tag, T> *next(T const &value) const {
    node<Tag, T> *t = get_root<Tag, T>();

    if (!t) {
      return t;
    }

    while (true) {
      if (less<Tag, T>(t->value, value) || equal<Tag>(t->value, value)) {
        if (!t->right) {
          return next<Tag>(t);
        }
        t = t->right;
      } else {
        if (!t->left) {
          return set_tree_root(t);
        }
        t = t->left;
      }
    }
  }

  template <typename Tag, typename T>
  std::pair<node<Tag, T>*, node<Tag, T>*> prev(node<Tag, T> *t) const {
    if (!t) {
      return t;
    }

    t = set_tree_root(t);

    if (!t->left) {
      return nullptr;
    }

    node<Tag, T> *tmp = t->left;
    while (tmp->right) {
      tmp = tmp->right;
    }

    return set_tree_root(tmp);
  }

  template <typename Tag, typename T>
  void destroy(node<Tag, T> *t) {
    if (!t) {
      return;
    }

    destroy(t->left);
    destroy(t->right);
    delete get_splay<Tag, left_t, right_t>(t);
  }

  template <typename Tag, typename T>
  node<Tag, T> *zig(node<Tag, T> *t) const{
    if (less<Tag>(t->value, t->parent->value)) {
      t->parent->left = t->right;
      if (t->right) {
        t->right->parent = t->parent;
      }

      t->right = t->parent;
    } else {
      t->parent->right = t->left;
      if (t->left) {
        t->left->parent = t->parent;
      }

      t->left = t->parent;
    }

    node<Tag, T> *tmp = t->parent->parent;
    t->parent->parent = t;
    t->parent = tmp;

    return t;
  }

  template <typename Tag, typename T>
  node<Tag, T> *splay(node<Tag, T> *t) const {
    if (!t || !t->parent) {
      return get_root<Tag, T>() = t;
    }
    if (!t->parent->parent) {
      return get_root<Tag, T>() = zig(t);
    }

    bool t_to_p = less<Tag>(t->value, t->parent->value);
    bool p_to_pp = less<Tag>(t->parent->value, t->parent->parent->value);
    if (t_to_p == p_to_pp) {
      t->parent = zig(t->parent);
      t = zig(t);
    } else {
      t = zig(t);
      t = zig(t);
    }

    return splay(t);
  }

  /**
   * @return two trees, all elements in first tree <= value
   */
  template <typename Tag, typename T>
  std::pair<node<Tag, T>*, node<Tag, T>*> split(T const &value) const {
    node<Tag, T> *t = next<Tag>(value);
    if (!t) {
      return {get_root<Tag, T>(), t};
    }

    node<Tag, T> *left_new = t->left;
    t->left = nullptr;

    if (left_new) {
      left_new->parent = nullptr;
    }
    return {left_new, t};
  }

  template <typename Tag, typename T>
  node<Tag, T> *find_max(node<Tag, T> *t) const {
    while (t->right) {
      t = t->right;
    }

    return set_tree_root(t);
  }

  template <typename Tag, typename T>
  node<Tag, T> *find_min(node<Tag, T> *t) const {
    while (t->left) {
      t = t->left;
    }

    return set_tree_root(t);
  }

  template <typename Tag, typename T>
  void merge(node<Tag, T> *a, node<Tag, T> *b) const {
    if (!a) {
      set_tree_root(b);
      return;
    }
    if (!b) {
      set_tree_root(a);
      return;
    }

    a = find_max(a);
    a->right = b;
    b->parent = a;
  }

  template <typename Tag, typename T>
  struct iterator {
    // Элемент на который сейчас ссылается итератор.
    // Разыменование итератора end_left() неопределено.
    // Разыменование невалидного итератора неопределено.
    T const &operator*() const {
      return tree->value;
    }

    // Переход к следующему по величине left'у.
    // Инкремент итератора end_left() неопределен.
    // Инкремент невалидного итератора неопределен.
    iterator &operator++() {
      tree = bmp->next<Tag, T>(tree);
      return *this;
    }
    iterator operator++(int) {
      node<Tag, T> *old = tree;
      ++*this;

      return {old, bmp};
    }

    // Переход к предыдущему по величине left'у.
    // Декремент итератора begin_left() неопределен.
    // Декремент невалидного итератора неопределен.
    iterator &operator--() {
      tree = bmp->prev<Tag, T>(tree);
      return *this;
    }
    iterator operator--(int) {
      node<Tag, T> *old = tree;
      --*this;

      return {old, bmp};
    }

    // left_iterator ссылается на левый элемент некоторой пары.
    // Эта функция возвращает итератор на правый элемент той же пары.
    // end_left().flip() возращает end_right().
    // end_right().flip() возвращает end_left().
    // flip() невалидного итератора неопределен.

    auto flip() const {
      if constexpr (std::is_same_v<Tag, left_tag>) {
        return iterator<right_tag, right_t>(get_opposite<left_tag>(tree), bmp);
      } else {
        return iterator<left_tag, left_t>(get_opposite<right_tag>(tree), bmp);
      }
    }

    bool operator==(iterator<Tag, T> const& other) const {
      return tree == other.tree;
    }
    bool operator!=(iterator<Tag, T> const& other) const {
      return tree != other.tree;
    }

    iterator(node<Tag, T> *tree, bimap const *bmp) : tree(tree), bmp(bmp) {};
    iterator(iterator const &other) : tree(other.tree), bmp(other.bmp) {};


    /**
     * if tree == nullptr, then iterator = end()
     */
  private:
    friend bimap;
    node<Tag, T> *tree;
    bimap const *bmp;
  };

public:
  /**
   * interface of bimap
   */
  using left_iterator = iterator<left_tag, left_t>;
  using right_iterator = iterator<right_tag, right_t>;

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight())
      : tree_left(nullptr), tree_right(nullptr),
        compare_left(compare_left), compare_right(compare_right), tree_size(0) {}

  // Конструкторы от других и присваивания
  bimap(bimap const &other) : tree_left(nullptr), tree_right(nullptr),
    compare_left(other.compare_left), compare_right(other.compare_right), tree_size(other.tree_size) {
    for (left_iterator it = other.begin_left(); it != other.end_left(); it++) {
      auto *tmp = new splay_tree_t(*it, other.at_left(*it));
      insert_both_trees(tmp);
    }
  }
  bimap(bimap &&other) noexcept
      : tree_left(other.tree_left), tree_right(other.tree_right),
        compare_left(std::move(other.compare_left)),
        compare_right(std::move(other.compare_right)),
        tree_size(other.tree_size) {}

  bimap &operator=(bimap const &other) {
    if (this == &other) {
      return *this;
    }

    bimap tmp(other);
    swap(tmp);

    return *this;
  }
  bimap &operator=(bimap &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    swap(other);
    return *this;
  }

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    destroy(tree_left);
  }

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(left_t const &left, right_t const &right) {
    return !contains(left, right) ? insert_operation<left_tag, left_t>(
               new splay_tree_t(left, right)) : end_left();
  }
  left_iterator insert(left_t const &left, right_t &&right) {
    return !contains(left, right) ? insert_operation<left_tag, left_t>(
               new splay_tree_t(left, std::move(right))) : end_left();
  }
  left_iterator insert(left_t &&left, right_t const &right) {
    return !contains(left, right) ? insert_operation<left_tag, left_t>(
               new splay_tree_t(std::move(left), right)) : end_left();
  }
  left_iterator insert(left_t &&left, right_t &&right) {
    return !contains(left, right) ? insert_operation<left_tag, left_t>(
               new splay_tree_t(std::move(left), std::move(right))) : end_left();
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    splay(it.tree);

    splay_tree_t *tmp = get_splay_l(it.tree);
    node<left_tag, left_t> *nxt = next(tree_left);

    remove<left_tag>(*it);
    remove<right_tag>(get_node_r(tmp)->value);

    tree_size--;
    delete tmp;

    return left_iterator(nxt, this);
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const &left) {
    node<left_tag, left_t> *t = find(tree_left, left);
    if (!t || !equal<left_tag>(t->value, left)) {
      return false;
    }

    erase_left(left_iterator(t, this));
    return true;
  }

  right_iterator erase_right(right_iterator it) {
    splay(it.tree);

    splay_tree_t *tmp = get_splay_r(it.tree);
    node<right_tag, right_t> *nxt = next(tree_right);

    remove<left_tag>(get_node_l(tmp)->value);
    remove<right_tag>(*it);

    tree_size--;
    delete tmp;

    return right_iterator(nxt, this);
  }
  bool erase_right(right_t const &right) {
    node<right_tag, right_t> *t = find(tree_right, right);
    if (!t || !equal<right_tag>(t->value, right)) {
      return false;
    }

    erase_right(right_iterator(t, this));
    return true;
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    while(first != last) {
      first = erase_left(first);
    }

    return last;
  }
  right_iterator erase_right(right_iterator first, right_iterator last) {
    while(first != last) {
      first = erase_right(first);
    }

    return last;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const &left) const {
    node<left_tag, left_t> *t = find(tree_left, left);
    if (t && equal<left_tag>(t->value, left)) {
      return left_iterator(t, this);
    } else {
      return end_left();
    }
  }
  right_iterator find_right(right_t const &right) const {
    node<right_tag, right_t> *t = find(tree_right, right);
    if (t && equal<right_tag>(t->value, right)) {
      return right_iterator(t, this);
    } else {
      return end_right();
    }
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const &at_left(left_t const &key) const {
    find<left_tag, left_t>(tree_left, key);

    node<right_tag, right_t> *opposite_node =
        get_node_r(get_splay_l(tree_left));
    if (tree_left && equal<left_tag>(tree_left->value, key)) {
      return opposite_node->value;
    }
    throw std::out_of_range("bimap::at_left - no such element");
  }

  left_t const &at_right(right_t const &key) const {
    find<right_tag, right_t>(tree_right, key);

    node<left_tag, left_t> *opposite_node =
        get_node_l(get_splay_r(tree_right));
    if (tree_right && equal<right_tag>(tree_right->value, key)) {
      return opposite_node->value;
    }
    throw std::out_of_range("bimap::at_right - no such element");
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename T = right_t, std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
  right_t const &at_left_or_default(left_t const &key) {
    find<left_tag, left_t>(tree_left, key);
    node<right_tag, right_t> *opposite_node =
        get_node_r(get_splay_l(tree_left));

    if (tree_left) {
      if (equal<left_tag>(tree_left->value, key)) {
        return opposite_node->value;
      } else {
        right_t value = right_t();
        find<right_tag, right_t>(tree_right, value);
        if (equal<right_tag>(tree_right->value, value)) {
          erase_right(right_iterator(tree_right, this));
        }
        insert(key, std::move(value));
      }
    } else {
      insert(key, right_t());
    }

    return get_node_r(get_splay_l(find<left_tag, left_t>(tree_left, key)))->value;
  }

  template <typename T = left_t , std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
  left_t const &at_right_or_default(right_t const &key) {
    find<right_tag, right_t>(tree_right, key);
    node<left_tag, left_t> *opposite_node = get_node_l(get_splay_r(tree_right));

    if (tree_right) {
      if (equal<right_tag>(tree_right->value, key)) {
        return opposite_node->value;
      } else {
        left_t value = left_t();
        find<left_tag, left_t>(tree_left, value);
        if (equal<left_tag>(tree_left->value, value)) {
          erase_left(left_iterator(tree_left, this));
        }
        insert(std::move(value), key);
      }
    } else {
      insert(left_t(), key);
    }

    return get_node_l(get_splay_r(find<right_tag, right_t>(tree_right, key)))->value;
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t &left) const {
    return bound_operation<left_tag, left_t>(left, true);
  }
  left_iterator upper_bound_left(const left_t &left) const {
    return bound_operation<left_tag, left_t>(left, false);
  }

  right_iterator lower_bound_right(const right_t &right) const {
    return bound_operation<right_tag, right_t>(right, true);
  }
  right_iterator upper_bound_right(const right_t &right) const {
    return bound_operation<right_tag, right_t>(right, false);
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    return left_iterator(find_min(tree_left), this);
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_iterator(nullptr, this);
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return right_iterator(find_min(tree_right), this);
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_iterator(nullptr, this);
  }

  // Проверка на пустоту
  bool empty() const {
    return tree_size == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return tree_size;
  }

  // операторы сравнения
  bool operator==(bimap const &b) const {
    if (tree_size != b.tree_size) {
      return false;
    }

    left_iterator l1 = begin_left();
    left_iterator l2 = b.begin_left();

    while (l1 != end_left()) {
      if (!equal<left_tag>(*l1, *l2) || !equal<right_tag>(*l1.flip(), *l2.flip())) {
        return false;
      }
      l1++;
      l2++;
    }
    return true;
  }
  bool operator!=(bimap const &b) const {
    return !(*this == b);
  }

private:
  void swap(bimap &second) {
    using std::swap;

    swap(tree_left, second.tree_left);
    swap(tree_right, second.tree_right);
    swap(tree_size, second.tree_size);
  }

  template <typename Tag, typename T>
  auto bound_operation(T const &value, bool lower_bound) const {
    node<Tag, T> *tree = find(get_root<Tag, T>(), value);
    if (!tree) {
      tree = next<Tag, T>(value);
    }

    if (tree && (lower_bound ? less<Tag>(tree->value, value) :
        (less<Tag>(tree->value, value) || equal<Tag>(tree->value, value)))) {
      return iterator<Tag, T>(next(tree), this);
    }

    return iterator<Tag, T>(tree, this);
  }

  template <typename Tag, typename T>
  iterator<Tag, T> insert_operation(splay_tree_t *new_node) {
    tree_size++;
    insert_both_trees(new_node);
    return iterator<Tag, T>(new_node, this);
  }

  bool contains(left_t const &left, right_t const &right) {
    node<left_tag, left_t> *left_find = find<left_tag, left_t>(tree_left, left);
    node<right_tag, right_t> *right_find = find<right_tag, right_t>(tree_right, right);

    return (left_find && equal<left_tag>(left_find->value, left)) ||
           (right_find && equal<right_tag>(right_find->value,right));
  }

  template <typename Tag, typename T>
  node<Tag, T>* &get_root() const {
    if constexpr (std::is_same_v<Tag, left_tag>) {
      return tree_left;
    } else {
      return tree_right;
    }
  }

  template <typename Tag, typename T>
  node<Tag, T> *set_tree_root(node<Tag, T>* t) const {
    if constexpr (std::is_same_v<Tag, left_tag>) {
      return tree_left = splay(t);
    } else {
      return tree_right = splay(t);;
    }
  }

  void insert_both_trees(splay_tree_t *node_new) {
    insert<left_tag>(get_node_l(node_new));
    insert<right_tag>(get_node_r(node_new));
  }

  template <typename Tag, typename T>
  static auto *get_opposite(node<Tag, T> *t) {
    if constexpr (std::is_same_v<Tag, left_tag>) {
      return get_node_r(get_splay_l(t));
    } else {
      return get_node_l(get_splay_r(t));
    }
  }
  /**
   * Bimap fields
   */
  mutable node<left_tag, left_t> *tree_left;
  mutable node<right_tag, right_t> *tree_right;

  CompareLeft compare_left;
  CompareRight compare_right;

  size_t tree_size;
};
