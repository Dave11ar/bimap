#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <functional>
#include <type_traits>
#include <stdexcept>

template <typename Left, typename Right,
    typename CompareLeft = std::less<Left>, typename CompareRight = std::less<Right>>
struct bimap {
  using left_t = Left;
  using right_t = Right;

private:
  struct left_tag;
  struct right_tag;

  template <typename Tag, typename Dummy = void>
  struct node {
    explicit node(left_t &&value) : value(std::move(value)) {}
    explicit node(left_t const &value) : value(value) {}

    ~node() = default;

    left_t value;
    node *parent = nullptr;
    node *left = nullptr;
    node *right = nullptr;
  };

  template <typename Dummy>
  struct node<right_tag, Dummy> {
    explicit node(right_t &&value) : value(std::move(value)) {}
    explicit node(right_t const &value) : value(value) {}

    ~node() = default;

    right_t value;
    node *parent = nullptr;
    node *left = nullptr;
    node *right = nullptr;
  };

  struct splay_tree : node<left_tag>, node<right_tag> {
    splay_tree() = default;

    splay_tree(splay_tree const &other)
        : node<left_tag>(get_node<left_tag>(&other)->value),
          node<right_tag>(get_node<right_tag>(&other)->value) {}

    splay_tree(left_t &&first_value, right_t &&second_value)
        : node<left_tag>(std::move(first_value)), node<right_tag>(std::move(second_value)) {}
    splay_tree(left_t const &first_value, right_t &&second_value)
        : node<left_tag>(first_value), node<right_tag>(std::move(second_value)) {}
    splay_tree(left_t &&first_value, right_t const &second_value)
        : node<left_tag>(std::move(first_value)), node<right_tag>(second_value) {}
    splay_tree(left_t const &first_value, right_t const &second_value)
        : node<left_tag>(first_value), node<right_tag>(second_value) {}

