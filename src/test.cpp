#include <random>
#include <vector>
#include "plotter.h"
#include <queue>

vector<vector<int>> distance_field(const vector<int>& start, const vector<vector<int>>& comap, int MOV)
{
    int WIDTH = comap[0].size();
    int HEIGHT =comap.size();
    using Node = tuple<int, int, int>;
    priority_queue<Node, vector<Node>, greater<Node>> open;
    vector<vector<int>> distance(HEIGHT, vector<int>(WIDTH, -1));
    distance[start[1]][start[0]] = 0;
    open.push({0, start[0], start[1]});

    vector<vector<int>> helpers = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

    while (!open.empty())
    {
        auto [current_cost, x, y] = open.top();
        open.pop();

        for (const vector<int>& helper : helpers)
        {
            int new_x = x + helper[0];
            int new_y = y + helper[1];
            if (new_x < 0 || new_y < 0 || new_x >= WIDTH || new_y >= HEIGHT) { continue; }
            int next_cost = current_cost + comap[new_y][new_x];
            if (comap[new_y][new_x] > MOV)
                continue;
            if (next_cost < distance[new_y][new_x] || distance[new_y][new_x] == -1)
            {
                distance[new_y][new_x] = next_cost;
                open.push({next_cost, new_x, new_y});
            }
        }
    }
    return distance;
}

vector<vector<int>> locate(vector<vector<int>>& out, const vector<int>& start, const vector<int>& end, const vector<vector<int>>& dijkstra, int MOV)
{
    vector<vector<int>> helpers = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    int x = start[0]; int y = start[1];
    for (const vector<int>& coord : helpers)
    {
        int new_x = x + coord[0]; int new_y = y + coord[1];
        if (new_x < 0 || new_y < 0 || new_x >= dijkstra.size() || new_y >= dijkstra[0].size() ||
            dijkstra[new_y][new_x] != dijkstra[y][x]+1)
            continue;
        if (dijkstra[new_y][new_x] == dijkstra[y][x]+1)
            out.push_back({new_x, new_y});
        if (vector<int>({new_x, new_y}) != end)
            locate(out, {new_x, new_y}, end, dijkstra, MOV); break;
    }
    return out;
}

int main()
{
    const int WIDTH = 50;
    const int HEIGHT = 50;
    int MOV = 5;
    vector<vector<int>> map(HEIGHT, vector<int>(WIDTH, 1));
    vector<int> start = {0, 0};
    vector<vector<int>> dijkstra = distance_field(start, map, MOV);
    vector<int> end = {49, 49};
    vector<vector<int>> out;
    out.push_back(start);
    out = locate(out, start, end, dijkstra, MOV);x
    Canvas canvas(WIDTH, HEIGHT);
    canvas.plot(out, 1);
    canvas.draw();
    return 0;
}
