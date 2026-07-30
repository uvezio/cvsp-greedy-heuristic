// All relative paths are relative to the "cvsp/" directory

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// This path is the only one relative to "main/"
#include "../include/cvsp.hpp"

static const std::filesystem::path instances_dir{"instances"};
static const std::filesystem::path dimacs{".col.dimacs"};

static const std::filesystem::path results_dir{"results"};
static const std::filesystem::path in_results{"RisultatiDimacsCVSP.csv"};
static const std::filesystem::path out_results{"results.csv"};
static const auto in_path  = results_dir / in_results;
static const auto out_path = results_dir / out_results;

static const std::vector<std::string> configurations{
    "R", "MD>R", "MS>R", "MD>MS>R", "MS>MD>R", "MD>MA>R"};

static const std::vector<std::string> result_type{"sol", "time (µs)", "1s"};

static const std::chrono::microseconds time_limit{1'000'000};

enum class Row : std::size_t
{
  name,
  vertices,
  edges,
  k,
  category,
  b,
  primal,
  dual,
  gap,
  time,
  status
};

static void validate_directory(std::filesystem::path const& dir)
{
  if (!std::filesystem::exists(dir)) {
    throw std::runtime_error("Directory \"" + dir.string() + "\" not found.");
  }
  if (!std::filesystem::is_directory(dir)) {
    throw std::runtime_error("Path \"" + dir.string()
                             + "\" is not a directory.");
  }
  if (std::filesystem::is_empty(dir)) {
    throw std::runtime_error("Directory \"" + dir.string() + "\" is empty.");
  }
}

static void update_row(cvsp::CVSP& cvsp, std::vector<std::string>& row,
                       std::vector<std::vector<cvsp::Rule>> const& configs)
{
  for (auto const& config : configs) {
    cvsp.solve(config);
    row.push_back(std::to_string(cvsp.separator().size()));
    row.push_back(std::to_string(cvsp.time().count()));

    std::size_t best{cvsp.graph().vertices()};
    std::chrono::microseconds time{0};
    while (time < time_limit) {
      cvsp.solve(config);
      auto dt = cvsp.time();
      time += dt.count() != 0 ? dt : std::chrono::microseconds(1);
      if (cvsp.separator().size() < best) {
        best = cvsp.separator().size();
      }
    }
    row.push_back(std::to_string(best));
  }
}

int main()
{
  const std::vector<cvsp::Rule> config_1{};
  const std::vector<cvsp::Rule> config_2{cvsp::degree};
  const std::vector<cvsp::Rule> config_3{cvsp::sum_adj_degree};
  const std::vector<cvsp::Rule> config_4{cvsp::degree, cvsp::sum_adj_degree};
  const std::vector<cvsp::Rule> config_5{cvsp::sum_adj_degree, cvsp::degree};
  const std::vector<cvsp::Rule> config_6{cvsp::degree, cvsp::max_adj_degree};

  const std::vector<std::vector<cvsp::Rule>> configs{
      config_1, config_2, config_3, config_4, config_5, config_6};

  try {
    validate_directory(instances_dir);
    validate_directory(results_dir);

    if (!std::filesystem::exists(in_path)) {
      throw std::runtime_error("File \"" + in_path.string() + "\" not found.");
    }

    std::filesystem::remove(out_path);

    std::ifstream infile{in_path};
    if (!infile) {
      throw std::runtime_error("File \"" + in_path.string()
                               + "\" not opened successfully.");
    }

    std::ofstream outfile{out_path};
    if (!outfile) {
      throw std::runtime_error("File \"" + out_path.string()
                               + "\" not created successfully.");
    }

    std::string line;

    if (std::getline(infile, line)) {
      std::stringstream instring(line);
      std::string cell;
      std::vector<std::string> row;

      while (std::getline(instring, cell, ';')) {
        if (cell.back() == '\r') {
          cell.pop_back();
        }
        if (!(outfile << cell << ';')) {
          throw std::runtime_error("File \"" + out_path.string()
                                   + "\" not written successfully.");
        }
      }

      for (auto const& config : configurations) {
        for (auto const& type : result_type) {
          cell = config + "-" + type;
          if (!(outfile << cell << ';')) {
            throw std::runtime_error("File \"" + out_path.string()
                                     + "\" not written successfully.");
          }
        }
      }

      outfile << '\n';
    }

    while (std::getline(infile, line)) {
      std::stringstream instring(line);
      std::string cell;
      std::vector<std::string> row;

      while (std::getline(instring, cell, ';')) {
        row.push_back(cell);
      }

      auto& status = row[static_cast<std::size_t>(Row::status)];
      if (status.back() == '\r') {
        status.pop_back();
      }

      auto path = instances_dir / row[static_cast<std::size_t>(Row::name)];
      path += dimacs;

      cvsp::Graph graph{path};

      auto k = static_cast<std::size_t>(
          std::stoull(row[static_cast<std::size_t>(Row::k)]));
      auto b = static_cast<std::size_t>(
          std::stoull(row[static_cast<std::size_t>(Row::b)]));

      cvsp::CVSP cvsp{k, b, graph};

      assert(cvsp.graph().vertices()
             == static_cast<std::size_t>(
                 std::stoull(row[static_cast<std::size_t>(Row::vertices)])));
      assert(cvsp.graph().edges()
             == static_cast<std::size_t>(
                 std::stoull(row[static_cast<std::size_t>(Row::edges)])));
      assert(cvsp.k() == k);
      assert(cvsp.b() == b);

      update_row(cvsp, row, configs);

      for (auto const& value : row) {
        if (!(outfile << value << ';')) {
          throw std::runtime_error("File \"" + out_path.string()
                                   + "\" not written successfully.");
        }
      }

      outfile << '\n';
    }

    infile.close();
    outfile.close();

  } catch (std::exception const& e) {
    std::cerr << "Caught exception: '" << e.what() << "'\n";
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Caught unknown exception\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}