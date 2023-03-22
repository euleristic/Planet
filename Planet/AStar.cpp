#include "AStar.h"

namespace AStar {

	WorkerThread::WorkerThread(Solver& solver) {

		thread = std::thread([&]() {
			
			// Might as well be while(true), but it does not look the nicest
			while (alive) {

				// Block until main thread gives the signal to begin
				beginSignal.acquire();

				// Check if main thread called destructor
				if (!alive) 
					break;

				// Run solver algorithm
				solver.Run();

				// Signal main thread that work is complete and allow it to unblock
				completeSignal.release();

			}
		});
	}

	WorkerThread::~WorkerThread() {
		// Signal worker to begin with !alive and wait for thread to join
		alive = false;
		beginSignal.release();
		if (thread.joinable())
			thread.join();
	}

	void Solver::Solve(const std::vector<Geometry::Polygon>& world, const Geometry::Vector2<float> startingPosition, const Geometry::Vector2<float> goal) {
		this->world = world;
		this->goal = goal;
		completePath.reset();
		fringe = std::priority_queue<Node, std::vector<Node>, std::function<bool(const Node&, const Node&)>>([&goal](const Node& lhs, const Node& rhs) {
			// We want to prioritize a node n the lower its f(n) = g(n) + h(n), where
			// g(n) is the cost of the node n, i.e. the length of the path to it, and
			// h(n) is the heuristic, in this case the distance to the goal

			return lhs.pathLength() + (goal - lhs.position()).Magnitude() > rhs.pathLength() + (goal - rhs.position()).Magnitude();
		});
		fringe.push(Node(startingPosition, {}));

		// Run threadpool
		std::for_each(std::execution::unseq, threadPool.begin(), threadPool.end(), [](auto& thread) {
			// Signal the worker to begin
			thread->beginSignal.release();
		});

		// And main thread
		Run();

		// Allow all threads to complete
		std::for_each(std::execution::unseq, threadPool.begin(), threadPool.end(), [](auto& thread) {
			// Block until that thread signals completion
			thread->completeSignal.acquire();
		});
		
		// Clean up solver
		discoveredNodes.clear();
		fringe = {};
	}


	// Handles the discovery of a node. If it is already discovered, do nothing. Else, insert into discovered set and fringe
	void Solver::Discover(const Node& node) {
		// Scope for discovered set lock guard (probably quite insignificant)
		{
			std::lock_guard lock(discoveredMutex);
			if (discoveredNodes.contains(node)) {
				if (discoveredNodes.find(node)->pathLength() < node.pathLength()) {
					return;
				}
				else {
					discoveredNodes.erase(node);
				}
			}
			discoveredNodes.insert(node);
		}
		std::lock_guard lock(fringeMutex);
		fringe.push(node);
	}

	// Locks the fringe and takes the top node
	std::optional<Solver::Node> Solver::AqcuireNextNodeInFringe() {
		std::lock_guard lock(fringeMutex);
		if (fringe.empty()) {
			return {};
		}
		Node top = fringe.top();
		fringe.pop();
		return std::make_optional(top);
	}

	// Threaded function
	void Solver::Run() {
		while (std::optional<Node> node = AqcuireNextNodeInFringe()) {

			// Done?
			if (completePath) {
				std::lock_guard lock(pathMutex);
				if (node->pathLength() >= completePath->pathLength()) {
					break;
				}
			}

			// Is this the goal?
			if (node->position() == goal) {
				std::lock_guard lock(pathMutex);
				completePath = node;
			}

			// Is the goal visible?
			if (!Geometry::Intersect(world, { node->position(), goal })) {
				Discover(Node(goal, node->path));
			}

			// Discover neighbours
			std::for_each(std::execution::unseq, world.begin(), world.end(), [&](const Geometry::Polygon& polygon) {
				auto found = std::find_if(std::execution::unseq, polygon.vertices.begin(), polygon.vertices.end(), 
					[&node](const Geometry::Vector2<float>& vertex) { return node->position() == vertex; }); // unseq, at most one vertex could be the node
				if (found != polygon.vertices.end()) {
					// node belongs to polygon

					// Discover node -1
					if (found == polygon.vertices.begin()) {
						Discover(Node(polygon.vertices.back(), node->path));
					}
					else {
						Discover(Node(*(found - 1), node->path));
					}

					// Discover node +1
					if (found == polygon.vertices.end() - 1) {
						Discover(Node(polygon.vertices.front(), node->path));
					}
					else {
						Discover(Node(*(found + 1), node->path));
					}

				}
				else {

					// Discover all "visible" polygon angular extrema
					const auto& [leftMost, rightMost] = Geometry::GetAnglularExtrema(polygon, node->position());
					if (!Geometry::Intersect(world, { node->position(), leftMost })) {
						Discover(Node(leftMost, node->path));
					}
					if (!Geometry::Intersect(world, { node->position(), rightMost })) {
						Discover(Node(rightMost, node->path));
					}

				}
			});
		}
	};

	Geometry::LineSequence FindPath(const std::vector<Geometry::Polygon>& world, const Geometry::Vector2<float>& startingPosition, const Geometry::Vector2<float>& goal) {

		// If you wish, uncomment and #include iostream to test the difference.

		//auto start = std::chrono::steady_clock::now();
		solver.Solve(world, startingPosition, goal);
		// auto duration = std::chrono::steady_clock::now() - start;

		// std::cout << solver.threadPool.size() + 1 << " threads: " << duration << '\n';

		return solver.completePath ? solver.completePath->path : Geometry::LineSequence{};
	}

	void AddThread() {
		solver.threadPool.push_back(std::make_unique<WorkerThread>(solver));
	}

	void RemoveThread() {
		if (!solver.threadPool.empty()) {
			solver.threadPool.pop_back();
		}
	}

	size_t ThreadCount() {
		return solver.threadPool.size() + 1;
	}
}
