# Agent Operating Guidelines

This document outlines the core operational principles and constraints for the AI agent interacting with this repository.

## Security Constraints

### Sudo Prohibition

Under no circumstances is the agent permitted to execute commands using `sudo`. Any operation requiring elevated privileges must be performed manually by the user. This is a critical safety protocol to prevent unintended system modifications.

## Code Formatting

### C++

When making changes to C++ files (`.cpp`, `.hpp`), it is mandatory to run `clang-format` to ensure code style consistency.

**Important:** Exclude the following directories from formatting: `vendor/`, `build/`, `build-ninja/`, and `.git/`.

You can format all relevant files using the following command:

```bash
find . \( -name "*.cpp" -o -name "*.hpp" \) \
  | grep -v -E "^\./(vendor|build|build-ninja|\.git)/" \
  | xargs clang-format -i
```

Alternatively, run the helper script:

```bash
./scripts/format-cpp.sh
```

## Build and Run (macOS)

The following are the trial-and-error steps used on macOS:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_METAL=ON -DMLN_WITH_OPENGL=OFF -G Xcode .
cmake --build build
./build/Debug/maplibre-slint-example
```

## Contribution Guidelines

- Comments in code: Must be written in English.
- Commit messages: Must be written in English.
- Pull Request description: Must be written in English.
- PR template: Use `.github/PULL_REQUEST_TEMPLATE.md` for all pull requests.
- Branch names: Must be written in English.

## Creating a Pull Request

When the user requests to "create a pull request for the current work", follow these steps:

- Verify repository state: Run `git status` and `git branch` to review changes and current branch.
- Create a new branch: Use a clear English name (e.g., `git checkout -b feature/update-agents-doc`).
- Commit changes: Use a clear, English commit message (e.g., `git add -A && git commit -m "docs: update AGENTS.md with macOS steps and contribution rules"`).
- Push the branch: `git push -u origin <branch-name>`.
- Create the PR:
  - If GitHub CLI (`gh`) is available: `gh pr create --base main --head <branch-name> --title "<English title>" --body-file .github/PULL_REQUEST_TEMPLATE.md` and fill the template sections appropriately.
  - Otherwise: open a PR via GitHub UI and ensure the description follows `.github/PULL_REQUEST_TEMPLATE.md` in English.
