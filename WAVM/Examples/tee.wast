;; poor man's tee
;; Outputs to stderr and stdout whatever comes in stdin

(module

  (import "env" "memory" (memory 1))
  (import "env" "_fwrite" (func $__fwrite (param i32 i32 i32 i32) (result i32)))
  (import "env" "_fread" (func $__fread (param i32 i32 i32 i32) (result i32)))
  (import "env" "_stdin" (global $stdinPtr i32))
  (import "env" "_stdout" (global $stdoutPtr i32))
  (import "env" "_stderr" (global $stderrPtr i32))
  (export "main" (func $main))

  (func $main
    (local $stdin i32)
    (local $stdout i32)
    (local $stderr i32)
    (local $nmemb i32)
    (local.set $stdin (i32.load align=4 (global.get $stdinPtr)))
    (local.set $stdout (i32.load align=4 (global.get $stdoutPtr)))
    (local.set $stderr (i32.load align=4 (global.get $stderrPtr)))

    (loop $loop
      (block $done
        (local.set $nmemb (call $__fread (i32.const 0) (i32.const 1) (i32.const 32) (local.get $stdin)))
        (br_if $done (i32.eq (i32.const 0) (local.get $nmemb)))
        (drop (call $__fwrite (i32.const 0) (i32.const 1) (local.get $nmemb) (local.get $stdout)))
        (drop (call $__fwrite (i32.const 0) (i32.const 1) (local.get $nmemb) (local.get $stderr)))
        (br $loop)
      )
    )
  )
)