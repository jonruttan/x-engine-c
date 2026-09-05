# State-image primitives

`(image save!)`, `(image rebuild!)` and `(image write!)` against the bare
engine: no library, every object made by a primitive, every word read back
at the offsets `tools/contract/obj-layout.x` declares.  Each case includes
`tests/bare/image-prelude.x`, which reaches those primitives through the
prims catalog and binds them under their bare names.  The contract is
x-lang's docs/state-image-format.md; the section numbers below are its.

Each case ends in `(error ...)` whose text is the verdict, as the smoke spec
does: the bare engine prints nothing on its own.

### save! saves an ordinary pair as two reference units (4.3)

```scheme
(include "tests/bare/image-prelude.x")
(def B (ptr-alloc 512))
(def n (image-save! (pair 1 2) B))
(error (match ((= n 2) (match ((= (w B 1) 0) (match ((= (w B 3) 0) "two refs") (#t "unit 1 kind"))) (#t "unit 0 kind"))) (#t "count")))
```
---
    *** ERROR: two refs

### save! saves a string as one bytes unit whose word is its text

```scheme
(include "tests/bare/image-prelude.x")
(def B (ptr-alloc 512))
(def n (image-save! "abc" B))
(error (match ((= n 1) (match ((= (w B 1) 2) (match ((= (ptr-strlen (int->ptr (w B 2))) 3) "bytes") (#t "text"))) (#t "kind"))) (#t "count")))
```
---
    *** ERROR: bytes

### save! saves an integer as one machine word

```scheme
(include "tests/bare/image-prelude.x")
(def B (ptr-alloc 512))
(def n (image-save! 42 B))
(error (match ((= n 1) (match ((= (w B 1) 1) (match ((= (w B 2) 42) "word") (#t "value"))) (#t "kind"))) (#t "count")))
```
---
    *** ERROR: word

### save! saves a primitive as two foreign units

```scheme
(include "tests/bare/image-prelude.x")
(def B (ptr-alloc 512))
(def n (image-save! first B))
(error (match ((= n 2) (match ((= (w B 1) 3) (match ((= (w B 3) 3) (match ((= (w B 2) 0) "no function") (#t "foreign"))) (#t "rest kind"))) (#t "first kind"))) (#t "count")))
```
---
    *** ERROR: foreign

### save! saves a procedure and an operative as a foreign call slot and a reference

```scheme
(include "tests/bare/image-prelude.x")
(def B (ptr-alloc 512))
(def np (image-save! (fn (_) 1) B))
(def kp (w B 1)) (def kq (w B 3))
(def no (image-save! (op (x) e x) B))
(error (match ((= np 2) (match ((= no 2) (match ((= kp 3) (match ((= kq 0) (match ((= (w B 1) 3) (match ((= (w B 3) 0) "call slot and state") (#t "operative state"))) (#t "operative call"))) (#t "procedure state"))) (#t "procedure call"))) (#t "operative count"))) (#t "procedure count")))
```
---
    *** ERROR: call slot and state

### save! saves a type handle -- its name atom, static-tagged and OWN -- as bytes (3.3)

```scheme
(include "tests/bare/image-prelude.x")
(def B (ptr-alloc 512))
(def h (make-type "IMGT" ()))
(def n (image-save! h B))
(error (match ((= n 1) (match ((= (w B 1) 2) (match ((= (ptr-strlen (int->ptr (w B 2))) 4) "handle is bytes") (#t "text"))) (#t "kind"))) (#t "count")))
```
---
    *** ERROR: handle is bytes

### save! saves a type-struct node structurally, as two references (3.3)

```scheme
(include "tests/bare/image-prelude.x")
(def B (ptr-alloc 512))
(def h (make-type "IMGT" ()))
(def st (%newest-struct))
(def n (image-save! st B))
(error (match ((= n 2) (match ((= (w B 1) 0) (match ((= (w B 3) 0) "struct node: two refs") (#t "unit 1"))) (#t "unit 0"))) (#t "count")))
```
---
    *** ERROR: struct node: two refs

### rebuild! patches references by index and by external, keeps the attribute flags and RO, sets SHARED (5)

Two records, written by hand: an untyped one-word object, and a struct node
whose first is that object and whose rest is external 1.  The record's flags
carry META too; that bit is the allocating base's policy, set or not as it
is on any fresh object here, never the record's.

