#include <iostream>
#include <fstream>
#include <string>
#include <format>
#include <vector>
#include <filesystem>
#include <ranges>
#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/json.hpp>

#include "project.hpp"

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cerr << std::format("Usage: {} <vscode_project directory> <keyword...>\n", argv[0]);
        return 1;
    }

    auto dirPath = std::filesystem::path(argv[1]);
    if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
        std::cerr << std::format("Error: '{}' is not a valid directory\n", dirPath.string());
        return 1;
    }

    auto keywordRanges = std::ranges::subrange(argv + 2, argv + argc)
        | std::ranges::views::transform([](const std::string& keyword) { return boost::algorithm::to_lower_copy(keyword); });
    auto keywords = std::vector<std::string>(keywordRanges.begin(), keywordRanges.end());

    auto projects = std::vector<std::unique_ptr<Project>>();
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".png") continue;

        auto file = std::ifstream(entry.path());
        if (!file.is_open()) continue;

        std::string line;
        while (std::getline(file, line)) {
            projects.push_back(Project::create(entry.path().filename(), line));
        }
    }

    auto items = boost::json::array();
    int i = 0;
    for (const auto& project : projects) {
        int score = project->getScore(keywords);
        if (score <= 0) continue;

        auto item = boost::json::object{
            {"uid", std::format("{}", i)},
            {"title", project->projectName},
            {"subtitle", std::format("open <{}> in <{}>", project->projectName, project->serverName)},
            {"arg", project->path},
            {"icon", boost::json::object{{"path", std::format("{}/{}", dirPath.string(), "vscode_icon.png")}}},
            {"score", score}
        };

        items.push_back(item);
    }

    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.as_object().at("score").as_int64() < b.as_object().at("score").as_int64();
    });

    auto item = boost::json::object();
    item["items"] = items;

    std::cout << boost::json::serialize(item) << std::endl;

    return 0;
}
