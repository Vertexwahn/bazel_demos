# BigPod

This is test application that reserves some RAM and gives it back after some defined time period.

Usage:

*macOS*:

Print version:

```bash
bazel run --config=macos //big_pod:BigPod.Cli -- --version
```

Reserve 2 GB of RAM and hold it for 10 seconds

```bash
bazel run --config=macos //big_pod:BigPod.Cli -- --ram_usage=2 --ram_hold_time=10
```
