# Agent Operating Guidelines

This document outlines the core operational principles and constraints for the AI agent interacting with this repository.

## Security Constraints

### Sudo Prohibition

Under no circumstances is the agent permitted to execute commands using `sudo`. Any operation requiring elevated privileges must be performed manually by the user. This is a critical safety protocol to prevent unintended system modifications.

## Code Formatting

### C++

When making changes to C++ files (`.cpp`, `.hpp`), it is mandatory to run `clang-format` to ensure code style consistency.

**Important:** The `vendor`, `build`, and `.git` directories must be excluded from formatting.

You can format all relevant files using the following command:

```bash
find . -name "*.cpp" -o -name "*.hpp" | grep -v "^./vendor/" | grep -v "^./build/" | grep -v "^./.git/" | xargs clang-format -i
```