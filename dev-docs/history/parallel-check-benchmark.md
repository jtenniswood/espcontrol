<!-- DEV-DOC-STATUS: historical -->

# Parallel Check Benchmark

> Historical record: this benchmark was captured on 2026-07-12. It supports the
> current default but is not a substitute for measuring the present check graph.

The initial Darwin arm64 benchmark ran the complete non-browser fast profile 20
times sequentially and 20 times with four workers. Every run passed with the
same task statuses and left the tracked diff fingerprint unchanged.

Median duration improved from 11.850 seconds to 9.635 seconds, an 18.7%
reduction. Because that was below the planned 20% threshold, parallel mode
remained explicit-only through `check:parallel`; normal npm aliases and CI kept
their one-worker default.
