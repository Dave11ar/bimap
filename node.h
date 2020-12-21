#pragma once

#include <utility>

struct left_tag;
struct right_tag;

template <typename Tag, typename T>
struct node {
  explicit node(T &&value) : value(std::move(value)) {}
  explicit node(T const &value) : value(value) {}

  ~node() = default;

  T value;
  node *parent = nullptr;
  node *left = nullptr;
  node *right = nullptr;
};
