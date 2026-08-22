# Generated reference

This directory is the output root for the generated C API reference. Its
contents are build artifacts, ignored by git; the directory itself is tracked,
because the generator expects it to exist -- Doxygen fails outright without it:

    error: tag OUTPUT_DIRECTORY: Output directory 'docs/ref/c/' does not
           exist and cannot be created

| subdirectory | generator | source |
|---|---|---|
| `c/` | `make doc-c` (Doxygen, `OUTPUT_DIRECTORY = docs/ref/c/`) | the C engine under `src/`, `opt/` and `ext/` |

`c-main.md` beside this file is the reference's hand-written main page
(`USE_MDFILE_AS_MAINPAGE`); it is source, not output. Nothing under `c/` should
be edited, since the next generator run overwrites it.

The reference for the library written *in x-lang* is generated in the
[x-lang](https://github.com/jonruttan/x-lang) repository, from the `(doc ...)`
forms in its sources.
