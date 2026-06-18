# Architecture & Tech Stack

> A high-concurrency real-time message push server built with C++17 + epoll ET + multi-threaded Reactor.
> Supports broadcast, private chat, group chat, offline messages, and ACK confirmation — with a web admin panel.

---

## Architecture Overview

```
                    ┌──────────────────────────────────────┐
                    │         Cloud Server (1.5GB / 2C)     │
                    │                                        │
   Browser ─HTTP──→ │  Node.js (bridge.js)                  │
                    │   ├─ /          → web/ (static)       │
                    │   └─ WebSocket ↔ TCP :8080             │
                    │                    │                   │
                    │                    ▼                   │
                    │     ┌──────────────────────────────┐   │
                    │     │    C++ Server (mpserver)       │   │
                    │     │                                │   │
                    │     │   Main Reactor (epoll ET)      │   │
                    │     │    ├─ accept new connections   │   │
                    │     │    ├─ drain() kernel buffer     │   │
                    │     │    └─ try_pop() frame decode   │   │
                    │     │              │                  │   │
                    │     │      SPSC lock-free queue      │   │
                    │     │              │                  │   │
                    │     │   Worker Pool (N threads)       │   │
                    │     │    ├─ parse JSON               │   │
                    │     │    ├─ route (broadcast/private) │   │
                    │     │    └─ DB read/write             │   │
                    │     │              │                  │   │
                    │     │      MPMC mutex queue          │   │
                    │     │              │                  │   │
                    │     │   Main Reactor                  │   │
                    │     │    └─ send() to clients         │   │
                    │     │                                │   │
                    │     │   SQLite (messages.db)          │   │
                    │     └──────────────────────────────┘   │
                    └──────────────────────────────────────┘
```

- **Main Reactor** does only I/O (recv/send), never blocks — O(1) back to `epoll_wait`
- **Workers** do only business logic (parse, query, compose), never touch sockets
- **Queues** decouple the two layers — each is optimized independently

---

## File Structure

```
msg-push-platform/
├── ARCHITECTURE.md              ← This file
├── README.md                    ← Project homepage (last)
├── Makefile                     ← Build rules
├── .gitignore
│
├── src/
│   ├── main.cpp                 ← Entry: parse args, start server
│   ├── config.h                 ← Compile-time constants (port, threads, buffer)
│   ├── fd_guard.h               ← RAII fd wrapper
│   ├── object_pool.h            ← Template object pool (placement new)
│   ├── lockfree_queue.h         ← SPSC lock-free ring buffer
│   ├── thread_pool.h            ← Fixed-size thread pool
│   ├── protocol.h               ← Message framing (delimiter + JSON)
│   ├── message.h                ← variant<Login, Chat, Group, Private, Ack, Error>
│   ├── ring_buffer.h            ← Per-connection receive buffer + try_pop('\n')
│   ├── reactor.h                ← epoll ET event loop
│   ├── connection.h             ← Connection + ConnPtr (shared_ptr)
│   ├── broker.h                 ← Message dispatch: broadcast/private/group/ACK
│   └── db_store.h               ← SQLite persistence layer
│
├── bridge/
│   └── bridge.js                ← WebSocket ↔ TCP bridge (Node.js)
│
├── web/
│   ├── index.html               ← Chat frontend
│   ├── admin.html               ← Admin panel (online users, throughput)
│   └── style.css
│
├── scripts/
│   ├── stress_test.sh           ← Connection stress test
│   └── deploy.sh                ← Build + scp + docker restart
│
├── Dockerfile                   ← Cloud deployment
│
└── test/
    ├── test_queue.cpp           ← Lock-free queue correctness
    ├── test_pool.cpp            ← Object pool correctness
    ├── test_protocol.cpp        ← Protocol framing correctness
    └── test_db.cpp              ← DB layer correctness
```

**~950 lines C++ + 15 lines JS + frontend.**

---

## Tech Stack

### C++ Language Features

| Feature | Where It's Used |
|---------|-----------------|
| RAII | `FdGuard` fd lifecycle, `lock_guard` auto-release |
| Move semantics | Passing `Message` between threads — zero copy |
| Smart pointers | `shared_ptr<Connection>` on main thread, `weak_ptr` in workers for safe lifetime detection |
| Templates | `ObjectPool<T>`, `SPSCQueue<T>`, `DBStore<DBImpl>` |
| `variant` + `visit` | 6 message types dispatched at compile time |
| `string_view` | Zero-copy sub-string references during JSON parse |
| `unordered_map` / `unordered_set` | Connection table, username index, group member sets |
| `placement new` | Construct objects on pre-allocated pool memory |
| `atomic + memory_order` | SPSC lock-free queue |
| `condition_variable` | ThreadPool producer-consumer |

### Networking

| Component | Description |
|-----------|-------------|
| epoll (ET mode) | Main Reactor — edge-triggered + drain to completion |
| `\n` delimiter protocol | Application-layer message boundary, one JSON per line |
| Length-prefix (backup) | Binary-safe alternative when needed |

### Database

SQLite — same on local and cloud. A single `messages.db` file, zero deployment overhead, no daemon process, no extra memory. Clouds server's 1.5GB is plenty. Only `-lsqlite3` at link time — no runtime dependency.

### Frontend & Bridge

| Component | Language | Purpose |
|-----------|----------|---------|
| Chat client | HTML/CSS/JS | Browser WebSocket client |
| Admin panel | HTML/CSS/JS | Online count, throughput charts |
| bridge.js | Node.js (ws) | WebSocket ↔ TCP translation |

### Deployment

| Tool | Purpose |
|------|---------|
| Docker | One-command cloud deployment |
| Makefile | Local build |
| scp | Binary upload |
| bash | Stress test scripts |

---

## Key Design Decisions

### Why ET, not LT?
ET notifies once + `drain()` clears the buffer. LT could re-notify the same fd repeatedly, wasting `epoll_wait` cycles.

### Why SPSC lock-free queue, not `mutex + std::queue`?
Main → Worker is single-producer-single-consumer. No contention — `atomic` is enough, no mutual exclusion overhead.

### Why templates, not virtual functions for DB abstraction?
Compile-time polymorphism, zero vtable overhead. Like Linux kernel's `struct file_operations`. Swap SQLite for MySQL by changing one template parameter and recompiling.

### Why `shared_ptr<Connection>` + `weak_ptr`, not raw pointers?
A connection may be closed by the main thread (removed from `connections_`) while a Worker still holds a pending message. `weak_ptr` lets the Worker safely detect "is the connection still alive?" — `lock()` returns `nullptr` if it's gone.

### Why SQLite, not MySQL?
The cloud server has only 1.5GB RAM — MySQL won't fit. SQLite is zero-deploy, zero-memory-overhead, and keeps local and remote environments identical.

### Why Makefile, not CMake?
Single project, ~12 source files. Makefile is 30 lines and does the job.

---

## What's NOT Included

| Skipped | Why |
|---------|-----|
| Boost | C++17 stdlib covers everything |
| Protobuf | JSON is human-readable, better for demos |
| Redis | Overkill — SQLite handles offline messages directly |
| nginx | bridge.js serves static + WebSocket, saves 50MB RAM |
| CMake | Overkill for a 12-file project |
