# stack-sampler

Wait and CPU profiles of a running mysqld, taken without stopping it. Unlike
gdb-based tools such as pt-pmp, it does not attach to the process and does
not stop its threads.

The wait profile shows where each thread is blocked — futex, socket, file,
fsync, sleep — with the full stack. The CPU profile shows where the process
spends CPU. Both go to one pprof file; symbols are resolved where the profile
is read, not on the server.

On a test server with 3,500 threads at 3,500 TPS the wait profile at one pass
per second cost 2.6 s of CPU in 90 s. The CPU profile costs as much as
`perf record -a -F 99`.

## Build

On Ubuntu 24.04, install the build dependencies:

```sh
sudo apt install build-essential cmake git zlib1g-dev clang libbpf-dev libelf-dev linux-tools-generic
```

Build and install the package:

```sh
git clone https://github.com/abychko/stack-sampler.git
cmake -S stack-sampler -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build && (cd build && cpack)
sudo apt install ./build/stack-sampler_*.deb
```

`-DWITH_LIBBPF=</path/to/libbpf>` takes libbpf from another installation
instead of libbpf-dev. `-DWITH_BPF=OFF` builds without the BPF task
iterator; waits are then read from `/proc` at a higher CPU cost.

## Run

As root, on Ubuntu 24.04 x86-64. The process has to be built with frame
pointers; Percona Server 8.4 is, by default.

```sh
sudo stack-sampler --pid $(pidof mysqld) --profile wait,cpu --duration 60 --output mysqld.pb.gz
```

Read it with [pprof](https://github.com/google/pprof):

```sh
pprof -top -tagfocus=profile=wait -sample_index=wall mysqld.pb.gz
pprof -top -tagfocus=profile=cpu -sample_index=cpu mysqld.pb.gz
pprof -http=: -tagfocus=wait=futex -tagroot=wait mysqld.pb.gz
```

Wait samples are labelled with thread, tid, state, wait class, syscall,
futex address and descriptor kind; CPU samples with thread, tid and mode
(user or kernel). To read the profile on another host, put mysqld of the same
build, with debug info, into `$PPROF_BINARY_PATH/<build id>/`; pprof prints
the build id in the header of its report.

## Credits

Inspired by [quickstack](https://github.com/yoshinorim/quickstack) by
Yoshinori Matsunobu. The BPF task iterator follows xstack from
[0x.tools](https://github.com/tanelpoder/0xtools).

## License

GNU General Public License, version 2.0: see [`LICENSE`](LICENSE) and the
header of any source file.
