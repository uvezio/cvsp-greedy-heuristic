// All relative paths are relative to the "cvsp/" directory

#ifndef CVSP_CVSP_HPP
#define CVSP_CVSP_HPP

#include <chrono>
#include <functional>
#include <vector>

// This path is the only one relative to the "include/" directory
#include "graph.hpp"

namespace cvsp {

using Rule = std::function<std::size_t(
    std::size_t, std::vector<std::vector<std::size_t>> const&)>;

/*
 * This class implements the Capacitated Vertex Separator Problem (CVSP).

 * A CVSP instance is defined by two parameters, k and b, and an input graph.
 *
 * The class maintains a mutable state consisting of:
 * - the current separator (removed vertices)
 * - the current adjacency list (after removals)
 * - the current connected components (after removals)
 *
 * A feasible solution can be computed using the solve() method, which applies a
 * recursive heuristic algorithm.
 *
 * The implemented procedure does not guarantee optimality of the solution.
 */
class CVSP
{
 private:
  const std::size_t k_; // Parameter k of CVSP
  const std::size_t b_; // Parameter b of CVSP
  const Graph& graph_;  // Original graph

  std::vector<std::vector<std::size_t>> adj_; // Current adjacency list
  std::vector<std::size_t> separator_;        // Vertices currently removed
  std::vector<bool> in_separator_;            // True if vertex is in separator_

  std::vector<std::vector<std::size_t>> components_; // Connected components

  std::chrono::microseconds time_; // Execution time in µs of the algorithm

  // Terminates program execution if any vertex-related invariant is violated.
  void check_vertex_invariants() const;

  // Terminates program execution if the components invariant is violated.
  void check_component_invariants() const;

  // Initializes state variables in the constructor.
  void initialize_state_variables();

  // Adds the given vertex to separator_ and updates in_separator_, and adj_.
  // The vertex must be valid and not already removed.
  void remove_vertex(std::size_t vertex);

  // Updates components_ using the current adj_.
  // Vertices in separator_ are excluded.
  void update_connected_components();

  // Recursively removes vertices from components with size >= b_.
  // For each oversized component, selects a vertex (using the given rules),
  // removes it and computes the new subcomponents. The process is repeated for
  // each subcomponent.
  // Terminates when each component has size <= b_.
  void recursive_algorithm(std::vector<std::vector<std::size_t>>& components,
                           std::vector<Rule> const& rules = {});

  // Terminates program execution if b-constraint is violated.
  void check_b_constraint();

  // Terminates program execution if k-constraint is violated.
  void check_k_constraint();

  // Applies a heuristic bin packing algorithm to pack the connected components
  // into <= k_ bins of size <= b_.
  // This method does not guarantee a feasible solution for all instances, even
  // if it exisits.
  void pack_connected_components();

 public:
  // Initializes CVSP.
  CVSP(std::size_t k, std::size_t b, Graph const& graph);

  // Using the given rules, applies the solving algorithm to find a feasible
  // solution to the current CVSP instance.
  // This method does not guarantee a feasible solution for all instances, even
  // if it exists.
  void solve(std::vector<Rule> const& rules = {});

  // Getters

  std::size_t k() const;

  std::size_t b() const;

  const Graph& graph() const;

  const std::vector<std::size_t>& separator() const;

  const std::vector<std::vector<std::size_t>>& adjacency_list() const;

  const std::vector<std::vector<std::size_t>>& components() const;

  std::chrono::microseconds time() const;
};

// Selects a single vertex from subset using the sequence of rules in order.
// Each rule reduces the candidate set to the vertices maximizing that rule.
// If only one vertex remains it is returned; otherwise, a random one is chosen.
// By assumption, subset is non-empty and contains valid indices.
std::size_t select_vertex(std::vector<std::size_t> subset,
                          std::vector<std::vector<std::size_t>> const& adj,
                          std::vector<Rule> const& rules = {});

// Returns the vertices in the given subset with the maximum score according to
// the specified rule.
// By assumption, subset is non-empty and contains valid indices.
std::vector<std::size_t>
select_vertices(std::vector<std::size_t> const& subset,
                std::vector<std::vector<std::size_t>> const& adj,
                Rule const& rule);

// Updates the current selection of vertices given a new candidate.
// If current_score > max_score the selection is reset to {vertex};
// if equal, vertex is added to the current selection.
void update_selection(std::size_t vertex, std::size_t current_score,
                      std::size_t& max_score,
                      std::vector<std::size_t>& selection);

// Returns the degree of the given vertex. By assumption, vertex index is valid.
std::size_t degree(std::size_t vertex,
                   std::vector<std::vector<std::size_t>> const& adj);

// Returns the maximum degree among neighbors of the given vertex.
// By assumption, vertex index is valid.
std::size_t max_adj_degree(std::size_t vertex,
                           std::vector<std::vector<std::size_t>> const& adj);

// Returns the sum of the degrees of all neighbors of the given vertex.
// By assumption, vertex index is valid.
std::size_t sum_adj_degree(std::size_t vertex,
                           std::vector<std::vector<std::size_t>> const& adj);

// Returns a random vertex in the given vector.
// By assumption, vertices is non-empty.
std::size_t random_vertex(std::vector<std::size_t> const& vertices);

} // namespace cvsp

#endif