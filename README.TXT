* Aim
  - This simulator 'cachesim' is used to simulate the Leve 1 cache inside CPU
    package.

* The contents in the tar file include:
  - README file
    The README file is myself :).

  - Source code files:
    - cachesim.h
      Contains the macro definitions and definitions of the struct data types.
      CacheLine: The struct for the cache line
      Cache    : The struct for the simulating cache, contain all the
        information & data for the cache
      SimResult: After the end of the simulation, all the information about the
        simulation will be put here
    - cachesim.c
      It's the source code program for the simulator. The function 'CacheSim( )'
      is the main body function in which everything happens.

    - makefile
      The file to compile the source code files and produce an executable file
      called as 'cachesim'.

  - ExpReport.txt
    contains the assumptions about the behaviour of the cache and the report
    about the experiments that I have done for the 2 issues of the cache's
    perfomance.

  - ExpMaterial.txt
    contains the raw experiment data.

* How to run the simulator and the information it produces
  - This simulator's name is 'cachesim', which accepts the dinero format data
    from STANDARD INPUT STREAM (keyboard) and stops when meeting a 'EOF'.  To
    run the simmulator, type the command in the command line in the way, like:

    gunzip -c traces/spec92/008.espresso.din.gz | cachesim 16 32 4 B A 5 200

    the meaning of parameters for cachesim is:
      The cache's size                 : 16 Kilo Bytes
      The cache line's size            : 32 Bytes
      The associativity of the cache is: 4-way
      The policy for Write Hit is      : B - 'Write Back'
      The policy for Write Miss is     : A - 'Write Allocate'
      The latency of cache access is   : 5 nano-seconds
      The latency of memory access is  : 200 nano-seconds

  - The information produced by the simulator 'cachesim'
    Take the above for example, it produces such information:

    # Cache size: 16 KB
    # Line size: 32 Bytes
    # Associativity: 4
    # Number of entries in the cache: 128
    # Write Hit Policy: WRITE BACK
    # Write Miss Policy: WRITE ALLOCATE
    # Cache latency: 5 ns
    # Memory latency: 200 ns
    Type    Accesses        Misses  %       Transfer        Bytes           Average time(ns)
    READ:   1402312         1126    0.080   26352           52704           8.762
    WRITE:  330960          1655    0.500   41687           83374           30.217
    TOTAL:  1733272         2781    0.160   68039           136078          12.859

    The data of Acceses mean how many times the data access to the cache or
    memory.

    The data of Misses mean how many times the data are not in the cache.

    The data of '%' mean miss ratio which is get by (Misses / Accesses).

    The data of Transfer mean how many times the data are transferred from or
    to the memory.

    The data of Bytes mean how many bytes are transferred fromm or to the
    memory. Note, there is a RELATIONSHIP between the data of 'Transfer' and
    'Bytes', which is the value of 'Bytes' is 2 times as much as its
    correspondant data of 'Transfer'.

    Average time is for every operation how much time it cost averagely, which
    is the time spent by all the operation divided by the number of 'Accesses'.

    There are 2 different operations considered by the simulator, that are READ
    and WRITE instructions. The total number is the sum of the 2 types.
