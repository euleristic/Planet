#pragma once

#include "Shapes.h"
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <unordered_set>
#include <algorithm>
#include <optional>
#include <semaphore>
#include <numeric>
#include <execution>
#include <chrono>

namespace AStar {

	class Solver;

	// Thread wrapper which associates a std::thread with a state
	struct WorkerThread {

		// Signals allow the worker and main threads to when to begin execution and when it is complete
		std::binary_semaphore beginSignal{0}, completeSignal{0};
		std::atomic<bool> alive{true};
		std::thread thread;
		WorkerThread(Solver& solver);
		~WorkerThread();
	};

	class Solver {

		struct Node {

			Node(Geometry::Vector2<float> position, Geometry::LineSequence parentPath) : path(parentPath) {
				path.vertices.push_back(position);
			}
			Geometry::LineSequence path;
			Geometry::Vector2<float> position() const {
				return path.vertices.back();
			}

			float pathLength() const {
				std::vector<Geometry::Vector2<float>> steps(path.vertices.size());
				auto stepsEnd = std::adjacent_difference(std::execution::unseq, path.vertices.begin(), path.vertices.end(), steps.begin()); // unseq is always fastest
				return std::transform_reduce(std::execution::unseq, steps.begin(), stepsEnd, 0.0f, std::plus{}, [](auto& vector) { return vector.Magnitude(); }); // here too
			}
		};

		friend Geometry::LineSequence FindPath(const std::vector<Geometry::Polygon>& world, const Geometry::Vector2<float>& startingPosition, const Geometry::Vector2<float>& goal);
		friend void AddThread();
		friend void RemoveThread();
		friend size_t ThreadCount();

		friend WorkerThread;

		std::vector<std::unique_ptr<WorkerThread>> threadPool;

		std::mutex pathMutex;
		std::optional<Node> completePath;

		std::vector<Geometry::Polygon> world;
		Geometry::Vector2<float> goal;

		// Threadsafe discovered set
		std::mutex discoveredMutex;
		std::unordered_set<Node, decltype([](const Node& node) {
			return std::hash<float>{}(node.position().x) ^ std::hash<float>{}(node.position().y);
		}), decltype([](const Node& lhs, const Node& rhs) {
			return lhs.position() == rhs.position();
		})> discoveredNodes;

		// Threadsafe fringe
		std::mutex fringeMutex;
		std::priority_queue<Node, std::vector<Node>, std::function<bool(const Node&, const Node&)>> fringe;

		void Solve(const std::vector<Geometry::Polygon>& world, const Geometry::Vector2<float> startingPosition, const Geometry::Vector2<float> goal);
		void Discover(const Node& node);
		std::optional<Node> AqcuireNextNodeInFringe();
		void Run();
	};

	

	static Solver solver;
	Geometry::LineSequence FindPath(const std::vector<Geometry::Polygon>& world,
		const Geometry::Vector2<float>& startingPosition, const Geometry::Vector2<float>& goal);

	void AddThread();
	void RemoveThread();
	size_t ThreadCount();
}