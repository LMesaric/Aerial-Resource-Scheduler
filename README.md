# A Heuristic Algorithm for Scheduling Aerial Resources for the Extinction of Large-scale Wildfires

Master of Science Thesis at Faculty of Electrical Engineering and Computing, University of Zagreb, 
under the mentorship of Lea Skorin-Kapov, PhD and co-mentorship of Nina Skorin-Kapov, PhD.

### Abstract

Expeditiously scheduling aerial resources is of vital importance when it comes to fighting large-scale wildfires.
In this thesis, a heuristic search algorithm is proposed to solve the resource scheduling and assignment problem, based on an existing integer linear model which meets Spanish aviation regulations.
The heuristic algorithm is implemented in software using the C++ programming language.
A comparison is made between the proposed algorithm and the integer linear programming (ILP) model.
Although the ILP gives optimal solutions, it is not scalable to larger instances.
Results show that the heuristic algorithm obtains optimal or near-optimal solutions for smaller instances, and consistently outperforms ILP solutions obtained after two hours of execution time for larger instances.
Furthermore, the heuristic is highly scalable, running in under five minutes for all cases, making it quite applicable in a dynamic firefighting environment.

**Keywords:** aerial firefighting, wildfire, scheduling, heuristic algorithm, combinatorial optimization, GRASP, large neighborhood search, simulated annealing.
