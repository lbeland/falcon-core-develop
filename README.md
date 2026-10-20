# falcon-core-develop — IPC benchmark

Origin: https://github.com/falcon-eyrie/falcon-core

[Falcon](https://github.com/falcon-eyrie/falcon-core) is a C++ real-time
processing framework built around a graph of processors connected by ring
buffers. `c_benchmark.py` drives a two-processor graph (`Producer` →
`Consumer`, under `extensions/falcon-fklab-extensions/processors/`) and
times message delivery.

## Install

From the repo root:

```bash
./bootstrap.sh --only falcon-core-develop --with-builds --with-system-deps
```

This installs `libzmq3-dev`/`gcc-14`/`g++-14`, then configures and builds
falcon in Release mode using the venv's own `cmake` (Ubuntu 24.04's system
`cmake` 3.28 has no GNU flag mapping for C++26, which this project
requires — pip installs a newer one into `falcon-core-develop/venv`
automatically) and `gcc-14`/`g++-14`. Without `--with-system-deps`, run the
`setcap` command it prints manually so falcon can get real-time thread
priority.

## Run the benchmark

```bash
cd falcon-core-develop
venv/bin/python3 c_benchmark.py \
  --n_messages 1000 \
  --num_channels 2 \
  --msg_size 2 \
  --t_wait 0.0001 \
  --overwrite
```

This writes `extensions/falcon-fklab-extensions/resources/graphs/benchmark.yaml`
and launches `build/falcon/falcon` (via `sudo chrt -f 99`) with that graph.
Falcon's `resources:` storage path in `.falcon/config.yaml` is relative
(`./extensions/...`), so it only resolves correctly when falcon is launched
with its CWD set to this directory — `c_benchmark.py` already does this.

- `--n_messages`: number of timed messages sent
- `--num_channels` / `--msg_size`: shape of each message
- `--t_wait`: seconds the producer sleeps between sends (0 = no wait)
- `--overwrite`: rerun even if results for this channel/size combination
  already exist

Results are written to `rt_c_results/<num_channels>_<msg_size>_{Producer,Consumer,send_times,recv_times,process_times}.csv`.