```scheme
(include "tests/bare/image-prelude.x")
(def T (ptr-alloc 512)) (def BL (ptr-alloc 64)) (def IX (ptr-alloc 64))
(s T 0 -3) (s T 1 207) (s T 2 1) (s T 3 1) (s T 4 42)
(s T 5 -1) (s T 6 0) (s T 7 2) (s T 8 0) (s T 9 1) (s T 10 0) (s T 11 -1)
(def X (pair 7 8))
(image-rebuild! T 0 2 (pair () X) 2 BL IX)
(def o (fn (_ i) (ptr->obj (int->ptr (w IX i)))))
(def f (w (obj->ptr (o 1)) %obj-slot-flags))
(def flags-ok (match ((= (& f %obj-flag-attr-mask) %obj-flag-attr-mask) (match ((= (& f %obj-flag-ro) %obj-flag-ro) (match ((= (& f %obj-flag-meta) (& (w (obj->ptr (pair 1 2)) %obj-slot-flags) %obj-flag-meta)) (match ((= (& f %obj-flag-shared) %obj-flag-shared) "rebuilt") (#t "SHARED unset"))) (#t "META not this base's"))) (#t "RO lost"))) (#t "attrs lost")))
(error (match ((= (w (obj->ptr (o 1)) %obj-meta-len) 42) (match ((eq? (first (o 2)) (o 1)) (match ((eq? (rest (o 2)) X) flags-ok) (#t "external"))) (#t "reference"))) (#t "word")))
```
---
    *** ERROR: rebuilt

### rebuild! points a bytes unit into the blob, reads a foreign unit from the externals, and makes a reference past the table nil (5)

```scheme
(include "tests/bare/image-prelude.x")
(def T (ptr-alloc 512)) (def BL (ptr-alloc 64)) (def IX (ptr-alloc 64))
(s T 0 -3) (s T 1 0) (s T 2 1) (s T 3 2) (s T 4 0)
(s T 5 -3) (s T 6 0) (s T 7 1) (s T 8 3) (s T 9 1)
(s T 10 -3) (s T 11 0) (s T 12 1) (s T 13 0) (s T 14 -7)
(s BL 0 2) (ptr-set! BL 8 104 1) (ptr-set! BL 9 105 1) (ptr-set! BL 10 0 1)
(image-rebuild! T 0 3 (pair () 99) 2 BL IX)
(def o (fn (_ i) (ptr->obj (int->ptr (w IX i)))))
(error (match ((= (ptr-strlen (int->ptr (w (obj->ptr (o 1)) %obj-meta-len))) 2) (match ((= (w (obj->ptr (o 2)) %obj-meta-len) 99) (match ((eq? (first (o 3)) ()) "blob, foreign, sentinel") (#t "sentinel"))) (#t "foreign"))) (#t "bytes")))
```
---
    *** ERROR: blob, foreign, sentinel

### write! then rebuild! round-trips a flagged type struct, asking the callable for what it cannot place (4, 5)

```scheme
(include "tests/bare/image-prelude.x")
(def h (make-type "IMGT" ()))
(def st (%newest-struct))
(def flag! (fn (_ o) (s (obj->ptr o) %obj-slot-flags (| (w (obj->ptr o) %obj-slot-flags) 1024))))
(def spair? (fn (_ o) (= (w (obj->ptr o) %obj-slot-type) (w (obj->ptr st) %obj-slot-type))))
(def flag-tree! (fn (self o)
  (match ((eq? o ()) ())
         ((spair? o) ((fn (_ a b c) c) (flag! o) (self (first o)) (self (rest o))))
         (#t (flag! o)))))
(flag-tree! st)
(def cur (pair 1 2))
(def T (ptr-alloc 8000)) (def BL (ptr-alloc 800)) (def R (ptr-alloc 64))
(s R 0 1000) (s R 1 800)
(def N (image-write! cur 1024 T BL (fn (_ word kind obj) ()) (pair st ()) R))
(def IX (ptr-alloc (* %word-size (+ N 1))))
(image-rebuild! T 0 N (pair () ()) 1 BL IX)
(def root (ptr->obj (int->ptr (w IX (w R 4)))))
(def nm (first (first root)))
(error (match ((< 0 N) (match ((spair? root) (match ((= (ptr-strlen (int->ptr (w (obj->ptr nm) %obj-meta-len))) 4) (match ((< 0 (w R 3)) "round trip") (#t "no sentinel"))) (#t "name"))) (#t "root type"))) (#t "count")))
```
---
    *** ERROR: round trip

### write! refuses an object whose type struct is not in the image

```scheme
(include "tests/bare/image-prelude.x")
(def p (pair 1 2))
(s (obj->ptr p) %obj-slot-flags (| (w (obj->ptr p) %obj-slot-flags) 1024))
(def T (ptr-alloc 8000)) (def BL (ptr-alloc 800)) (def R (ptr-alloc 64))
(s R 0 1000) (s R 1 800)
(guard (e (error "raised")) (image-write! p 1024 T BL (fn (_ word kind obj) ()) () R))
```
---
    *** ERROR: raised

### write! refuses a table too small for a record

```scheme
(include "tests/bare/image-prelude.x")
(def h (make-type "IMGT" ()))
(s (obj->ptr h) %obj-slot-flags (| (w (obj->ptr h) %obj-slot-flags) 1024))
(def T (ptr-alloc 64)) (def BL (ptr-alloc 64)) (def R (ptr-alloc 64))
(s R 0 2) (s R 1 64)
(guard (e (error "raised")) (image-write! h 1024 T BL (fn (_ word kind obj) ()) () R))
```
---
    *** ERROR: raised
