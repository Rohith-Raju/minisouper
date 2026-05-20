; ModuleID = 'test_strength.c'
source_filename = "test_strength.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @test_mul_power2(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = mul nsw i32 %3, 8
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @test_div_power2(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = udiv i32 %3, 16
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @test_mul_by_one(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = mul nsw i32 %3, 1
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @test_add_zero(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = add nsw i32 %3, 0
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @test_shift_zero(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = shl i32 %3, 0
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @test_and_zero(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = and i32 %3, 0
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @test_or_zero(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = or i32 %3, 0
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @test_complex(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %5 = load i32, ptr %2, align 4
  %6 = mul nsw i32 %5, 4
  store i32 %6, ptr %3, align 4
  %7 = load i32, ptr %3, align 4
  %8 = sdiv i32 %7, 2
  store i32 %8, ptr %4, align 4
  %9 = load i32, ptr %4, align 4
  ret i32 %9
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 7, !"Dwarf Version", i32 4}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{i32 7, !"frame-pointer", i32 2}
!4 = !{!"clang version 19.1.7 ( 19.1.7-2.module+el8.10.0+23045+e1f8e80e)"}
