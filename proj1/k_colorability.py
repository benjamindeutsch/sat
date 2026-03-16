import sys

def parse_arguments():
    if len(sys.argv) != 3:
        print("Usage: python k_colorability.py <graph_file.col> <k>")
        sys.exit(1)
    graph_file = sys.argv[1]
    try:
        k = int(sys.argv[2])
        if k <= 0:
            raise ValueError
    except ValueError:
        print("Error: k must be a positive integer.")
        sys.exit(1)
    return graph_file, k

def get_adjacency_list(graph_file):
    graph = {}
    with open(graph_file, 'r') as f:
        for line in f:
            if line.startswith('e'):
                _, u, v = line.split()
                u, v = int(u), int(v)
                if(u == v): continue

                if u not in graph:
                    graph[u] = []
                if v not in graph:
                    graph[v] = []
                graph[u].append(v)
                graph[v].append(u)
    return graph

def get_edge_list(graph_file):
    edges = []
    n = 0
    with open(graph_file, 'r') as f:
        for line in f:
            if line.startswith('p'):
                _, _, n, _ = line.split()
                n = int(n)
            elif line.startswith('e'):
                _, u, v = line.split()
                u, v = int(u), int(v)
                if(u == v): continue

                edges.append((u, v))
    return n, edges

# For each vertex i, we create k variables: (i-1)*k + 1, (i-1)*k + 2, ..., (i-1)*k + k
# If in a model variable (i-1)*k + c is true, it means vertex i can be colored with color c.
def simple_sat_reduction(edges, n, k):
    clauses = []
    for i in range(1, n + 1):
        # Each vertex must be colored with at least one color
        clause = [(i-1)*k + c for c in range(1, k + 1)]
        clauses.append(clause)

    # Adjacent vertices have different colors
    for u, v in edges:
        for c in range(1, k + 1):
            clause = [-((u-1)*k + c), -((v-1)*k + c)]
            clauses.append(clause)
            
    return clauses

def dimacs(clauses, n, k):
    num_vars = n * k
    num_clauses = len(clauses)
    dimacs_str = f"p cnf {num_vars} {num_clauses}\n"
    for clause in clauses:
        dimacs_str += " ".join(map(str, clause)) + " 0\n"
    return dimacs_str


def main():
    graph_file, k = parse_arguments()
    n, edges = get_edge_list(graph_file)
    clauses = simple_sat_reduction(edges, n, k)
    dimacs_str = dimacs(clauses, n, k)
    print(dimacs_str)

if __name__ == "__main__":
    main()