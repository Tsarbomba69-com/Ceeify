# C Coding Conventions & Architecture Guidelines

**Scope:** C (C99 baseline; see §1.1) — applies to application code, libraries, and tooling written in-house.  
**Status:** Living document — changes go through the same review process as code.

---

## 1. Foundation

This document is opinionated where the language is silent, and silent where good C style is already well established (K&R spacing debates, brace placement, etc. — pick one, put it in `.clang-format`, stop arguing about it). It draws on two sources of practice:

- Established C idiom (C99-era stdlib conventions, `stdint.h`, POSIX naming where relevant)
- The "single-person module," black-box, and control-flow philosophy popularized by Eskil Steenberg and echoed by Andrew Kelley's data-oriented design work — both converge independently on: small modules, explicit fault handling, indices over pointers, and wrapped dependencies

Where this document contradicts a popular external style guide, this document wins for the case it explicitly covers.

### 1.1 Language Version

- **Baseline: C99.** This is the recommended default for new projects — it fixes the worst ergonomic problems of C89 (inline declarations, designated initializers, fixed-width integer types) while remaining close to universally supported.
- **C89** is acceptable only for code that must compile on genuinely legacy toolchains (embedded targets with old vendor compilers) or that explicitly wants C++-compatibility as a hard constraint. This is the exception, not the default — don't reach for it out of habit.
- **C11/C23 features** (`_Generic`, `_Static_assert`, anonymous structs/unions, etc.) may be used per-project if the whole toolchain supports them, but must be a deliberate, documented project decision, not an incidental dependency introduced by one file. Don't mix standards within a single translation unit graph.
- Whatever version is chosen, state it explicitly in the build (`-std=c99`, `-std=c11`, …) — never rely on compiler default.

---

## 2. Versioning — SemVer

