+++
id = "M1-T005"
title = "jsb zero-copy buffers"
milestone = 1
deps = ["M1-T004"]
agent = "any"
owns = ["src/jsb/**", "tests/smz/test_jsb_buffers.c", "tests/smz/jsb_buffers.cmake"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add jsb buffer calls: `jsb_arg_bytes` (pointer and length of an ArrayBuffer/TypedArray/DataView argument without copying, valid until the call returns), `jsb_new_external_buffer` (native memory exposed to JS with a free callback) and external-memory accounting (`AdjustAmountOfExternalAllocatedMemory`) on V8. Both backends.

## Acceptance criteria
- [ ] Tests in `tests/smz/test_jsb_buffers.c`: views with offsets, a detached buffer throws, writes from C are visible in JS and back, the free callback runs exactly once after GC (forced with `--expose-gc` in the test isolate), in both modes.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1, §15.8.
