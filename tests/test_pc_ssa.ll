; ModuleID = 'test_pc_ssa.ll'
source_filename = "test_pc.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_path_condition(i32 noundef %0) #0 {
  %2 = icmp sgt i32 %0, 10
  br i1 %2, label %3, label %5

3:                                                ; preds = %1
  %4 = sub nsw i32 %0, 5
  br label %6

5:                                                ; preds = %1
  br label %6

6:                                                ; preds = %5, %3
  %.0 = phi i32 [ %4, %3 ], [ 0, %5 ]
  ret i32 %.0
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_branch_optimization(i32 noundef %0) #0 {
  %2 = icmp eq i32 %0, 5
  br i1 %2, label %3, label %5

3:                                                ; preds = %1
  %4 = mul nsw i32 %0, 2
  br label %6

5:                                                ; preds = %1
  br label %6

6:                                                ; preds = %5, %3
  %.0 = phi i32 [ %4, %3 ], [ %0, %5 ]
  ret i32 %.0
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_nonzero_branch(i32 noundef %0) #0 {
  %2 = icmp ne i32 %0, 0
  br i1 %2, label %3, label %5

3:                                                ; preds = %1
  %4 = sdiv i32 %0, %0
  br label %6

5:                                                ; preds = %1
  br label %6

6:                                                ; preds = %5, %3
  %.0 = phi i32 [ %4, %3 ], [ 0, %5 ]
  ret i32 %.0
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 7, !"Dwarf Version", i32 4}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{i32 7, !"frame-pointer", i32 2}
!4 = !{!"clang version 19.1.7 ( 19.1.7-2.module+el8.10.0+23045+e1f8e80e)"}
