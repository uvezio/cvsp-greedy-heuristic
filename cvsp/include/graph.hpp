// All relative paths are relative to the "cvsp/" directory

#ifndef CVSP_GRAPH_HPP
#define CVSP_GRAPH_HPP

#include <filesystem>
#include <fstream>
#include <vector>

namespace cvsp {

/*
 * This class represents a simple undirected graph loaded from a file.

 * The graph is stored as an adjacency list.

 * Vertices are labeled with natural numbers, either starting from 0 or from 1.
 * To simplify indexing, vectors of size vertices_ + 1 are adopted.
 * - In the 1-based case, index 0 is unused and can be treated as an isolated
 * vertex.
 * - In the 0-based case, index vertices_ is unused and can be treated as an
 * isolated vertex.
 */
class Graph
{
 private:
  std::size_t vertices_;                      // Number of vertices
  std::size_t edges_;                         // Number of edges
  std::vector<std::vector<std::size_t>> adj_; // Adjacency list

  std::size_t base_index_; // 1 if the graph is 1-based, 0 if 0-based

  // Adds an undirected edge between vertices u and v.
  // Requires base_index_ <= u, v <= max_index() and u != v.
  // Edge u-v must not already exists.
  void add_edge(std::size_t u, std::size_t v);

  // Reads and constructs the graph representation from a DIMACS input stream.
  // Assumes the input file is well-formed and conforms to the DIMACS format.
  void read_from_dimacs(std::ifstream& instream);

  // Terminates program execution if the class invariant is violated.
  void check_invariants() const;

 public:
  // Creates and loads the graph from a file whose extension is supported.
  // The only supported extension is ".dimacs".
  Graph(std::filesystem::path const& path);

  // Loads the graph from a file whose extension is supported.
  // The only supported extension is ".dimacs".
  // Clears current graph contents before loading.
  void load_from_file(std::filesystem::path const& path);

  // Returns the connected components of the graph, each as a vector of vertex
  // indices.
  std::vector<std::vector<std::size_t>> get_connected_components() const;

  // Getters

  std::size_t vertices() const;

  std::size_t edges() const;

  const std::vector<std::vector<std::size_t>>& adjacency_list() const;

  // Returns 1 if 1-based, 0 if 0-based
  std::size_t base_index() const;

  // Returns vertices_ if 1-based, vertices_ - 1 if 0-based.
  std::size_t max_index() const;

  // Returns 0 if 1-based, vertices_ if 0-based.
  std::size_t redundant_index() const;
};

// Returns true if min <= vertex <= max, false otherwise.
bool is_valid_index(std::size_t vertex, std::size_t min, std::size_t max);

// Performs a Depth-First Search (DFS) starting from the given vertex.
// adj is the adjacency list of the graph.
// Marks all reachable vertices in visited and collects them into component.
// visited must have size equal to adj.size() and is updated in place.
void DFS(std::vector<std::vector<std::size_t>> const& adj, std::size_t vertex,
         std::vector<std::size_t>& component, std::vector<bool>& visited);

// Returns the connected components in the given subset of vertices, each as a
// vector of vertex indices.
// By assumption, subset contains valid indices and vertices in subset do not
// have any edges connecting to vertices outside subset.
std::vector<std::vector<std::size_t>>
connected_components(std::vector<std::vector<std::size_t>> const& adj,
                     std::vector<std::size_t> const& subset);

} // namespace cvsp

#endif