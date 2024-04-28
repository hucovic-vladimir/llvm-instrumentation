!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "clang version 17.0.0 (https://github.com/llvm/llvm-project fffb389c8b2ff29a7a05fd17addf23a50005b057)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "test/test.c", directory: "/home/vladimir/Documents/llvm-instrumentation", checksumkind: CSK_MD5, checksum: "84aaf278904f9a9e1caf2cc3830e038c")
!2 = !{i32 7, !"Dwarf Version", i32 5}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 8, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{!"clang version 17.0.0 (https://github.com/llvm/llvm-project fffb389c8b2ff29a7a05fd17addf23a50005b057)"}
!10 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 4, type: !11, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !14)
!11 = !DISubroutineType(types: !12)
!12 = !{!13}
!13 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!14 = !{}
!15 = !DILocalVariable(name: "a", scope: !10, file: !1, line: 5, type: !13)
!16 = !DILocation(line: 5, column: 6, scope: !10)
!17 = !DILocation(line: 6, column: 5, scope: !18)
!18 = distinct !DILexicalBlock(scope: !10, file: !1, line: 6, column: 5)
!19 = !DILocation(line: 6, column: 7, scope: !18)
!20 = !DILocation(line: 6, column: 5, scope: !10)
!21 = !DILocation(line: 7, column: 5, scope: !22)


define dso_local i32 @main() #0 !dbg !10 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  call void @llvm.dbg.declare(metadata ptr %2, metadata !15, metadata !DIExpression()), !dbg !16
  store i32 5, ptr %2, align 4, !dbg !16
  %3 = load i32, ptr %2, align 4, !dbg !17
  %4 = icmp sgt i32 %3, 3, !dbg !19
  br i1 %4, label %5, label %6, !dbg !20

