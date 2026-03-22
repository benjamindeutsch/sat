# Diary
I implemented the standard SAT encoding for k-colorability, as given in the exercise description. 
### Symmetry Breaking
I added very simple symmetry breaking, by forcing node 1 to have color 1, and forcing a neighbour of node 1 to have color 2. To my surprise, this simple technique improved performance on the benchmarks by a lot.
### At most one color constraint
At first, I did not add the constraints forcing each vertex to have at most one color (in the following i call these constraints amoc), since they are not necessary for checking satisfiability. Using manual testing I could not decide whether adding the amoc constraints sped up search or not. So I benchmarked it and the results are still not completely conclusive.
On average proof search without amoc constraints is 61% slower than with amoc constraints. However, this result is scewed by extreme outliers, like fpsol2.i.2.col, fpsol2.i.3.col and david.col. On a typical instance proof search with amoc is 21% faster.

In the end I decided to keep the amoc constraints.

**Benchmarks**
| Instance           | k    | Normal  | No-AMOC | Ratio |
|-------------------|------|---------|---------|-------|
| fpsol2.i.1.col    | 65   | 0.280s  | 0.120s  | 0.43  |
| fpsol2.i.1.col    | 64   | TIMEOUT | TIMEOUT | N/A   |
| fpsol2.i.2.col    | 30   | 0.170s  | 2.280s  | 13.41 |
| fpsol2.i.2.col    | 29   | TIMEOUT | TIMEOUT | N/A   |
| fpsol2.i.3.col    | 30   | 0.090s  | 1.650s  | 18.33 |
| fpsol2.i.3.col    | 29   | TIMEOUT | TIMEOUT | N/A   |
| inithx.i.1.col    | 54   | 1.260s  | 0.540s  | 0.43  |
| inithx.i.1.col    | 53   | TIMEOUT | TIMEOUT | N/A   |
| inithx.i.2.col    | 31   | 0.470s  | 0.210s  | 0.45  |
| inithx.i.2.col    | 30   | TIMEOUT | TIMEOUT | N/A   |
| inithx.i.3.col    | 31   | 0.450s  | 0.290s  | 0.64  |
| inithx.i.3.col    | 30   | TIMEOUT | TIMEOUT | N/A   |
| mulsol.i.1.col    | 49   | 0.240s  | 0.110s  | 0.46  |
| mulsol.i.1.col    | 48   | TIMEOUT | TIMEOUT | N/A   |
| mulsol.i.2.col    | 31   | 0.130s  | 0.090s  | 0.69  |
| mulsol.i.2.col    | 30   | TIMEOUT | TIMEOUT | N/A   |
| mulsol.i.3.col    | 31   | 0.110s  | 0.070s  | 0.64  |
| mulsol.i.3.col    | 30   | TIMEOUT | TIMEOUT | N/A   |
| mulsol.i.4.col    | 31   | 0.180s  | 0.110s  | 0.61  |
| mulsol.i.4.col    | 30   | TIMEOUT | TIMEOUT | N/A   |
| mulsol.i.5.col    | 31   | 0.100s  | 0.060s  | 0.60  |
| mulsol.i.5.col    | 30   | TIMEOUT | TIMEOUT | N/A   |
| zeroin.i.1.col    | 49   | 0.380s  | 0.120s  | 0.32  |
| zeroin.i.1.col    | 48   | TIMEOUT | TIMEOUT | N/A   |
| zeroin.i.2.col    | 30   | 0.110s  | 0.060s  | 0.55  |
| zeroin.i.2.col    | 29   | TIMEOUT | TIMEOUT | N/A   |
| zeroin.i.3.col    | 30   | 0.150s  | 0.090s  | 0.60  |
| zeroin.i.3.col    | 29   | TIMEOUT | TIMEOUT | N/A   |
| le450_5a.col      | 5    | 0.370s  | 0.340s  | 0.92  |
| le450_5a.col      | 4    | 0.160s  | 0.150s  | 0.94  |
| le450_5b.col      | 5    | 0.330s  | 0.490s  | 1.48  |
| le450_5b.col      | 4    | 0.170s  | 0.130s  | 0.76  |
| le450_5c.col      | 5    | 0.450s  | 0.380s  | 0.84  |
| le450_5c.col      | 4    | 0.360s  | 0.280s  | 0.78  |
| le450_5d.col      | 5    | 0.350s  | 0.280s  | 0.80  |
| le450_5d.col      | 4    | 0.390s  | 0.250s  | 0.64  |
| le450_15a.col     | 15   | 7.520s  | 9.420s  | 1.25  |
| le450_15a.col     | 14   | TIMEOUT | TIMEOUT | N/A   |
| le450_15b.col     | 15   | 10.790s | 4.660s  | 0.43  |
| le450_15b.col     | 14   | TIMEOUT | TIMEOUT | N/A   |
| le450_25a.col     | 25   | 2.560s  | 9.160s  | 3.58  |
| le450_25a.col     | 24   | TIMEOUT | TIMEOUT | N/A   |
| le450_25b.col     | 25   | 2.470s  | 2.060s  | 0.83  |
| le450_25b.col     | 24   | TIMEOUT | TIMEOUT | N/A   |
| miles250.col      | 8    | 0.040s  | 0.020s  | 0.50  |
| miles250.col      | 7    | 0.030s  | 0.020s  | 0.67  |
| miles500.col      | 20   | 0.240s  | 0.320s  | 1.33  |
| miles750.col      | 31   | 0.740s  | 0.940s  | 1.27  |
| miles1000.col     | 42   | 3.150s  | 2.480s  | 0.79  |
| miles1500.col     | 73   | 7.400s  | 6.350s  | 0.86  |
| games120.col      | 9    | 0.010s  | 0.010s  | 1.00  |
| games120.col      | 8    | 0.240s  | 0.140s  | 0.58  |
| anna.col          | 11   | 0.010s  | 0.010s  | 1.00  |
| anna.col          | 10   | 1.520s  | 0.700s  | 0.46  |
| david.col         | 11   | 0.010s  | 0.010s  | 1.00  |
| david.col         | 10   | 0.320s  | 2.400s  | 7.50  |
| homer.col         | 13   | 0.580s  | 0.270s  | 0.47  |
| huck.col          | 11   | 0.010s  | 0.010s  | 1.00  |
| huck.col          | 10   | 0.460s  | 0.140s  | 0.30  |
| jean.col          | 10   | 0.010s  | 0.000s  | 0.00  |
| jean.col          | 9    | 0.290s  | 0.110s  | 0.38  |
| queen6_6.col      | 7    | 0.020s  | 0.030s  | 1.50  |
| queen6_6.col      | 6    | 0.040s  | 0.040s  | 1.00  |
| queen7_7.col      | 7    | 0.030s  | 0.100s  | 3.33  |
| queen7_7.col      | 6    | 0.020s  | 0.010s  | 0.50  |
| queen8_8.col      | 9    | 2.180s  | 2.100s  | 0.96  |
| queen8_12.col     | 12   | 0.180s  | 0.230s  | 1.28  |
| queen9_9.col      | 10   | 3.130s  | 12.390s | 3.96  |
| queen11_11.col    | 10   | 2.090s  | 0.380s  | 0.18  |
| myciel5.col       | 5    | 0.360s  | 0.480s  | 1.33  |
| myciel7.col       | 8    | 0.010s  | 0.010s  | 1.00  |