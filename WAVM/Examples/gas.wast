(module
  (type (;0;) (func (result i32)))
  (type (;1;) (func (param i32 i32)))
  (type (;2;) (func (param i32) (result i32)))
  (type (;3;) (func))
  (type (;4;) (func (param i64) (result i64)))
  (import "env" "STACKTOP" (global (;0;) i32))
  (import "env" "memory" (memory (;0;) 256 256))
  (import "env" "_add_gas" (func (;0;) (type 1)))
  (func (;1;) (type 0) (result i32)
    i64.const 10
    call 2
    i32.wrap/i64
    i32.const 10
    i32.add)
  (func (;2;) (type 4) (param i64) (result i64)
    (local i32 i32 i64)
    get_local 0
    i64.const 10
    i64.add
    set_local 3
    i64.const 10
    set_local 0
    loop  ;; label = @1
      get_local 2
      i32.const 100
      i32.ne
      if  ;; label = @2
        i32.const 0
        set_local 1
        loop  ;; label = @3
          get_local 3
          get_local 1
          i64.extend_u/i32
          i64.gt_u
          if  ;; label = @4
            get_local 0
            get_local 1
            get_local 2
            i32.add
            i64.extend_u/i32
            i64.add
            set_local 0
            get_local 1
            i32.const 1
            i32.add
            set_local 1
            br 1 (;@3;)
          end
        end
        get_local 2
        i32.const 1
        i32.add
        set_local 2
        br 1 (;@1;)
      end
    end
    get_local 0
    i64.const -10
    i64.add)
  (func (;3;) (type 3)
    i32.const 0
    i32.const 0
    call 0)
  (func (;4;) (type 2) (param i32) (result i32)
    (local i32)
    get_global 1
    set_local 1
    get_global 1
    get_local 0
    i32.add
    set_global 1
    get_global 1
    i32.const 15
    i32.add
    i32.const -16
    i32.and
    set_global 1
    get_local 1)
  (global (;1;) (mut i32) (get_global 0))
  (export "__Z10dummy_funcv" (func 3))
  (export "_main" (func 1))
  (export "stackAlloc" (func 4)))
