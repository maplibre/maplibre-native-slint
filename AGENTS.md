# AGENTS.md

Read [README.md](README.md) first. It says what this repository is, which parts
are supported, which are experiments, and how to build and run them. Read
[AI_POLICY.md](AI_POLICY.md) before opening a pull request or writing in an
issue.

This file covers only what an agent needs on top of those two. Where the two
disagree with this file, they win.

## Operating rules

- Never run `sudo`. Anything needing elevated privileges is for the user to do.
- Run `clang-format` on C++ files you touch. `vendor`, `build` and `.git` are
  excluded:

  ```bash
  find . -name "*.cpp" -o -name "*.hpp" | grep -v "^./vendor/" | grep -v "^./build/" | grep -v "^./.git/" | xargs clang-format -i
  ```

- Write code comments, commit messages, branch names and pull request
  descriptions in English.
- Follow [the pull request template](.github/PULL_REQUEST_TEMPLATE.md),
  including its `AI assistance` section.

## Invariants

- A green `cargo test` is not evidence that anything rendered. The renderer
  tests in `experiments/rust/tests/` skip themselves unless both
  `MAPLIBRE_NATIVE_SLINT_RUN_RENDERER_TESTS=1` and a display are set, and CI
  sets neither.
- Passing renderer tests are not evidence either. They exercise MapLibre on its
  own, and some failures need the toolkit's renderer live in the same process.
  Only running the app reaches those. A graphics change is verified by running
  the app, not by a test summary.
- Say which platform and render backend you actually ran, and which parts you
  only reasoned about. Most of the hard problems here are where MapLibre
  Native, Slint and a graphics backend meet, and they do not show up in a diff.
- `experiments/` is not the supported path. Do not treat it as one, and do not
  make CI slower or heavier on its behalf.
- `experiments/rust/Cargo.lock` is tracked. Deleting it moves Slint as well as
  the crate you meant to move, which leaves two variables in any comparison.
  Use `cargo update -p <crate> --precise <version>` instead.
