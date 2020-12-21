#pragma once

#include "node.h"
template <typename Left, typename Right>
struct splay_tree;

template <typename Tag, typename Left, typename Right, typename T>
static splay_tree<Left, Right> *get_splay(node<Tag, T> *t) {
  return static_cast<splay_tree<Left, Right>*>(t);
}

template <typename Tag, typename Left, typename Right, typename T>
static node<Tag, T> *get_node(splay_tree<Left, Right> *t) {
  return static_cast<node<Tag, T>*>(t);
}

template <typename Left, typename Right>
struct splay_tree : node<left_tag, Left>, node<right_tag, Right> {
  using left_t = Left;
  using right_t = Right;

  splay_tree() = default;

  splay_tree(splay_tree const &other)
      : node<left_tag, left_t>(get_node<left_tag, right_t>(&other)->value),
        node<right_tag, right_t>(get_node<right_tag, left_t>(&other)->value) {}

  splay_tree(left_t &&first_value, right_t &&second_value)
      : node<left_tag, left_t>(std::move(first_value)), node<right_tag, right_t>(std::move(second_value)) {}
  splay_tree(left_t const &first_value, right_t &&second_value)
      : node<left_tag, left_t>(first_value), node<right_tag, right_t>(std::move(second_value)) {}
  splay_tree(left_t &&first_value, right_t const &second_value)
      : node<left_tag, left_t>(std::move(first_value)), node<right_tag, right_t>(second_value) {}
  splay_tree(left_t const &first_value, right_t const &second_value)
      : node<left_tag, left_t>(first_value), node<right_tag, right_t>(second_value) {}

  ~splay_tree() = default;
};
