import sys

def parse_arguments():
    args = sys.argv[1:]
    disable_amoc = '--disable-amoc' in args
    args = [a for a in args if a != '--disable-amoc']
    if len(args) != 2:
        print("Usage: python k_colorability.py <graph_file.col> <k> [--disable-amoc]")
        sys.exit(1)
    graph_file = args[0]
    try:
        k = int(args[1])
        if k <= 0:
            raise ValueError
    except ValueError:
        print("Error: k must be a positive integer.")
        sys.exit(1)
    return graph_file, k, disable_amoc

def get_edge_list(graph_file):
    """
    Returns a list of edges and the number of vertices in the graph.
    """
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

def simple_sat_reduction(edges, n, k, disable_amoc=False):
    """
    We use the simple encoding for k-colorability from the task description.
    For each vertex i, we create k variables: (i-1)*k + 1, (i-1)*k + 2, ..., (i-1)*k + k
    If in a model variable (i-1)*k + c is true, it means vertex i can be colored with color c.

    """
    clauses = []
    # Each vertex must be colored with at least one color
    for i in range(1, n + 1):
        clause = [(i-1)*k + c for c in range(1, k + 1)]
        clauses.append(clause)

    # Each vertex can be colored with at most one color
    # If the --disable-amoc flag is set, these constraints are skipped
    if not disable_amoc:
        for i in range(1, n + 1):
            for c1 in range(1, k + 1):
                for c2 in range(c1 + 1, k + 1):
                    clause = [-((i-1)*k + c1), -((i-1)*k + c2)]
                    clauses.append(clause)

    # Adjacent vertices must have different colors
    for u, v in edges:
        for c in range(1, k + 1):
            clause = [-((u-1)*k + c), -((v-1)*k + c)]
            clauses.append(clause)

    # Simple symmetry breaking: vertex 1 is colored with 1 and one of its neighbors is colored with 2
    clauses.append([1])
    for u, v in edges:
        if u == 1:
            clauses.append([((v-1)*k + 2)])
            break
        elif v == 1:
            clauses.append([((u-1)*k + 2)])
            break

    return clauses

def dimacs(clauses, num_vars):
    num_clauses = len(clauses)
    dimacs_str = f"p cnf {num_vars} {num_clauses}\n"
    for clause in clauses:
        dimacs_str += " ".join(map(str, clause)) + " 0\n"
    return dimacs_str


def main():
    graph_file, k, disable_amoc = parse_arguments()
    n, edges = get_edge_list(graph_file)
    clauses = simple_sat_reduction(edges, n, k, disable_amoc)
    dimacs_str = dimacs(clauses, n*k)
    print(dimacs_str)

if __name__ == "__main__":
    main()