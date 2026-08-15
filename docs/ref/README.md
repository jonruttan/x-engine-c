# Generated reference

This directory is the output root for generated API reference. Its contents
are build artifacts, ignored by git; the directory itself is tracked, because
the generators expect it to exist -- Doxygen fails outright without it:

    error: tag OUTPUT_DIRECTORY: Output directory 'docs/ref/c/' does not
           exist and cannot be created

| subdirectory | generator | source |
|---|---|---|
| `c/` | `make doc-c` (Doxygen, `OUTPUT_DIRECTORY = docs/ref/c/`) | the C engine under `src/` and `ext/` |
| `x/` | `make doc-x` (`tools/dev/doc.x`) | the `(doc ...)` forms in `lib/x/**` |

`make doc` runs both. Hand-written documentation lives one level up, in
`docs/`; nothing here should be edited, since the next generator run
overwrites it.
