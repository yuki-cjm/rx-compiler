#include "const_deps.hpp"

#include <functional>

namespace rx::semantic {

void ConstDepGraph::addDependency(const std::string &from,
                                  const std::string &to) {
    edges_[from].push_back(to);
}

bool ConstDepGraph::hasCycle() const { return !findCycle().empty(); }

std::vector<std::string> ConstDepGraph::findCycle() const {
    enum class Color {
        White,
        Gray,
        Black,
    };

    std::map<std::string, Color> color;
    std::vector<std::string> path;
    std::vector<std::string> cycle;

    std::function<bool(const std::string &)> visit =
        [&](const std::string &node) -> bool {
        color[node] = Color::Gray;
        path.push_back(node);

        auto found = edges_.find(node);
        if (found != edges_.end()) {
            for (const std::string &next : found->second) {
                if (color[next] == Color::White) {
                    if (visit(next)) {
                        return true;
                    }
                } else if (color[next] == Color::Gray) {
                    // 从 path 中截取环（next 到末尾）。
                    auto begin = std::find(path.begin(), path.end(), next);
                    if (begin != path.end()) {
                        cycle.assign(begin, path.end());
                        cycle.push_back(next);
                    }
                    return true;
                }
            }
        }

        path.pop_back();
        color[node] = Color::Black;
        return false;
    };

    for (const auto &[node, unused] : edges_) {
        (void)unused;
        if (color[node] == Color::White) {
            if (visit(node)) {
                break;
            }
        }
    }
    return cycle;
}

} // namespace rx::semantic
