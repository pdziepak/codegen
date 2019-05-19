# CodeGen

[![Build Status](https://travis-ci.com/pdziepak/codegen.svg?branch=master)](https://travis-ci.com/pdziepak/codegen)
[![codecov](https://codecov.io/gh/pdziepak/codegen/branch/master/graph/badge.svg)](https://codecov.io/gh/pdziepak/codegen)

Experimental wrapper over LLVM for generating and compiling code at run-time.

## About

CodeGen is a library that builds on top of LLVM.  It facilitates just-in-time code generation and compilation, including debugging information and human-readable source code. C++ type system is employed to guard against, at least some, errors in the generated intermediate representation. The intention is to allow the application to improve performance by taking advantage of information that becomes available only once it is running. A sample use case would be prepared statements in of database engines.

The general idea is not unlike that described in [P1609R0: C++ Should Support Just-in-Time Compilation](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1609r0.html).

## Building

The build requirements are as follows:

* CMake 3.12
* GCC 8+ or Clang 8+
* LLVM 8
* fmt
* Google Test (optional)

`fedora:30` docker container may be a good place to start.

The build instructions are quite usual for a CMake-based project:

```
cd <build-directory>
cmake -DCMAKE_BUILD_TYPE=<Debug|Release> -G Ninja <source-directory>
ninja
ninja test
```

## Design

The main object representing the JIT compiler is `codegen::compiler`. All function pointers to the compiled code remain valid during its lifetime. `codegen::module_builder` allows creating an LLVM builder, while `codegen::module` represents an already compiled module. The general template that for CodeGen use looks as follows:

```c++
  namespace cg = codegen;
  auto compiler = cg::compiler{};
  auto builder = cg::module_builder(compiler, "module_name");

  auto function_reference = builder.create_function<int(int)>("function_name",
    [](cg::value<int> v) {
      /* more logic here */
      cg::return_(v + cg::constant<int>(1));
    });

  auto module = std::move(builder).build();
  using function_pointer_type = int(*)(int);
  function_pointer_type function_pointer = module.get_address(function_reference);
```

The code above compiles a function that returns an integer that was passed to it as an argument incremented by one. Each module may contain multiple functions. `codegen::module_builder::create_function` returns a function reference that can be used to obtain a pointer to the function after the module is compiled (as in this example) or to call it from another function generated with CodeGen.

`codegen::value<T>` is a typed equivalent of `llvm::Value` and represents a SSA value. As of now, only fundamental types are supported. CodeGen provides operators for those arithmetic and relational operations that make sense for a given type. Expression templates are used in a limited fashion to allow producing more concise human-readable source code. Unlike C++ there are no automatic promotions or implicit casts of any kind. Instead, `bit_cast<T>` or `cast<T>` need to be explicitly used where needed.

SSA starts getting a bit more cumbersome to use once the control flow diverges, and a Φ function is required. This can be avoided by using local variables `codegen::variable<T>`. The resulting IR is not going to be perfect, but the LLVM optimisation passes tend to do an excellent job converting those memory accesses.

### Statements


* `return_()`, `return_(Value)` – returns from function. Note, that the type of the returned value is not verified and CodeGen will not prevent generating a function of type `int()` that returns `void`.
* `load(Pointer)` – takes a pointer of type `T*` and loads the value from memory,  `codegen::value<T >`.
* `store(Value, Pointer)` – stores `Value` of type `T` at the location pointed to by `Pointer`. The type of the pointer needs to be `T*`.
* `if_(Value, TrueBlock, FalseBlock)`, `if_(Value, TrueBlock)` – an `if` conditional statement. The type of the provided value needs to be `bool`. `TrueBlock` and `FalseBlock` are expected to be lambdas. For example:

```c++
auto silly_function = builder.create_function<bool(bool)>("silly_function",
    [](cg::value<bool> is_true) {
      cg::if_(is_true, [] { cg::return_(cg::true_()); }, [] { cg::return_(cg::false_()); });
    });
```

* `while_(Condition, LoopBody)` – a `while` loop. `Condition` is a lambda returning a value of type `bool`. `LoopBody` is a lambda that generates the body of the loop. For example:

```c++
auto silly_function2 = builder.create_function<unsigned(unsigned)>("silly_function2",
    [](cg::value<unsigned> target) {
      auto var = cg::variable<unsigned>("var", cg::constant<unsigned>(0));
      cg::while_([&] { return var.get() < target; },
        [&] {
          var.set(var.get() + cg::constant<unsigned>(1));
        });
      cg::return_(var.get());
    });
```

* `call(Function, Arguments...)` – a function call. `Function` is a function reference. `Arguments...` is a list of arguments matching the function type.

## Examples

### Tuple comparator


In this example, let's consider tuples which element's types are known only at run-time. If the goal is to write a less-comparator for such tuples, the naive approach would be to have a virtual function call for each element. That is far from ideal if the actual comparison is very cheap, e.g. the elements are integers. With CodeGen, we can do better. First, let's write comparators for individual types:

```c++
size_t less_i32(cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr, size_t off) {
  auto a_val = cg::load(cg::bit_cast<int32_t*>(a_ptr + cg::constant<uint64_t>(off)));
  auto b_val = cg::load(cg::bit_cast<int32_t*>(b_ptr + cg::constant<uint64_t>(off)));
  cg::if_(a_val < b_val, [&] { cg::return_(cg::true_()); });
  cg::if_(a_val > b_val, [&] { cg::return_(cg::false_()); });
  return sizeof(int32_t) + off;
}
size_t less_u16(cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr, size_t off) {
  auto a_val = cg::load(cg::bit_cast<uint16_t*>(a_ptr + cg::constant<uint64_t>(off)));
  auto b_val = cg::load(cg::bit_cast<uint16_t*>(b_ptr + cg::constant<uint64_t>(off)));
  cg::if_(a_val < b_val, [&] { cg::return_(cg::true_()); });
  cg::if_(a_val > b_val, [&] { cg::return_(cg::false_()); });
  return sizeof(uint16_t) + off;
}
size_t less_f32(cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr, size_t off) {
  auto a_val = cg::load(cg::bit_cast<float*>(a_ptr + cg::constant<uint64_t>(off)));
  auto b_val = cg::load(cg::bit_cast<float*>(b_ptr + cg::constant<uint64_t>(off)));
  cg::if_(a_val < b_val, [&] { cg::return_(cg::true_()); });
  cg::if_(a_val > b_val, [&] { cg::return_(cg::false_()); });
  return sizeof(float) + off;
}
```

Those lambdas generate comparison code for `int32_t`, `uint16_t` and `float`, respectively. The arguments are pointers to buffers containing both tuples and an offset at which the element is located. The return value is the offset of the next element.

Now, let's say we want to generate a less-comparator for `tuple<i32, float, u16>`.

```c++
  auto less = builder.create_function<bool(std::byte*, std::byte*)>(
      "less", [&](cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr) {
        size_t offset = 0;
        offset = less_i32(a_ptr, b_ptr, offset);
        offset = less_f32(a_ptr, b_ptr, offset);
        offset = less_u16(a_ptr, b_ptr, offset);
        (void)offset;
        cg::return_(cg::false_());
      });
```

As we can see, building the actual comparator is quite straightforward. The human-readable source code that CodeGen generates looks like this:

```c
1   bool less(byte* arg0, byte* arg1) {
2       val0 = *bit_cast<i32*>((arg0 + 0))
3       val1 = *bit_cast<i32*>((arg1 + 0))
4       if ((val0 < val1)) {
5           return true;
6       }
7       if ((val0 > val1)) {
8           return false;
9       }
10      val2 = *bit_cast<f32*>((arg0 + 4))
11      val3 = *bit_cast<f32*>((arg1 + 4))
12      if ((val2 < val3)) {
13          return true;
14      }
15      if ((val2 > val3)) {
16          return false;
17      }
18      val4 = *bit_cast<u16*>((arg0 + 8))
19      val5 = *bit_cast<u16*>((arg1 + 8))
20      if ((val4 < val5)) {
21          return true;
22      }
23      if ((val4 > val5)) {
24          return false;
25      }
26      return false;
27  }

```

The assembly that LLVM emits:

```x86asm
   0x00007fffefd57000 <+0>:   mov    (%rdi),%ecx
   0x00007fffefd57002 <+2>:   mov    (%rsi),%edx
   0x00007fffefd57004 <+4>:   mov    $0x1,%al
   0x00007fffefd57006 <+6>:   cmp    %edx,%ecx
   0x00007fffefd57008 <+8>:   jl     0x7fffefd57026 <less+38>
   0x00007fffefd5700a <+10>:  cmp    %edx,%ecx
   0x00007fffefd5700c <+12>:  jg     0x7fffefd57024 <less+36>
   0x00007fffefd5700e <+14>:  vmovss 0x4(%rdi),%xmm0
   0x00007fffefd57013 <+19>:  vmovss 0x4(%rsi),%xmm1
   0x00007fffefd57018 <+24>:  vucomiss %xmm0,%xmm1
   0x00007fffefd5701c <+28>:  ja     0x7fffefd57026 <less+38>
   0x00007fffefd5701e <+30>:  vucomiss %xmm1,%xmm0
   0x00007fffefd57022 <+34>:  jbe    0x7fffefd57027 <less+39>
   0x00007fffefd57024 <+36>:  xor    %eax,%eax
   0x00007fffefd57026 <+38>:  retq
   0x00007fffefd57027 <+39>:  movzwl 0x8(%rdi),%eax
   0x00007fffefd5702b <+43>:  cmp    0x8(%rsi),%ax
   0x00007fffefd5702f <+47>:  setb   %al
   0x00007fffefd57032 <+50>:  retq
```

Since CodeGen takes care of emitting all necessary debugging information, and informing GDB about the JIT-ed functions, the debugging experience shouldn't be too bad:

```
(gdb) b 3
Breakpoint 2 at 0x7fffefd57002: file /tmp/examples-11076310111440055155/tuple_i32f32u16_less.txt, line 3.
(gdb) c
Continuing.

Breakpoint 2, less (arg0=0x60200001c7b0 "", arg1=0x60200001c790 "\001") at /tmp/examples-11076310111440055155/tuple_i32f32u16_less.txt:3
3	    val1 = *bit_cast<i32*>((arg1 + 0))
(gdb) p val0
$1 = 0
(gdb) n
4	    if ((val0 < val1)) {
(gdb) p val1
$3 = 1
(gdb) n
less (arg0=0x60200001c7b0 "", arg1=0x60200001c790 "\001") at /tmp/examples-11076310111440055155/tuple_i32f32u16_less.txt:5
5	        return true;
```

A more complicated example would be if one of the tuple elements was an ASCII string. The following code generates a comparator for `tuple<i32, string>` assuming that a string is serialised in the form of `<length:u32><bytes...>`:

```c++
  auto less = builder.create_function<bool(std::byte*, std::byte*)>(
      "less", [&](cg::value<std::byte*> a_ptr, cg::value<std::byte*> b_ptr) {
        size_t offset = 0;
        offset = less_i32(a_ptr, b_ptr, offset);

        auto a_len = cg::load(cg::bit_cast<uint32_t*>(a_ptr + cg::constant<uint64_t>(offset)));
        auto b_len = cg::load(cg::bit_cast<uint32_t*>(b_ptr + cg::constant<uint64_t>(offset)));
        // TODO: extract to a separate function
        auto len = cg::call(min, a_len, b_len);
        auto ret = cg::builtin::memcmp(a_ptr + cg::constant<uint64_t>(offset) + 4_u64,
                                       b_ptr + cg::constant<uint64_t>(offset) + 4_u64, len);
        cg::if_(ret < 0_i32, [&] { cg::return_(cg::true_()); });
        cg::if_(ret > 0_i32, [&] { cg::return_(cg::false_()); });
        cg::return_(a_len < b_len);
      });
```

Let's look at the emitted assembly mixed with human-readable source code:

```
(gdb) disas /s less
Dump of assembler code for function less:
/tmp/examples-12144749341750180701/tuple_i32str_less.txt:
7  bool less(byte* arg0, byte* arg1) {
   0x00007fffefd47010 <+0>:   push   %rbp
   0x00007fffefd47011 <+1>:   push   %r14
   0x00007fffefd47013 <+3>:   push   %rbx

8      val6 = *bit_cast<i32*>((arg0 + 0))
   0x00007fffefd47014 <+4>:   mov    (%rdi),%eax

9      val7 = *bit_cast<i32*>((arg1 + 0))
   0x00007fffefd47016 <+6>:   mov    (%rsi),%ecx
   0x00007fffefd47018 <+8>:   mov    $0x1,%bl

10      if ((val6 < val7)) {
   0x00007fffefd4701a <+10>:  cmp    %ecx,%eax
   0x00007fffefd4701c <+12>:  jl     0x7fffefd4704e <less+62>

12      }
13      if ((val6 > val7)) {
   0x00007fffefd4701e <+14>:  cmp    %ecx,%eax
   0x00007fffefd47020 <+16>:  jle    0x7fffefd47026 <less+22>
   0x00007fffefd47022 <+18>:  xor    %ebx,%ebx
   0x00007fffefd47024 <+20>:  jmp    0x7fffefd4704e <less+62>

14          return false;
15      }
16      val8 = *bit_cast<u32*>((arg0 + 4))
   0x00007fffefd47026 <+22>:  mov    0x4(%rdi),%r14d

17      val9 = *bit_cast<u32*>((arg1 + 4))
   0x00007fffefd4702a <+26>:  mov    0x4(%rsi),%ebp

2      if ((arg0 < arg1)) {
   0x00007fffefd4702d <+29>:  cmp    %ebp,%r14d
   0x00007fffefd47030 <+32>:  mov    %ebp,%edx
   0x00007fffefd47032 <+34>:  cmovb  %r14d,%edx

18      min_ret = min(val8, val9, );
19      memcmp_ret = memcmp(((arg0 + 4) + 4), ((arg1 + 4) + 4), min_ret);
   0x00007fffefd47036 <+38>:  add    $0x8,%rdi
   0x00007fffefd4703a <+42>:  add    $0x8,%rsi
   0x00007fffefd4703e <+46>:  movabs $0x7ffff764bd90,%rax
   0x00007fffefd47048 <+56>:  callq  *%rax

20      if ((memcmp_ret < 0)) {
   0x00007fffefd4704a <+58>:  test   %eax,%eax
   0x00007fffefd4704c <+60>:  jns    0x7fffefd47055 <less+69>

11          return true;
   0x00007fffefd4704e <+62>:  mov    %ebx,%eax
   0x00007fffefd47050 <+64>:  pop    %rbx
   0x00007fffefd47051 <+65>:  pop    %r14
   0x00007fffefd47053 <+67>:  pop    %rbp
   0x00007fffefd47054 <+68>:  retq

2      if ((arg0 < arg1)) {
   0x00007fffefd47055 <+69>:  cmp    %ebp,%r14d
   0x00007fffefd47058 <+72>:  setb   %cl

21          return true;
22      }
23      if ((memcmp_ret > 0)) {
   0x00007fffefd4705b <+75>:  test   %eax,%eax
   0x00007fffefd4705d <+77>:  sete   %al
   0x00007fffefd47060 <+80>:  and    %cl,%al
   0x00007fffefd47062 <+82>:  pop    %rbx
   0x00007fffefd47063 <+83>:  pop    %r14
   0x00007fffefd47065 <+85>:  pop    %rbp
   0x00007fffefd47066 <+86>:  retq
```

As we can see, LLVM has inlined calls to `min`. `memcmp` is an external function, so it could never be inlined. The source code lines match the assembly most of the time, but slight confusion there is expected since the code is compiled with aggressive optimisations.

### Vectorisation

In the previous example, we knew the computations that we wanted to perform but didn't know the data. Let's now look at the opposite situation. The data organised as a structure of arrays, but we don't know ahead of time what arithmetic operations the application will need to execute. How the information about the desired computations is represented is out of the scope of CodeGen, though we may suspect an abstract syntax tree being involved there. The application would have to translate that to appropriate calls to CodeGen. For example, if for a value `a` and arrays `b` and `c` we wanted to compute `d[i] = a * b[i] + c[i]` it could be achieved by the code like this:

```c++
  auto compute = builder.create_function<void(int32_t, int32_t*, int32_t*, int32_t*, uint64_t)>(
      "compute", [&](cg::value<int32_t> a, cg::value<int32_t*> b_ptr, cg::value<int32_t*> c_ptr,
                     cg::value<int32_t*> d_ptr, cg::value<uint64_t> n) {
        auto idx = cg::variable<uint64_t>("idx", 0_u64);
        cg::while_([&] { return idx.get() < n; },
                   [&] {
                     auto i = idx.get();
                     cg::store(a * cg::load(b_ptr + i) + cg::load(c_ptr + i), d_ptr + i);
                     idx.set(i + 1_u64);
                   });
        cg::return_();
      });
```

CodeGen configures LLVM so that it takes advantage of the features available on the CPU it executes on. For instance, Skylake supports AVX2, so it is going to be used to vectorise the loop.

```x86asm
6            val11 = *(arg1 + idx)
7            *(arg3 + idx) = ((arg0 * val11) + val10)
   0x00007fffefd27140 <+320>:    vpmulld (%rsi,%r9,4),%ymm0,%ymm1
   0x00007fffefd27146 <+326>:    vpmulld 0x20(%rsi,%r9,4),%ymm0,%ymm2
   0x00007fffefd2714d <+333>:    vpmulld 0x40(%rsi,%r9,4),%ymm0,%ymm3
   0x00007fffefd27154 <+340>:    vpmulld 0x60(%rsi,%r9,4),%ymm0,%ymm4
   0x00007fffefd2715b <+347>:    vpaddd (%rdx,%r9,4),%ymm1,%ymm1
   0x00007fffefd27161 <+353>:    vpaddd 0x20(%rdx,%r9,4),%ymm2,%ymm2
   0x00007fffefd27168 <+360>:    vpaddd 0x40(%rdx,%r9,4),%ymm3,%ymm3
   0x00007fffefd2716f <+367>:    vpaddd 0x60(%rdx,%r9,4),%ymm4,%ymm4
   0x00007fffefd27176 <+374>:    vmovdqu %ymm1,(%rcx,%r9,4)
   0x00007fffefd2717c <+380>:    vmovdqu %ymm2,0x20(%rcx,%r9,4)
   0x00007fffefd27183 <+387>:    vmovdqu %ymm3,0x40(%rcx,%r9,4)
   0x00007fffefd2718a <+394>:    vmovdqu %ymm4,0x60(%rcx,%r9,4)
   0x00007fffefd27191 <+401>:    vpmulld 0x80(%rsi,%r9,4),%ymm0,%ymm1
   0x00007fffefd2719b <+411>:    vpmulld 0xa0(%rsi,%r9,4),%ymm0,%ymm2
   0x00007fffefd271a5 <+421>:    vpmulld 0xc0(%rsi,%r9,4),%ymm0,%ymm3
   0x00007fffefd271af <+431>:    vpmulld 0xe0(%rsi,%r9,4),%ymm0,%ymm4
   0x00007fffefd271b9 <+441>:    vpaddd 0x80(%rdx,%r9,4),%ymm1,%ymm1
   0x00007fffefd271c3 <+451>:    vpaddd 0xa0(%rdx,%r9,4),%ymm2,%ymm2
   0x00007fffefd271cd <+461>:    vpaddd 0xc0(%rdx,%r9,4),%ymm3,%ymm3
   0x00007fffefd271d7 <+471>:    vpaddd 0xe0(%rdx,%r9,4),%ymm4,%ymm4
   0x00007fffefd271e1 <+481>:    vmovdqu %ymm1,0x80(%rcx,%r9,4)
   0x00007fffefd271eb <+491>:    vmovdqu %ymm2,0xa0(%rcx,%r9,4)
   0x00007fffefd271f5 <+501>:    vmovdqu %ymm3,0xc0(%rcx,%r9,4)
   0x00007fffefd271ff <+511>:    vmovdqu %ymm4,0xe0(%rcx,%r9,4)
8            idx = (idx + 1);
   0x00007fffefd27209 <+521>:    add    $0x40,%r9
   0x00007fffefd2720d <+525>:    add    $0x2,%r11
   0x00007fffefd27211 <+529>:    jne    0x7fffefd27140 <compute+320>
   0x00007fffefd27217 <+535>:    test   %r10,%r10
   0x00007fffefd2721a <+538>:    je     0x7fffefd2726d <compute+621>
```

## TODO

* Support for aggregate types. This requires CodeGen to be aware of the ABI and would benefit if C++ had any form of static reflection.
* Add missing operations (e.g. shifts).
* Type-Based Alias Anaylsis.
* Allow the user to tune optimisation options and disable generation of debugging information.
* Bind compiled functions lifetimes to their module instead of the compiler object.
* Support for other versions of LLVM.
* Allow adding more metadata and attribute, e.g. `noalias` for function parameters.
* Try harder to use C++ type system to prevent generation of invalid LLVM IR.
* The TODO list is incomplete. Add more items to it.
