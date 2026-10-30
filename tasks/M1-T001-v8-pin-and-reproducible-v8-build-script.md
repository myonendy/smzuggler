+++
id = "M1-T001"
title = "V8 pin and reproducible V8 build script"
milestone = 1
deps = []
agent = "any"
owns = ["tools/v8/**", "tests/tools/test_build_v8.py"]
docs = ["docs/build-v8.md", "docs/DECISIONS.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Propose the V8 version pin (§11 Q9) and write `tools/v8/build_v8.sh`, which fetches depot_tools and the pinned V8 into `$SMZ_DEPS_DIR/v8-src`, builds a static monolith with the §4.1 GN args (release, `v8_monolithic=true`, `use_custom_libcxx=false`, `v8_use_external_startup_data=false`, `is_component_build=false`, `v8_enable_i18n_support` per Q8, `v8_enable_pointer_compression` as decided) and installs headers, `libv8_monolith.a` and a `v8-config.cmake` manifest into `$SMZ_DEPS_DIR/v8/<version>/`. `--print-args` prints the GN args without fetching.

## Acceptance criteria
- [ ] `tests/tools/test_build_v8.py` runs `build_v8.sh --print-args` and asserts the pinned version, every required GN arg, and that the script never writes outside `$SMZ_DEPS_DIR` (checked by running it with a temp `SMZ_DEPS_DIR` in print mode and grepping the script for absolute paths).
- [ ] `docs/build-v8.md` explains the pin, the args (one line of why each), the disk/time cost, and how CI gets the tarball (§9 M1 "V8 CI tarball").
- [ ] DEC-M1-T001 records the proposed pin, why, and that the owner must approve it before M1-T002 merges.

## Notes
Spec §4.1, §9 M1, §11 Q8/Q9. Human checkpoint H1 (docs/FORGE.md): the owner approves the pin and runs the build once (hours) on dev-laptop; the card does not run the full build.
