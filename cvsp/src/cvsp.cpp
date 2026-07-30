// All relative paths are relative to the "cvsp/" directory

#include <algorithm>
#include <cassert>
#include <numeric>
#include <random>
#include <utility>
#include <stdexcept>

// This path is the only one relative to the "src/" directory
#include "../include/cvsp.hpp"

namespace cvsp {

void CVSP::check_vertex_invariants() const
{
  assert(adj_.size() == graph_.vertices() + 1);
  assert(adj_[graph_.redundant_index()].empty());

  assert(in_separator_.size() == graph_.vertices() + 1);
  assert(in_separator_[graph_.redundant_index()] == false);

  assert(separator_.size()
         == static_cast<std::size_t>(
             std::count(in_separator_.begin(), in_separator_.end(), true)));

  assert(std::all_of(separator_.begin(), separator_.end(), [&](std::size_t v) {
    return in_separator_[v] && adj_[v].empty();
  }));
}

void CVSP::check_component_invariants() const
{
  assert(graph_.vertices() - separator_.size()
         == static_cast<std::size_t>(std::accumulate(
             components_.begin(), components_.end(), std::size_t{0},
             [](auto sum, auto const& component) {
               return sum + component.size();
             })));

  assert(std::all_of(
      components_.begin(), components_.end(),
      [&](std::vector<std::size_t> const& component) {
        return std::all_of(
            component.begin(), component.end(), [&](std::size_t u) {
              return is_valid_index(u, graph_.base_index(), graph_.max_index())
                  && !in_separator_[u];
            });
      }));
}

void CVSP::initialize_state_variables()
{
  adj_ = graph_.adjacency_list();

  separator_.clear();
  in_separator_.assign(adj_.size(), false);

  check_vertex_invariants();

  components_ = graph_.get_connected_components();

  check_component_invariants();

  time_ = std::chrono::microseconds::zero();
}

void CVSP::remove_vertex(std::size_t vertex)
{
  check_vertex_invariants();

  assert(is_valid_index(vertex, graph_.base_index(), graph_.max_index()));
  assert(!in_separator_[vertex]);

  separator_.push_back(vertex);
  in_separator_[vertex] = true;

  for (auto u : adj_[vertex]) {
    auto it = std::find(adj_[u].begin(), adj_[u].end(), vertex);
    assert(it != adj_[u].end());
    adj_[u].erase(it);
  }

  adj_[vertex].clear();

  check_vertex_invariants();
}

void CVSP::update_connected_components()
{
  check_vertex_invariants();

  std::vector<std::size_t> graph(graph_.vertices());
  std::iota(graph.begin(), graph.end(), graph_.base_index());

  for (auto v : separator_) {
    graph.erase(std::find(graph.begin(), graph.end(), v));
  }

  components_ = connected_components(adj_, graph);

  check_vertex_invariants();
  check_component_invariants();
}

void CVSP::recursive_algorithm(
    std::vector<std::vector<std::size_t>>& components,
    std::vector<Rule> const& rules)
{
  for (auto& component : components) {
    if (component.size() > b_) {
      auto to_remove = select_vertex(component, adj_, rules);
      remove_vertex(to_remove);
      component.erase(std::find(component.begin(), component.end(), to_remove));
      auto subcomponents = connected_components(adj_, component);
      recursive_algorithm(subcomponents, rules);
    }
  }
}

void CVSP::check_b_constraint()
{
  assert(std::all_of(
      components_.begin(), components_.end(),
      [&](auto const& component) { return component.size() <= b_; }));
}

void CVSP::check_k_constraint()
{
  assert(components_.size() <= k_);
}

void CVSP::pack_connected_components()
{
  check_vertex_invariants();
  check_component_invariants();

  check_b_constraint();

  std::sort(
      components_.begin(), components_.end(),
      [](auto const& c1, auto const& c2) { return c1.size() > c2.size(); });

  std::vector<std::vector<std::size_t>> packing;

  for (auto const& component : components_) {
    auto min_residual = static_cast<int>(b_);
    std::size_t best_index{0};

    for (std::size_t i{0}; i != packing.size(); ++i) {
      auto check_residual = static_cast<int>(b_);
      check_residual -= static_cast<int>(packing[i].size() + component.size());
      if (check_residual >= 0 && check_residual < min_residual) {
        min_residual = check_residual;
        best_index   = i;
      }
    }

    auto check_residual = static_cast<int>(b_);
    check_residual -= static_cast<int>(component.size());
    if (check_residual >= 0 && check_residual < min_residual) {
      min_residual = check_residual;
      best_index   = packing.size();
      packing.resize(best_index + 1);
    }

    packing[best_index].insert(packing[best_index].end(), component.begin(),
                               component.end());
  }

  components_ = packing;

  check_vertex_invariants();
  check_component_invariants();

  check_k_constraint();
  check_b_constraint();
}

CVSP::CVSP(std::size_t k, std::size_t b, Graph const& graph)
    : k_{k}
    , b_{b}
    , graph_{graph}
{
  initialize_state_variables();
}

void CVSP::solve(std::vector<Rule> const& rules)
{
  initialize_state_variables();

  auto start = std::chrono::steady_clock::now();

  recursive_algorithm(components_, rules);

  update_connected_components();

  check_b_constraint();

  if (components_.size() > k_) {
    pack_connected_components();
    if (components_.size() > k_) {
      throw std::runtime_error("gg");
    }
  }

  auto end = std::chrono::steady_clock::now();

  time_ = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
}

std::size_t CVSP::k() const
{
  return k_;
}

std::size_t CVSP::b() const
{
  return b_;
}

const Graph& CVSP::graph() const
{
  return graph_;
}

const std::vector<std::size_t>& CVSP::separator() const
{
  return separator_;
}

const std::vector<std::vector<std::size_t>>& CVSP::adjacency_list() const
{
  return adj_;
}

const std::vector<std::vector<std::size_t>>& CVSP::components() const
{
  return components_;
}

std::chrono::microseconds CVSP::time() const
{
  return time_;
}

std::size_t select_vertex(std::vector<std::size_t> subset,
                          std::vector<std::vector<std::size_t>> const& adj,
                          std::vector<Rule> const& rules)
{
  for (auto const& rule : rules) {
    auto selection = select_vertices(subset, adj, rule);
    if (selection.size() <= 1) {
      return selection[0];
    }
    subset = std::move(selection);
  }

  return random_vertex(subset);
}

std::vector<std::size_t>
select_vertices(std::vector<std::size_t> const& subset,
                std::vector<std::vector<std::size_t>> const& adj,
                Rule const& rule)
{
  std::vector<std::size_t> selected;

  std::size_t max{0};
  for (auto vertex : subset) {
    update_selection(vertex, rule(vertex, adj), max, selected);
  }

  return selected;
}

void update_selection(std::size_t vertex, std::size_t current_score,
                      std::size_t& max_score,
                      std::vector<std::size_t>& selection)
{
  if (current_score > max_score) {
    max_score = current_score;
    selection.clear();
    selection.push_back(vertex);
  } else if (current_score == max_score) {
    selection.push_back(vertex);
  }
}

std::size_t degree(std::size_t vertex,
                   std::vector<std::vector<std::size_t>> const& adj)
{
  return adj[vertex].size();
}

std::size_t max_adj_degree(std::size_t vertex,
                           std::vector<std::vector<std::size_t>> const& adj)
{
  std::size_t max_deg{0};
  for (auto v : adj[vertex]) {
    auto deg = adj[v].size();
    if (deg > max_deg) {
      max_deg = deg;
    }
  }
  return max_deg;
}

std::size_t sum_adj_degree(std::size_t vertex,
                           std::vector<std::vector<std::size_t>> const& adj)
{
  return std::accumulate(adj[vertex].begin(), adj[vertex].end(), std::size_t{0},
                         [&](auto sum, auto u) { return sum + adj[u].size(); });
}

std::size_t random_vertex(std::vector<std::size_t> const& vertices)
{
  static thread_local std::mt19937 eng{std::random_device{}()};
  std::uniform_int_distribution<std::size_t> dist{0, vertices.size() - 1};

  return vertices[dist(eng)];
}

} // namespace cvsp