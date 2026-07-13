# Contributing to OBS Multistreaming Kit

Thank you for contributing to `obs-ms-kit`! To maintain code quality and performance metrics, please adhere to these guidelines.

## Development Workflow

1. **Coding Style**: Run `clang-format` and `clang-tidy` before submitting a PR.
2. **Branching Model**: Make changes in separate feature branches. Target `main` or the release branch.
3. **Coding Rules**:
   - Prefer RAII and standard smart pointers (`std::unique_ptr`, `std::shared_ptr`).
   - Banned: Raw `new`/`delete` allocations in runtime paths.
   - Banned: Thread block calls on the graphics render threads.
   - Banned: Global mutable variables without atomic controls.
4. **Pull Requests**:
   - Ensure the build succeeds under MSVC and CMake checks.
   - Verify unit tests pass.
