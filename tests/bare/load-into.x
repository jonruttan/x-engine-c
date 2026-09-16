; Loaded by env.spec.md's "include with an environment binds the file's
; definitions there".  One definition, and a closure that reads it.
(def loaded-into 7)
(def loaded-reader (fn (_) loaded-into))