    ~splay_tree() = default;
  };

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
    return !less<Tag, T>(a, b) && !less<Tag, T>(b, a);
  }

  /**
   * invariant for all functions, tree_left and tree_right stay correct
   */

  /**
   * @return node with equal value if tree with root t, contains it, nullptr otherwise
   */
  template <typename Tag, typename T>
  node<Tag> *find(node<Tag> *t, T const &value) const {
    if (!t) {
      return t;
    }

    if (equal<Tag, T>(t->value, value)) {
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
  template <typename Tag>
  bool insert(node<Tag> *new_node) const {
    node<Tag> *t = get_root<Tag>();

    if (!t) {
      set_tree_root(new_node);
      return true;
    }

    std::pair<node<Tag>*, node<Tag>*> tmp = split<Tag>(new_node->value);

    if (tmp.first && tmp.first->value == new_node->value) {
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
    node<Tag> *t = get_root<Tag>();

    if (!t) {
      return false;
    }

    t = find(t, value);
    if (t->value != value) {
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
   * @return next element or nullptr if element is last in tree
   */
  template <typename Tag>
  node<Tag>* next(node<Tag> *t) const {
    if (!t) {
      return t;
    }

    t = set_tree_root(t);

    if (!t->right) {
      return nullptr;
    }

    node<Tag> *tmp = t->right;
    while (tmp->left) {
      tmp = tmp->left;
    }

    return set_tree_root(tmp);
  }


  template <typename Tag, typename T>
  node<Tag> *next(T const &value) const {
    node<Tag> *t = get_root<Tag>();

    if (!t) {
      return t;
    }

    while (true) {
      if (less<Tag, T>(t->value, value) || equal<Tag, T>(t->value, value)) {
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

//  /**
//   * @return pair with root of new tree and prev element
//   */
//  template <typename Tag>
//  std::pair<node<Tag>*, node<Tag>*> prev(node<Tag> *t) const {
//    t = splay(t);
//
//    if (!t->left) {
//      return {t, nullptr};
//    }
//
//    node<Tag> *tmp = t->left;
//    while (tmp->right) {
//      tmp = tmp->right;
//    }
//
//    tmp = splay(tmp);
//    return {tmp, tmp};
//  }

  template <typename Tag>
  void destroy(node<Tag> *t) {
    if (!t) {
      return;
    }

    destroy(t->left);
    destroy(t->right);
    delete get_splay(t);
  }

  template <typename Tag>
  node<Tag> *zig(node<Tag> *t) const{
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

    node<Tag> *tmp = t->parent->parent;
    t->parent->parent = t;
    t->parent = tmp;

    return t;
  }

  template <typename Tag>
  node<Tag> *splay(node<Tag> *t) const {
    if (!t || !t->parent) {
      return get_root<Tag>() = t;
    }
    if (!t->parent->parent) {
      return get_root<Tag>() = zig(t);
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
  std::pair<node<Tag>*, node<Tag>*> split(T const &value) const {
    node<Tag> *t = next<Tag>(value);
    if (!t) {
      return {get_root<Tag>(), t};
    }

    node<Tag> *left_new = t->left;
    t->left = nullptr;

    if (left_new) {
      left_new->parent = nullptr;
    }
    return {left_new, t};
  }

  template <typename Tag>
  node<Tag> *find_max(node<Tag> *t) const {
    while (t->right) {
      t = t->right;
    }

    return set_tree_root(t);
  }

  template <typename Tag>
  node<Tag> *find_min(node<Tag> *t) const {
    while (t->left) {
      t = t->left;
    }

    return set_tree_root(t);
  }

  template <typename Tag>
  void merge(node<Tag> *a, node<Tag> *b) const {
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
      if (!tree->right) {
        if (!tree->parent || tree->parent->right == tree) {
          tree = nullptr;
        } else {
          tree = tree->parent;
        }
      } else {
        tree = tree->right;
        while (tree->left) {
          tree = tree->left;
        }
      }

      return *this;
    }
    iterator operator++(int) {
      node<Tag> *old = tree;
      ++*this;

      return {old};
    }

    // Переход к предыдущему по величине left'у.
    // Декремент итератора begin_left() неопределен.
    // Декремент невалидного итератора неопределен.
    iterator &operator--() {
      if (!tree->left) {
        if (!tree->parent || tree->parent->left == tree) {
          tree = nullptr;
        } else {
          tree = tree->parent;
        }
      } else {
        tree = tree->left;
        while (tree->right) {
          tree = tree->right;
        }
      }

      return *this;
    }
    iterator operator--(int) {
      node<Tag> *old = tree;
      --*this;

      return {old};
    }

    // left_iterator ссылается на левый элемент некоторой пары.
    // Эта функция возвращает итератор на правый элемент той же пары.
    // end_left().flip() возращает end_right().
    // end_right().flip() возвращает end_left().
    // flip() невалидного итератора неопределен.

    auto flip() const {
      if constexpr (std::is_same_v<Tag, left_tag>) {
        return iterator<right_tag, right_t>(get_opposite(tree));
      } else {
        return iterator<left_tag, left_t>(get_opposite(tree));
      }
    }

    bool operator==(iterator<Tag, T> const& other) const {
      return tree == other.tree;
    }
    bool operator!=(iterator<Tag, T> const& other) const {
      return tree != other.tree;
    }

    iterator(node<Tag> *tree) : tree(tree) {};
    iterator(iterator const &other) : tree(other.tree) {};

    /**
     * if tree == nullptr, then iterator = end()
     */
    node<Tag> *tree;
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
        compare_left(compare_left), compare_right(compare_right), siz(0) {}

  // Конструкторы от других и присваивания
  bimap(bimap const &other) : tree_left(nullptr), tree_right(nullptr) {
    copy(other);
  }
  bimap(bimap &&other) noexcept
      : tree_left(other.tree_left), tree_right(other.tree_right),
        compare_left(std::move(other.compare_left)),
        compare_right(std::move(other.compare_right)), siz(other.siz) {}

  bimap &operator=(bimap const &other) {
    clear();
    copy(other);

    return *this;
  }
  bimap &operator=(bimap &&other) noexcept {
    clear();

    tree_left = other.tree_left;
    tree_right = other.tree_right;
    compare_left = std::move(other.compare_left);
    compare_right = std::move(other.compare_right);
    siz = other.siz;
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
    if (contains(left, right)) {
      return end_left();
    }

    siz++;
    insert_both_trees(new splay_tree(left, right));
    return left_iterator(find<left_tag, left_t>(tree_left, left));
  }
  left_iterator insert(left_t const &left, right_t &&right) {
    if (contains(left, right)) {
      return end_left();
    }

    siz++;
    insert_both_trees(new splay_tree(left, std::move(right)));
    return left_iterator(find<left_tag, left_t>(tree_left, left));
  }
  left_iterator insert(left_t &&left, right_t const &right) {
    if (contains(left, right)) {
      return end_left();
    }

    siz++;
    insert_both_trees(new splay_tree(std::move(left), right));
    return left_iterator(find<left_tag, left_t>(tree_left, left));
  }
  left_iterator insert(left_t &&left, right_t &&right) {
    if (contains(left, right)) {
      return end_left();
    }

    siz++;
    insert_both_trees(new splay_tree(std::move(left), std::move(right)));
    return left_iterator(find<left_tag, left_t>(tree_left, left));
  }

  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    splay(it.tree);

    splay_tree *tmp = get_splay(it.tree);
    node<left_tag> *nxt = next(tree_left);

    remove<left_tag>(*it);
    remove<right_tag>(get_node<right_tag>(tmp)->value);

    siz--;
    delete tmp;

    return left_iterator(nxt);
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const &left) {
    node<left_tag> *t = find(tree_left, left);
    if (!t || t->value != left) {
      return false;
    }

    erase_left(left_iterator(t));
    return true;
  }

  right_iterator erase_right(right_iterator it) {
    splay(it.tree);

    splay_tree *tmp = get_splay(it.tree);
    node<right_tag> *nxt = next(tree_right);

    remove<left_tag>(get_node<left_tag>(tmp)->value);
    remove<right_tag>(*it);

    siz--;
    delete tmp;

    return right_iterator(nxt);
  }
  bool erase_right(right_t const &right) {
    node<right_tag> *t = find(tree_right, right);
    if (!t || t->value != right) {
      return false;
    }

    erase_right(right_iterator(t));
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
    node<left_tag> *t = find(tree_left, left);
    if (t && t->value == left) {
      return left_iterator(t);
    } else {
      return end_left();
    }
  }
  right_iterator find_right(right_t const &right) const {
    node<right_tag> *t = find(tree_right, right);
    if (t && t->value == right) {
      return right_iterator(t);
    } else {
      return end_right();
    }
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const &at_left(left_t const &key) const {
    find<left_tag, left_t>(tree_left, key);

    node<right_tag> *opposite_node = get_node<right_tag>(get_splay(tree_left));
    if (tree_left && tree_left->value == key) {
      return opposite_node->value;
    }
    throw std::out_of_range("bimap::invalid iterator");
  }

  left_t const &at_right(right_t const &key) const {
    find<right_tag, right_t>(tree_right, key);

    node<left_tag> *opposite_node = get_node<left_tag>(get_splay(tree_right));
    if (tree_right && tree_right->value == key) {
      return opposite_node->value;
    }
    throw std::out_of_range("bimap::invalid iterator");
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  right_t const &at_left_or_default(left_t const &key) {
    find<left_tag, left_t>(tree_left, key);
    node<right_tag> *opposite_node = get_node<right_tag>(get_splay(tree_left));

    if (tree_left) {
      if (tree_left->value == key) {
        return opposite_node->value;
      } else {
        find<right_tag, right_t>(tree_right, right_t());
        if (tree_right->value == right_t()) {
          erase_right(right_t());
        }
        insert(key, right_t());
      }
    } else {
      insert(key, right_t());
    }

    return get_node<right_tag>(get_splay(find<left_tag, left_t>(tree_left, key)))->value;
  }
  left_t const &at_right_or_default(right_t const &key) {
    find<right_tag, right_t>(tree_right, key);
    node<left_tag> *opposite_node = get_node<left_tag>(get_splay(tree_right));

    if (tree_right) {
      if (tree_right->value == key) {
        return opposite_node->value;
      } else {
        find<left_tag, left_t>(tree_left, left_t());
        if (tree_left->value == left_t()) {
          erase_left(left_t());
        }
        insert(left_t(), key);
      }
    } else {
      insert(left_t(), key);
    }

    return get_node<left_tag>(get_splay(find<right_tag, right_t>(tree_right, key)))->value;
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
    return left_iterator(find_min(tree_left));
  }
  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_iterator(nullptr);
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return right_iterator(find_min(tree_right));
  }
  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_iterator(nullptr);
  }

  // Проверка на пустоту
  bool empty() const {
    return siz == 0;
  }

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return siz;
  }

  // операторы сравнения
  bool operator==(bimap const &b) const {
    if (siz != b.siz) {
      return false;
    }

    left_iterator l1 = begin_left();
    left_iterator l2 = b.begin_left();

    while (l1 != end_left()) {
      if (*l1 != *l2 || at_left(*l1) != b.at_left(*l2)) {
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
  template <typename Tag, typename T>
  auto bound_operation(T const &value, bool lower_bound) const {
    node<Tag> *tree = find(get_root<Tag>(), value);
    if (!tree) {
      tree = next<Tag, T>(value);
    }

    if (tree && (lower_bound ? tree->value < value : tree->value <= value)) {
      return iterator<Tag, T>(next(tree));
    }
    return iterator<Tag, T>(tree);
  }

  bool contains(left_t const &left, right_t const &right) {
    node<left_tag> *left_find = find<left_tag, left_t>(tree_left, left);
    node<right_tag> *right_find = find<right_tag, right_t>(tree_right, right);

    return (left_find && equal<left_tag>(left_find->value, left)) ||
           (right_find && equal<right_tag>(right_find->value,right));
  }

  void copy(bimap const &other) {
    compare_left = other.compare_left;
    compare_right = other.compare_right;
    siz = other.siz;

    for (left_iterator it = other.begin_left(); it != other.end_left(); it++) {
      auto *tmp = new splay_tree(*it, other.at_left(*it));
      insert<left_tag>(get_node<left_tag>(tmp));
      insert<right_tag>(get_node<right_tag>(tmp));
    }
  }

  static node<left_tag> *get_opposite(node<right_tag> *t) {
    return get_node<left_tag>(get_splay(t));
  }

  static node<right_tag> *get_opposite(node<left_tag> *t) {
    return get_node<right_tag>(get_splay(t));
  }

  template <typename Tag>
  static splay_tree *get_splay(node<Tag> *t) {
    return static_cast<splay_tree*>(t);
  }

  template <typename Tag>
  static node<Tag> *get_node(splay_tree *t) {
    return static_cast<node<Tag>*>(t);
  }

  template <typename Tag>
  node<Tag>* &get_root() const {
    if constexpr (std::is_same_v<Tag, left_tag>) {
      return tree_left;
    } else {
      return tree_right;
    }
  }

  template <typename Tag>
  node<Tag> *set_tree_root(node<Tag>* t) const {
    if constexpr (std::is_same_v<Tag, left_tag>) {
      return tree_left = splay(t);
    } else {
      return tree_right = splay(t);;
    }
  }

  void insert_both_trees(splay_tree *node_new) {
    insert<left_tag>(get_node<left_tag>(node_new));
    insert<right_tag>(get_node<right_tag>(node_new));
  }

  void clear() {
    erase_left(begin_left(), end_left());
    tree_left = nullptr;
    tree_right = nullptr;
  }

  /**
   * Bimap fields
   */
  mutable node<left_tag> *tree_left;
  mutable node<right_tag> *tree_right;

  CompareLeft compare_left;
  CompareRight compare_right;

  mutable size_t siz;
};
