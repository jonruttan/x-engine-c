; tools/base-layout.x — canonical slot layout of the x-eval base object.
;
; SINGLE SOURCE OF TRUTH for the base/evaluator pair-tree.  Consumed two ways:
;   1. tools/gen-base-layout.awk  ->  C (accessors + construction skeleton)
;   2. read as data by the X layer at runtime  ->  the slot descriptor
; Valid X (plain s-expressions); the awk parses the same bytes.  Both layers
; derive from this one file, so their views cannot drift.
;
; Tags (the contract):
;   (node NAME L R)   named interior pair  -> x_eval_NAME(X) anchor macro
;   (pair L R)        anonymous interior pair (structure only)
;   (cell NAME)       stack-cell leaf   -> field macro; pair(nil,nil) at build
;   (slot NAME)       direct-value leaf -> field macro; nil at build
;   (nil)             the empty list / list terminator
;   (build SUBTREE)   construction root: x-eval builds SUBTREE and assigns it
;                     at this position in x_eval_make
;   (todo NAME)       external (x-expr) or undescribed subtree — skipped
;
; x-eval fills five reserved slots in x-expr's spine.  Each top-level (node ...)
; is anchored at the x-expr extension macro the awk maps its name to:
;   base -> x_base   io-group -> x_base_field_io_group
;   profile -> x_base_field_profile   meta-group -> x_base_field_meta_group

; --- base.first: env + ctrl ---
(node base
  (build
    (pair
      (node env
        (cell env-alist)
        (pair (slot env-local-boundary)
              (pair (slot env-global-tree)
                    (slot shadow-list))))
      (node ctrl
        (pair (slot save-stack)
              (cell error-handler))
        (pair (cell tco-expr)
              (cell tco-env)))))
  (todo io-meta))

; --- io group: type-alist cell + io-state ---
(node io-group
  (pair
    (build (cell type-alist))
    (todo files))
  (build
    (node io-state
      (cell line)
      (pair (cell true)
            (cell false)))))

; --- profile: x-eval's 9 counters appended after x-expr's allocs ---
(node profile
  (todo allocs)
  (build
    (pair (cell profile-evals)
      (pair (cell profile-tco)
        (pair (cell profile-assoc-calls)
          (pair (cell profile-assoc-steps)
            (pair (cell profile-sym-find-calls)
              (pair (cell profile-sym-find-steps)
                (pair (cell profile-gc-runs)
                  (pair (cell profile-bst-hits)
                    (pair (cell profile-bst-misses)
                          (nil))))))))))))

; --- meta group: x-expr's alloc group, then the state group (was 'extras') ---
(node meta-group
  (todo profile-hooks)
  (pair
    (todo heap)
    (pair
      (todo alloc)
      (build
        (node state
          (cell eval-list)
          (pair (cell token-cache)
            (pair (cell sigint)
              (pair (cell error-str)
                    (cell prims)))))))))
