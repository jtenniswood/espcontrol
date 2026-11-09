# Main-loop configuration transactions

Native configuration GET and PUT requests are handed from the IDF HTTP task to
`EspControlApp::loop()`. The existing configuration service still owns validation,
generation checks and persistence. Preparation, durable commit, runtime activation
and compatibility mirroring now execute on the main loop. This is required before
selecting the PageStore renderer: configuration preparation and retirement can
touch LVGL, entity registrations and modal owners.

`ConfigurationMainLoop` has one request slot and borrows the existing HTTP document
buffer until its caller returns. It allocates no additional full-document buffer.
The IDF server processes these handlers synchronously on its single server task;
it cannot receive a second upload into that buffer while awaiting completion.
Queue synchronization uses a mutex and condition variable, not a polling loop on
the HTTP task. The main loop handles at most one request per iteration and never
holds the queue mutex while running the configuration service.

An occupied queue rejects additional callers immediately. A queued request expires
after five seconds if execution has not started. Both cases return HTTP 503 without
committing anything. After execution starts, the handler waits for the actual
result, even if the queue deadline passes. Returning early during commit would
allow its borrowed document to be overwritten and leave an ambiguous save result.
A client disconnect does not roll back an already started durable transaction.

Shutdown stops admission and cancels queued work. A running transaction retains
its buffer until it finishes. The queue, service and buffer must outlive the
waiting caller and executing main-loop stack. The owner cannot destroy them from
another thread during execution. A main-loop caller must use the configuration
service directly; submitting a blocking request from the main loop would deadlock.
Boot restoration already uses that direct path and loads the newest durable
generation before applying it.

The wire document, `/api/v1` routes, authentication, ETags, generation conflicts
and durable/mirror failure responses retain their existing semantics. The eager
renderer remains selected. No migration or nesting change is introduced here.

Host coverage runs 1,000 save/read pairs using the real service and two-slot store,
checks that preparation/storage/activation/mirroring run on the processing thread,
and checks queue expiry, overload, queued shutdown, reentrant calls and a running
save continuing beyond its queue deadline. It also covers preparation/storage
failures, generation conflicts and a mirror failure after durable publication.
Address and undefined-behaviour sanitizers pass. The real S3 SDK compiles the
application, HTTP handlers and synchronization implementation. Physical save/read,
shutdown and touch responsiveness still need an integration firmware test; host
checks do not establish whole-device memory or latency.
