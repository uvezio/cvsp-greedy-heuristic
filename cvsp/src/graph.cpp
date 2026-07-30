// All relative paths are relative to the "cvsp/" directory

#include <algorithm>
#include <cassert>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>

// This path is the only one relative to the "src/" directory
#include "../include/graph.hpp"

namespace cvsp {

void Graph::add_edge(std::size_t u, std::size_t v)
{
  assert(adj_.size() == vertices_ + 1);

  assert(u != v);
  assert(is_valid_index(u, base_index_, max_index()));
  assert(is_valid_index(v, base_index_, max_index()));

  adj_[u].push_back(v);
  adj_[v].push_back(u);

  assert(std::count(adj_[u].begin(), adj_[u].end(), v) == 1);
  assert(std::count(adj_[v].begin(), adj_[v].end(), u) == 1);

  assert(adj_.size() == vertices_ + 1);
}

void Graph::read_from_dimacs(std::ifstream& instream)
{
  base_index_ = 1;

  std::string line;
  while (std::getline(instream, line)) {
    if (line.empty() || line[0] == 'c') {
      continue;

    } else if (line[0] == 'p') {
      std::string type;
      std::istringstream instring(line);
      instring >> type >> type >> vertices_ >> edges_;
      adj_.assign(vertices_ + 1, {});

    } else if (line[0] == 'e') {
      char e;
      std::size_t u, v;
      std::istringstream instring(line);
      instring >> e >> u >> v;
      add_edge(u, v);
    }
  }
}

void Graph::check_invariants() const
{
  // Check that the adjacency list container has the correct number of entries
  assert(adj_.size() == vertices_ + 1);

  // Check that the number of edges in the adjacency lists matches edges_
  assert(2 * edges_
         == static_cast<std::size_t>(std::accumulate(
             adj_.begin(), adj_.end(), std::size_t{0},
             [](auto sum, auto const& adj_v) { return sum + adj_v.size(); })));

  // Ensure that all adjacency list entries refer to valid vertex indices
  assert(std::all_of(adj_.begin(), adj_.end(), [&](auto const& adj_v) {
    return std::all_of(adj_v.begin(), adj_v.end(), [&](auto u) {
      return is_valid_index(u, base_index_, max_index());
    });
  }));

  // Check that base_index_ is valid
  assert(base_index_ == 1 || base_index_ == 0);

  // Check that the redundant entry is consistent with base_index_
  assert(base_index_ == 1 ? adj_[0].empty() : adj_[vertices_].empty());
}

Graph::Graph(std::filesystem::path const& path)
    : vertices_{0}
    , edges_{0}
    , adj_{{}}
    , base_index_{1}
{
  load_from_file(path);
}

void Graph::load_from_file(std::filesystem::path const& path)
{
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error(("File \"" + path.string() + "\" not found."));
  }
  if (!std::filesystem::is_regular_file(path)) {
    throw std::runtime_error("File \"" + path.string()
                             + "\" is not a regular file.");
  }
  if (path.extension() != ".dimacs") {
    throw std::runtime_error("File \"" + path.string()
                             + "\" has an invalid extension.");
  }
  if (std::filesystem::is_empty(path)) {
    throw std::runtime_error("File \"" + path.string() + "\" is empty.");
  }

  vertices_ = 0;
  edges_    = 0;
  adj_.assign(1, {});
  base_index_ = 1;

  std::ifstream graph_infile(path);

  if (!graph_infile) {
    throw std::runtime_error("File \"" + path.string()
                             + "\" not opened successfully.");
  }

  if (path.extension() == ".dimacs") {
    read_from_dimacs(graph_infile);
  }

  graph_infile.close();

  check_invariants();
}

std::vector<std::vector<std::size_t>> Graph::get_connected_components() const
{
  check_invariants();

  std::vector<std::size_t> graph(vertices_);
  std::iota(graph.begin(), graph.end(), base_index_);

  std::vector<std::vector<std::size_t>> components;
  components = connected_components(adj_, graph);

  return components;
}

std::size_t Graph::vertices() const
{
  return vertices_;
}

std::size_t Graph::edges() const
{
  return edges_;
}

const std::vector<std::vector<std::size_t>>& Graph::adjacency_list() const
{
  return adj_;
}

std::size_t Graph::base_index() const
{
  return base_index_;
}

std::size_t Graph::max_index() const
{
  auto max_index = vertices_;
  if (base_index_ == 0) {
    --max_index;
  }
  return max_index;
}

std::size_t Graph::redundant_index() const
{
  return base_index_ == 1 ? 0 : vertices_;
}

bool is_valid_index(std::size_t vertex, std::size_t min, std::size_t max)
{
  return vertex >= min && vertex <= max;
}

void DFS(std::vector<std::vector<std::size_t>> const& adj, std::size_t vertex,
         std::vector<std::size_t>& component, std::vector<bool>& visited)
{
  visited[vertex] = true;
  component.push_back(vertex);

  for (auto neighbor : adj[vertex]) {
    if (!visited[neighbor]) {
      DFS(adj, neighbor, component, visited);
    }
  }
}

std::vector<std::vector<std::size_t>>
connected_components(std::vector<std::vector<std::size_t>> const& adj,
                     std::vector<std::size_t> const& subset)
{
  std::vector<std::vector<std::size_t>> connected_components;
  std::vector<bool> visited(adj.size(), false);

  for (auto v : subset) {
    if (!visited[v]) {
      std::vector<std::size_t> component;
      DFS(adj, v, component, visited);
      connected_components.push_back(component);
    }
  }

  // Every vertex must belong to exactly one component
  assert(subset.size()
         == static_cast<std::size_t>(std::accumulate(
             connected_components.begin(), connected_components.end(),
             std::size_t{0}, [](auto sum, auto const& component) {
               return sum + component.size();
             })));

  return connected_components;
}

} // namespace cvsp