All published libraries/services follow [Semantic Versioning 2.0.0](https://semver.org/): `MAJOR.MINOR.PATCH`.

- **MAJOR** — breaking change to a public contract: any change to a `.h` file's exported function signature, struct layout of a struct callers are allowed to allocate/inspect directly, enum value renumbering, or removal of a public symbol.
- **MINOR** — backward-compatible functionality added (new public function, new optional struct field appended at the end, new enum value appended at the end).
- **PATCH** — backward-compatible bug fix, no public contract change.
- Pre-1.0 (`0.x.y`) is allowed only for genuinely unstable internal tooling, never for a library another module or another team links against.
- Breaking changes must be called out explicitly in the changelog entry, not buried in a `fix` or `refactor` commit.
- Because C has no access modifiers, the "public contract" is defined by what's declared in a header meant for external `#include`, not by convention alone — see §6.1.

---

## 3. Commit Convention

```bash
<type>(<scope>): <short summary>
  │       │             │
  │       │             └─⫸ Present tense, not capitalized, no trailing period.
  │       │
  │       └─⫸ Scope: the module touched, e.g.
  │                  arena | strings | render | net | platform |
  │                  build | tests | tools
  │
  └─⫸ Type: build | ci | docs | feat | fix | perf | refactor | test
```

| Type | Description |
| --- | --- |
| `build` | Build system or dependency changes |
| `ci` | CI configuration/scripts |
| `docs` | Documentation only |
| `feat` | New feature |
| `fix` | Bug fix |
| `perf` | Performance improvement, no behavior change |
| `refactor` | Neither fixes a bug nor adds a feature |
| `test` | Adding/correcting tests |

- Imperative present tense: `add arena reset` not `added` or `adds`.
- No capitalization, no trailing period on the summary line.
- Body (optional, but **required** for anything MAJOR per §2): explain *why*, not *what*.
- Footer: `BREAKING CHANGE: ...` when applicable, and issue references (`Refs #123`).
- Keep the scope list in `CONTRIBUTING.md`, in sync with the actual module directories — don't let it drift.

---

## 4. Naming

### 4.1 Case Convention — Types vs. Everything Else

C has no IDE-guaranteed type/value disambiguation the way languages with rich tooling do, so casing carries that load instead:

```c
typedef struct Chapter Chapter;      // Types: PascalCase
typedef struct ArenaAllocator ArenaAllocator;

int  chapter_load(Chapter *out, int id);   // Functions, variables, fields: snake_case
bool chapter_is_published(const Chapter *c);
```

Seeing `Chapter` versus `chapter` should be immediately, unambiguously readable — capitalization alone tells you "this is a type" or "this is a value/action," without needing to jump to a declaration. Don't mix conventions within a project; pick this one and apply it uniformly, including to typedef'd primitives (`typedef uint32_t ChapterId;`).

### 4.2 Public API surface — verb-first, raylib-style

For **public, callable operations** — any function declared in a header meant for external `#include` — name using:

```c
<verb>_<subject>_<complement>(...)
<action>_<object>_<attribute/state>(...)
```

Examples:

```c
chapter_create_invite(...)
chapter_revoke_access(...)
chapter_mark_published(...)
merge_resolve_conflict(...)
```

This exists to keep function names scannable-by-verb in a header, a `grep`, or an IDE's symbol list, and to force every public operation to state its effect, not just its subject. The subject usually doubles as a namespace prefix (`chapter_`, `arena_`, `string_`) since C has no real namespacing — see §4.4.

**Explicitly exempt** from verb-first ordering (idiomatic C forms apply instead):

- Struct/type declarations (§4.3) — nouns, not actions.
- Standard predicate/factory patterns C developers expect: `x_try_get(...)`, `x_create(...)`, `x_from(...)`, `x_is_...(...)`, `x_has_...(...)` — these are themselves verb-first in spirit, so don't force them into an unnatural order.
- Accessor-style one-liners on a struct (`chapter_length(c)` reads fine without forcing `chapter_get_length`).

### 4.3 Structs / value types — one word, two at most

Struct type names are one word; two only when one word is genuinely ambiguous without it. This is a forcing function against over-modeling — if you need three or more words to name a struct, it's very likely two concepts glued together and should be split.

```c
// Preferred
typedef struct ChapterId ChapterId;
typedef struct Money Money;
typedef struct Percentage Percentage;

// Acceptable (two words, second disambiguates a unit/kind)
typedef struct ChapterSlug ChapterSlug;
typedef struct UtcTimestamp UtcTimestamp;

// Avoid — split it
typedef struct ChapterCollaboratorInviteToken ChapterCollaboratorInviteToken;
// → CollaboratorInvite + InviteToken
```

### 4.4 Namespacing by Prefix

C has no namespaces or modules, so the verb-first prefix in §4.2 *is* your namespacing mechanism. Every symbol exported from a module's header carries that module's prefix consistently:

```c
// arena.h
void *arena_alloc(ArenaAllocator *a, size_t size);
void  arena_reset(ArenaAllocator *a);
void  arena_destroy(ArenaAllocator *a);
```

Never export a bare, unprefixed symbol (`create(...)`, `destroy(...)`) from a header meant for external use — it *will* collide once the project has more than a handful of modules linked together. `static`/file-local helpers are exempt (§6.1).

---

## 5. Function Signatures — Dimensionality

> "Simplify function signatures to minimize branches at the call site, which are viral through the call graph."

- **≤ 5 parameters**, hard default. Above that, introduce a parameter struct (per §4.3's naming rules) — this is not just a style preference, it's what makes the "no more than 5" rule survive refactors instead of being worked around with a growing argument list.
- Prefer fewer, more expressive types over more, primitive ones. A signature with three `bool` flags has 8 silent behavioral branches at every call site — replace with a single `enum` or a bitflag type with named constants (§11).
- **Return-type hierarchy, in order of preference** (C has no exceptions, so this hierarchy is about the return value *and* how failure is signaled, together):
  1. `bool` (or a two-state result) — the caller has exactly one branch. Reserve this for operations where failure is a single, undifferentiated case.
  2. A bounded value type (an `int` with a known range, a custom struct, an `enum` result code) — finite, enumerable branches. This is the C idiom for "operation with a small number of named failure modes": return an enum, write the success value through an `out` pointer parameter.
  3. An unbounded/open type (`char *`, `void *`, or a value that can be null/sentinel with no further structure) — only when the first two genuinely cannot express the result. Do not make this the default fallback; in C it's tempting because it's familiar (`NULL` on failure, `errno` for detail), but `errno`-style out-of-band state is exactly the "ambient state" §5's control-flow rule below warns against — prefer an explicit out-parameter or enum result over `errno`.
- Push control flow *up* (callers decide branching) and data flow *down* (callees receive what they need, don't reach out for it). Concretely: a function shouldn't read a global, a `static` mutable, or `errno` to decide its own branching — that state should arrive as a parameter, and the branching decision should live as close to the entry point (`main`, a top-level event handler) as reasonable.
- Declare variables as close as possible to first use (C99 permits this — see §1.1). A variable declared 40 lines above its use is a standing invitation for someone to insert logic between declaration and use that invalidates the assumption it was declared under.
- Pass `const T *` for read-only parameters rather than `T` by value for anything larger than a machine word — this is both a performance default and a documentation signal ("this function does not own or mutate what you passed it").

---

## 6. Interfaces & Boundaries

C has no `public`/`internal` keywords, so "surface area" is controlled entirely by two mechanisms: what you put in a header, and `static`. Treat both as load-bearing, not cosmetic.

### 6.1 Minimize Surface Area

- **`static` by default.** Any function or file-scope variable not meant to be called/read from another translation unit is `static`. Promote something out of `static` only when a real external caller needs it today — not "might need it."
- **Headers are the contract.** A `.h` file is a promise you now have to keep under SemVer (§2). Don't put a function prototype in a shared header just because it's convenient to call from two files in the same module — if both callers are in the same module, a module-internal header (`_internal.h`, not installed/shipped) is the right place.
- **Black-box modules (single-person rule).** Every module should be small enough that one person can hold its entire implementation in their head, and its consumers should never need to read its `.c` file to use it correctly — the header plus its doc comments (§7) should be sufficient. If you find yourself explaining "well, internally it does X, so you need to call it in Y order," that ordering constraint belongs in the header's documented contract, not in tribal knowledge.
- **Don't leak representation.** If a struct's internal layout may change, expose it as an opaque pointer (`typedef struct Chapter Chapter;` with the `struct Chapter { ... }` definition only in the `.c` file) plus accessor functions, rather than a fully-defined struct callers can reach into. This is the C equivalent of "don't leak private state" and is what makes MAJOR-vs-MINOR (§2) actually enforceable for struct changes.

### 6.2 Define Fault Models

Every function whose operation can fail — DB, HTTP, file I/O, external SDK call, allocation — must have an explicit, documented answer to: *what happens on timeout, on partial failure, on malformed input, on allocation failure?*

In C this answer must be concrete about mechanism, because there's no exception to fall back on:

- State whether the function returns an enum/result code, writes an out-parameter, returns a sentinel (`NULL`, `-1`), or some combination — and document it in the header doc comment (§7), not just in the `.c` file.
- "It aborts the process" is a valid answer for unrecoverable states (out-of-memory in a context with no graceful degradation path) — but it must be the *stated* answer, not discovered at 3am.
- Never silently swallow a failure to make a signature simpler. If a bounded-value-type result code doesn't have room for a case you're hitting, that's a signal the enum needs another value, not a signal to return success.

### 6.3 Abstract Non-Deterministic Interfaces

Wall-clock time, random values, network calls, file system state — none of these are called directly from business/domain logic. Wrap each behind a small interface (a struct of function pointers, or a set of `clock_*`/`rand_*`-prefixed wrapper functions per §4.4) so domain logic is replayable and testable with fixed inputs. This is also what makes §9's property-based testing viable at all, and what enables the "data recorder + replay" debugging technique in §13.

### 6.4 Push Control Flow Up, Data Flow Down

High-level orchestration (`main`, a top-level request handler, an event loop) manages control flow, branching, and lifetime/ownership boundaries. Low-level module code receives the data it needs as parameters, executes deterministic logic, and returns computed results without reaching for ambient/global state or performing hidden side effects. See §5's control-flow bullet for the mechanical rule this implies for function signatures.

### 6.5 The Core/Platform Split

Never call platform-specific code (OS APIs, a specific windowing library, a specific renderer, a specific third-party SDK) directly from core application/domain logic. Create a wrapper API that *you* own, matching §4's naming and §6.2's fault-model rules like any other internal API — it does not just mirror the underlying platform call's own shape. If the platform changes, or you port to a new OS, you rewrite the wrapper's implementation; call sites in core logic don't change. This is the same discipline as §8's third-party library wrapping, applied to the OS/platform layer specifically.

- **Headless-first:** design core logic so it can run and be tested with zero UI attached — this is a direct consequence of the core/platform split, and it's what makes automated testing (§9) and CLI/server execution possible at all.

---

## 7. Documentation

- Every function declared in a header meant for external use **must** have a doc comment (Doxygen-style `/** ... */`) directly above its prototype — enforced by running Doxygen (or equivalent) in CI with warnings-as-errors for undocumented public symbols.
- Doc comments describe **contract**, not implementation: preconditions, postconditions, ownership/lifetime of any pointer parameter or return value, and the fault-model answer from §6.2 where relevant. Not "loops through the list" — "returns `NULL` if `chapter_id` is not found; caller does not own the returned pointer."
- **Ownership must be explicit for every pointer in a public signature** — this is C-specific and non-optional: does the callee take ownership of a passed-in pointer? Does the caller own the returned pointer and need to free it, and with which function? A doc comment that's silent on ownership is an incomplete doc comment.
- `static`/file-local functions: comment *why*, only when the *why* isn't obvious from the code — don't restate the signature in prose.

---

## 8. Third-Party Libraries — Wrap Core Dependencies

If a library implements a **core feature** of the system (i.e., replacing it would require a real migration, not a config change) — a compression library, a windowing/rendering library, a networking stack, a parser generator's runtime — it is accessed **only** through an internal wrapper API owned by this codebase, never called directly from business/domain code.

Rules for the wrapper:

- The wrapper's public shape follows §4/§5/§7 like any other internal API — it does **not** just mirror the third-party library's own naming/shape.
- The wrapper is the single seam for the fault model (§6.2) of that dependency — retries, timeouts, translating the library's error codes into this codebase's own result types all happen here, once.
- Swapping the underlying library should touch only the wrapper's implementation file, never its call sites or its header.
- Trivial/leaf utility libraries (e.g. a small string-formatting header-only library) don't need this — use judgment; the bar is "core feature, hard to replace," not "any dependency."
- This applies equally to the platform/OS layer — see §6.5.

---

## 9. Testing — Property-Based

Unit tests for domain/business logic define **invariants** (properties that must hold for *all* valid inputs) and generate randomized inputs to check them — not solely example-based (`given X, expect Y`) tests.

- Use a property-based testing approach alongside your example-based test framework — e.g. a lightweight C property-testing harness, or a fuzzing-driven approach (libFuzzer, AFL) for boundary-heavy code, since C's testing ecosystem doesn't have a single dominant FsCheck-equivalent.
- Every property test states its invariant explicitly in the test name or a comment above it — e.g. *"merging two non-conflicting chapter edits is commutative"*, *"`string_slice` never reads outside the original buffer's bounds regardless of start/length input."*
- Example-based tests aren't discarded — keep them for known edge cases and regressions, but every regression that came from a real bug should also prompt asking: *is there a missing invariant this bug reveals?* If so, add the property test, not just the regression test.
- **Selective, not exhaustive.** Don't chase test-coverage metrics for their own sake. Test things that are actually hard to verify just by reading/running the code once. Spend proportionally more testing effort on code you don't control (third-party wrapper boundaries, §8) than on trivial functions you wrote yourself and can visually verify.
- Pure functions and value types (§4.3 structs) are the easiest and highest-value targets — start there before reaching for property tests around I/O-boundary code (which should be thin anyway, per §6/§8).
- Test names follow Given/When/Then for clarity: `test_given_expired_token_when_validate_called_then_returns_invalid` — this survives translation from the .NET convention fine since it's just a naming discipline, not a language feature.

---

## 10. Compiler Discipline

> "Code, like steel, is easier to change while it's hot."

- All code compiles clean from day one — not retrofitted later. Set this up before the first feature commit, not after the first bug.
- Baseline flags for every build:

```bash
clang -std=c99 -Wall -Wextra -pedantic main.c
```

- **Curate your warnings-as-errors list rather than blanket `-Werror`.** Blanket-promoting every warning to an error is tempting but has a real cost: it makes upgrading your compiler painful (a new compiler version's new warnings now break your build unexpectedly) and discourages compiler authors' own use case of "warn, don't yet block, on something borderline." Prefer an explicit, reviewed list of warnings you've decided you care about, promoted individually:

```bash
clang -std=c99 -Wall -Wextra \
  -Werror=implicit-function-declaration \
  -Werror=incompatible-pointer-types \
  -Werror=return-type \
  -Werror=uninitialized \
  main.c
```

  Revisit and extend this list deliberately during code review, not by flipping a global switch.

- **Address Sanitizer on by default in development.** `-fsanitize=address` (plus `-fsanitize=undefined` where supported) should be part of the default local/dev/CI build, not an opt-in someone has to remember. It catches memory corruption — writes that don't segfault but silently corrupt adjacent memory — that a debugger alone won't show you. Turn it off only for release/perf builds; it has real runtime overhead.
- No suppressed warning ships without a comment justifying it and linking a ticket — a bare, unexplained suppression is a rejected PR.
- No magic values (§11) is caught partly by tooling (`-Wswitch-enum` for un-handled enum cases in a `switch`) and partly in review.

---

## 11. No Magic Values

- No bare literals (numbers, strings) in business logic that carry meaning beyond their literal self — timeout counts, buffer sizes, status/error codes used for branching, etc. Name them: `enum`, `#define`, or `static const`.
- This includes **magic strings used as keys or identifiers** — config keys, protocol field names, hash map keys. Centralize these as named constants in one header per subsystem so a typo is a compile error (or at minimum a single-point-of-truth), not a silent runtime mismatch.
- Prefer `enum` over `#define` for a related family of named integer constants — it gives you a type, and `-Wswitch-enum` can then catch an unhandled case at compile time. Reserve `#define` for values that genuinely aren't part of an enumerable family (buffer sizes, magic-number file signatures).
- Exception: universally self-evident literals in non-domain contexts (`0`/`1` in a loop increment, `2` in `x / 2` for a midpoint) don't need naming — the goal is eliminating *unexplained* significance, not eliminating all literals.

---

## 12. Memory: Indices, Poison Values, and Arenas

C's lack of automatic memory management is a feature you actively work with, not a problem to code around. Three concrete patterns, in order of how often they apply:

### 12.1 Indices Instead of Raw Pointers

When storing relationships between objects held in a resizable array, store an index, not a pointer:

```c
typedef struct User {
    int friend_indexes[10];   // not: User *friends[10];
} User;
```

Benefits, in rough order of importance:

1. **Survives resizing.** A `realloc`'d array invalidates every pointer into it; an index remains valid.
2. **Enables bounds checking.** Dereferencing a stored index must go through the owning array's accessor, so out-of-range access is checkable at that one seam (see §12.2 for what to do when it's out of range).
3. **Memory savings.** A 32-bit index is half the size of a 64-bit pointer on most current targets.
4. **Serialization-safe.** Indices remain valid after save/load; pointers generally do not (ASLR, differing load addresses).
5. Prefer `uint32_t` (not `uint64_t`) for indices specifically because it makes an out-of-bounds/corrupted index produce a large, obviously-wrong jump when misused as an offset — a corrupted 32-bit index used against 64-bit-addressed memory tends to crash loudly rather than quietly land in adjacent valid memory. This is a deliberate trade: a smaller index type is *more* likely to fail loudly, which is what you want in development.

- For deletion/reuse of slots, consider generational indices (an index plus a generation counter, incremented on slot reuse) so a stale index from before a deletion is detected rather than silently pointing at whatever now occupies that slot.

### 12.2 Bounds-Checked Array Access

Don't rely on raw pointer arithmetic for anything that isn't a hot-loop leaf. Wrap arrays that need runtime safety:

```c
typedef struct IntArray {
    int *data;
    int length;
    int capacity;
} IntArray;

int intarray_get(const IntArray *array, int index) {
    if (index < 0 || index >= array->length) {
        // deliberate assert/log point — see §13's "no-op breakpoint" technique
        return 0;
    }
    return array->data[index];
}
```

A well-optimizing compiler eliminates redundant bounds checks inside tight loops where the range is already proven, so this is not automatically a performance tax — measure before assuming otherwise.

### 12.3 Poison Values, Not Zero-Initialization

Don't reflexively zero-initialize memory as a safety net. Zero is frequently a *valid* domain value (a valid index, a valid enum member, a valid count), so zero-initializing hides the class of bug where something is read before being properly set — the program keeps running with a plausible-looking wrong value instead of crashing where the bug actually is.

- In debug builds, initialize freshly allocated memory with a recognizable non-zero poison pattern (e.g. `0xCD` repeated) instead of relying on the allocator's zeroing (or lack thereof). An uninitialized value used as a pointer or index then stands out immediately in a debugger and is far more likely to crash immediately rather than silently corrupt state later.
- This is a development-time aid; it does not replace actually initializing every field correctly before use.

### 12.4 Arena Allocators

Rather than tracking every individual `malloc`/`free` pair, group allocations by **lifetime**, not by type. Most data falls into one of three lifetimes:

1. **Static** — lives for the whole program (global config, core tables).
2. **Function/stack scope** — lives for one function call.
3. **Task scope** — lives for one bounded unit of work (a request, a file import, a single frame, a document-editing session).

For task-scoped data, allocate one large block (an arena) up front, hand out sub-allocations from it during the task, and free the whole arena in a single call when the task ends — rather than freeing each sub-allocation individually. This is both a large performance win (one free instead of thousands) and a correctness win (a whole class of use-after-free and leak bugs becomes structurally impossible, because nothing is freed piecemeal).

---

## 13. Debugging Discipline

Debugging is a core engineering skill, not an afterthought, and this codebase treats it as something to design *for*, not just cope with when things break.

- **Prioritize the class of bug, not just the instance.** A goal of good code structure is to move bugs from "silent memory corruption" (hardest) → "runtime crash" (medium) → "compile error" (easiest). §12.3's poison values and §12.1's small-index-type choice are both deliberate applications of this: make failure loud and immediate rather than quiet and deferred.
- **Use a real debugger, not print statements, as the default tool.** Print debugging in C is slow to iterate on and often fails to capture the exact moment of corruption. A debugger that breaks at the exact faulting line with a full call stack and live variable inspection is the expected default workflow, with print/log instrumentation as a supplement for cases a debugger genuinely can't reach (timing-sensitive races, distributed systems).
- **Build an investigation kit per module** — print/dump functions, and where relevant a way to visualize the module's data — as part of the module itself, not improvised each time something breaks. Treat these as part of the module's maintenance surface, not throwaway scaffolding.
- **Validation "pepper."** For a hard-to-isolate bug, insert cheap validation/assertion calls before and after suspect operations to narrow down exactly where a data structure becomes invalid, then remove or gate them behind a debug flag once resolved.
- **The no-op breakpoint.** A line like `x += 0;` inside a specific conditional gives you a stable, searchable, source-portable place to set a conditional breakpoint, without needing an actual code change to add debug logic later.
- **Independent debug-mode toggle.** Your own application-level debug tooling (poison values, validation pepper, extra assertions) should be togglable independently of the compiler's optimization level, so you can reproduce "release-build-only" bugs with debug aids still active.
- **Data recorders for hard-to-reproduce bugs.** For state-machine-heavy or event-driven code, record the input/event sequence, so a failure can be replayed exactly rather than chased live. This depends on §6.3's non-determinism abstraction — you can't replay something that reads the wall clock or `rand()` directly.

---

## 14. Folder Organization: Vertical Slicing

Traditional layered architectures organize directories horizontally by technical role (/controllers, /models, /services, /views). This codebase mandates Vertical Slicing (feature-first organization), grouping code by distinct business capabilities or self-contained modules.

### 14.1 Directory Layout & Structure

Code is structured so that a developer working on a single domain feature works almost exclusively within that feature's directory. Shared infrastructure or cross-cutting primitives live in explicitly distinct shared folders.

```bash
src/
├── core/                   # System-wide infrastructure & shared primitives
│   ├── arena.h / arena.c
│   ├── string_utils.h / string_utils.c
│   └── platform/           # OS/Platform wrappers (§6.5)
│       ├── platform_clock.h
│       └── platform_win32.c
│
├── features/               # Vertical slices (domain features)
│   ├── application/        # Feature slice: Application domain
│   │   ├── application.h           # Public API contract (§6.1)
│   │   ├── application_internal.h  # Shared across feature files, hidden from outside
│   │   ├── application_workflow.c  # State machine / main logic
│   │   ├── application_db.c        # Feature-specific persistence
│   │   └── application_test.c      # Feature unit/property tests (§9)
│   │
│   └── compliance/         # Feature slice: Compliance domain
│       ├── compliance.h
│       ├── compliance_check.c
│       └── compliance_test.c
│
└── main.c                  # Top-level orchestration & main loop (§6.4)
```

### 14.2 Rules for Vertical Slices

Feature Cohesion: Everything required to execute a business vertical (data structures, logic, persistence calls, and feature-specific tests) belongs inside that feature’s dedicated folder.

Cross-Slice Dependencies: Vertical slices must not directly depend on or #include internal implementation headers (_internal.h) of other slices. Communication between slices must go strictly through public feature headers (feature.h) or top-level event loops (§6.4).

Core vs. Feature: Code placed in core/ must be completely domain-agnostic. If a utility function references business logic, domain structs, or specific feature states, it belongs in a vertical feature directory, not in core/.

Feature Deletion: A well-architected vertical slice should be safely removable by deleting its folder and unhooking its top-level calls in main.c, leaving no dangling code in horizontal global directories.

## 15. Quick Reference Checklist

- [ ] Public surface documented (§7) with ownership stated for every pointer
- [ ] SemVer bump matches the actual change (§2)
- [ ] Commit message matches `<type>(<scope>): <summary>` (§3)
- [ ] New structs: 1–2 word PascalCase names (§4.3); functions snake_case with module prefix (§4.1, §4.4)
- [ ] New public operations: verb-first naming (§4.2)
- [ ] Functions: ≤5 params, bool > bounded enum/out-param > open type as return (§5)
- [ ] No `errno`-style ambient state driving control flow (§5, §6.4)
- [ ] New core-feature or platform dependency: wrapped (§8, §6.5)
- [ ] No unnamed magic literals (§11)
- [ ] Relationships between array elements: indices, not raw pointers (§12.1)
- [ ] Task-scoped allocations: arena, not individual malloc/free (§12.4)
- [ ] Debug builds: poison-initialized, ASan on (§10, §12.3)
- [ ] Domain logic: invariants identified, property tests added (§9)
- [ ] Fault model stated for any I/O boundary touched (§6.2)
- [ ] Folder structure follows feature vertical slicing; no feature logic in `core/` (§14)
