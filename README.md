# BFHT

# Build All
```
$ make
```

# Before Run
[You should mount your PMEM* to a NVM-aware filesystem with DAX.](https://docs.pmem.io/persistent-memory/getting-started-guide/creating-development-environments/linux-environments/linux-memmap)
```
$ sudo mkfs.xfs /dev/pmem0
$ sudo mkdir /mnt/pmem
$ sudo mount -o dax /dev/pmem0 /mnt/pmem
$ sudo modprobe msr
```

# Run with PiBench
```
$ make
$ cd bin
$ sudo ./PiBench [lib.so] [args...]

#some problem to generate Dash for make command
eg:
make[3]: execvp: ../../src/../utils/check-os.sh: Permission denied

solution as follow:
```
 $ chmod 777 -R ./hash/Dash/pmdk/utils/check-os.sh
```
then reexecute the make command
```
$ make
```
eg:
$ sudo ./PiBench ./Dash.so -S 16777216 -r 0 -i 1 -d 0 -t 1 -N 0 -p 200000000
```
## PiBench 
The `PiBench` executable is generated and supports the following arguments:
```
$ ./PiBench --help
Benchmark framework for persistent indexes.
Usage:
  PiBench [OPTION...] INPUT

      --input arg           Absolute path to library file
  -n, --records arg         Number of records to load (default: 0)
  -N, --negative_ratio arg  Negative search ratio (default: 0.000000)
  -S, --hash_size arg       hashtable size (default: 4096)
  -M, --test_mode arg       Test mode (default: THROUGHPUT)
  -p, --operations arg      Number of operations to execute (default: 1024)
  -t, --threads arg         Number of threads to use (default: 1)
  -r, --read_ratio arg      Ratio of read operations (default: 0.000000)
  -i, --insert_ratio arg    Ratio of insert operations (default: 1.000000)
  -d, --remove_ratio arg    Ratio of remove operations (default: 0.000000)
      --skip_load           Skip the load phase (default: true)
      --distribution arg    Key distribution to use (default: UNIFORM)
      --help                Print help
```
### Test mode

#### Throughput
eg: 
```
insert
$ sudo ./PiBench ./Dash.so -S 16777216 -r 0 -i 1 -d 0 -t 1 -N 0 -p 200000000 --distribution UNIFORM -M THROUGHPUT

postive search
$ sudo ./PiBench ./Dash.so -S 16777216 -r 1 -i 0 -d 0 -t 1 -N 0 --skip_load=false -p 200000000 --distribution UNIFORM -M THROUGHPUT

negative search
$ sudo ./PiBench ./Dash.so -S 16777216 -r 1 -i 0 -d 0 -t 1 -N 1 --skip_load=false -p 200000000 --distribution UNIFORM -M THROUGHPUT 

delete
$ sudo ./PiBench ./Dash.so -S 16777216 -r 0 -i 0 -d 1 -t 1 -N 0 --skip_load=false -p 200000000 --distribution UNIFORM -M THROUGHPUT
```
#### LOAD_FACTOR

the load factor of hash table during execution.
eg:
```
$ sudo ./PiBench ./BFHT.so -S 16777216 -p 200000000 -M LOAD_FACTOR
```

For Dash, you need to recompile with `DA_FLAGS=-DCOUNTING`
```
$ make clean -C hash/Dash
$ make DA_FLAGS=-DCOUNTING -C hash/Dash
$ make
```

#### PM info

```
insert
$ sudo ./PiBench ./Dash.so -S 16777216 -r 0 -i 1 -d 0 -t 1 -N 0 -p 200000000 --distribution UNIFORM -M PM

positive search
$ sudo ./PiBench ./Dash.so -S 16777216 -r 1 -i 0 -d 0 -t 1 -N 0 --skip_load=false -p 200000000 --distribution UNIFORM -M PM 

negative search
$ sudo ./PiBench ./Dash.so -S 16777216 -r 1 -i 0 -d 0 -t 1 -N 1 --skip_load=false -p 200000000 --distribution UNIFORM -M PM 

delete
$ sudo ./PiBench ./Dash.so -S 16777216 -r 0 -i 0 -d 1 -t 1 -N 0 --skip_load=false -p 200000000 --distribution UNIFORM -M PM
```

#### LATENCY

```
insert
$ sudo ./PiBench ./Dash.so -S 16777216 -r 0 -i 1 -d 0 -t 32 -N 0 -p 200000000 --distribution UNIFORM -M LATENCY

positive search
$ sudo ./PiBench ./Dash.so -S 16777216 -r 1 -i 0 -d 0 -t 32 -N 0 --skip_load=false -p 200000000 --distribution UNIFORM -M LATENCY

negative search
$ sudo ./PiBench ./Dash.so -S 16777216 -r 1 -i 0 -d 0 -t 32 -N 1 --skip_load=false -p 200000000 --distribution UNIFORM -M LATENCY 

delete
$ sudo ./PiBench ./Dash.so -S 16777216 -r 0 -i 0 -d 1 -t 32 -N 0 --skip_load=false -p 200000000 --distribution UNIFORM -M LATENCY
```

#### RECOVERY only for BFHT

```
$ sudo ./PiBench ./BFHT.so -S 16777216 -r 0 -i 1 -d 0 -t 1 -N 0 -p 200000000 --distribution UNIFORM -M RECOVERY
```

