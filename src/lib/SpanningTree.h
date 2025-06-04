#include "DAG.h"

class UnionFind {
	public:
		UnionFind(int n) {
			parent.resize(n);
			rank.resize(n, 0);
			// Initialize each element as its own parent
			for (int i = 0; i < n; i++) {
				parent[i] = i;
			}
		}

		// Find with path compression
		int find(int x) {
			if (parent[x] != x) {
				// Path compression
				parent[x] = find(parent[x]); 
			}
			return parent[x];
		}

		// Union by rank
		bool unite(int x, int y) {
			int rootX = find(x);
			int rootY = find(y);

			if (rootX == rootY) {
				// Already in same set (would create cycle)
				return false; 
			}

			// Union by rank
			if (rank[rootX] < rank[rootY]) {
				parent[rootX] = rootY;
			} else if (rank[rootX] > rank[rootY]) {
				parent[rootY] = rootX;
			} else {
				parent[rootY] = rootX;
				rank[rootX]++;
			}
			return true;
		}

	private: 
		vector<int> parent, rank;
};

class SpanningTree {
	public:
		static vector<GraphEdge*> kruskalMaxSpanningTree(DAG* dag) {
			vector<GraphEdge*> mst;
			vector<GraphEdge*> edges = dag->getEdges();
			vector<GraphNode*> vertices = dag->getNodes();

			// Initialize Union-Find
			UnionFind uf(vertices.size());

			// Process Edges
			for (GraphEdge* edge : edges) {
				if (uf.unite(edge->getSrc()->getId(), edge->getDst()->getId())) {
					mst.push_back(edge);

					if (mst.size() == vertices.size() - 1) {
						break;
					}
				}
			}
			return mst;
		}
};
