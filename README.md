# Filter .fastq files (serial-mpi-openMP)

In order to run the serial c program (filter-serial.c), you can compile it using gcc and when running it pass as 1st argument the input .fastq file and as 2nd argument the .fastq output file.

In order to run the MPI c program (filter-mpi.c), you can compile it using mpicc and when running it pass as 1st argument the input .fastq file and as 2nd argument the .fastq output file (also don't forget to set the number of processes).

In order to run the OpenMP c program (filter-openmp.c), you can compile it using gcc and the parameter -fopenmp and when running it pass as 1st argument the input .fastq file, as 2nd argument the .fastq output file and as 3rd argument the number of threads.
