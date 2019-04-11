/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_TRACE_PROCESSOR_SYSCALLS_X86_64_H_
#define SRC_TRACE_PROCESSOR_SYSCALLS_X86_64_H_

namespace perfetto {
namespace trace_processor {
namespace {

// See tools/extract_linux_syscall_tables .
const char* kSyscalls_x86_64[] = {
    "sys_read",                    // 0
    "sys_write",                   // 1
    "sys_open",                    // 2
    "sys_close",                   // 3
    "sys_stat",                    // 4
    "sys_fstat",                   // 5
    "sys_lstat",                   // 6
    "sys_poll",                    // 7
    "sys_lseek",                   // 8
    "sys_mmap",                    // 9
    "sys_mprotect",                // 10
    "sys_munmap",                  // 11
    "sys_brk",                     // 12
    "sys_rt_sigaction",            // 13
    "sys_rt_sigprocmask",          // 14
    "sys_rt_sigreturn",            // 15
    "sys_ioctl",                   // 16
    "sys_pread64",                 // 17
    "sys_pwrite64",                // 18
    "sys_readv",                   // 19
    "sys_writev",                  // 20
    "sys_access",                  // 21
    "sys_pipe",                    // 22
    "sys_select",                  // 23
    "sys_sched_yield",             // 24
    "sys_mremap",                  // 25
    "sys_msync",                   // 26
    "sys_mincore",                 // 27
    "sys_madvise",                 // 28
    "sys_shmget",                  // 29
    "sys_shmat",                   // 30
    "sys_shmctl",                  // 31
    "sys_dup",                     // 32
    "sys_dup2",                    // 33
    "sys_pause",                   // 34
    "sys_nanosleep",               // 35
    "sys_getitimer",               // 36
    "sys_alarm",                   // 37
    "sys_setitimer",               // 38
    "sys_getpid",                  // 39
    "sys_sendfile",                // 40
    "sys_socket",                  // 41
    "sys_connect",                 // 42
    "sys_accept",                  // 43
    "sys_sendto",                  // 44
    "sys_recvfrom",                // 45
    "sys_sendmsg",                 // 46
    "sys_recvmsg",                 // 47
    "sys_shutdown",                // 48
    "sys_bind",                    // 49
    "sys_listen",                  // 50
    "sys_getsockname",             // 51
    "sys_getpeername",             // 52
    "sys_socketpair",              // 53
    "sys_setsockopt",              // 54
    "sys_getsockopt",              // 55
    "sys_clone",                   // 56
    "sys_fork",                    // 57
    "sys_vfork",                   // 58
    "sys_execve",                  // 59
    "sys_exit",                    // 60
    "sys_wait4",                   // 61
    "sys_kill",                    // 62
    "sys_uname",                   // 63
    "sys_semget",                  // 64
    "sys_semop",                   // 65
    "sys_semctl",                  // 66
    "sys_shmdt",                   // 67
    "sys_msgget",                  // 68
    "sys_msgsnd",                  // 69
    "sys_msgrcv",                  // 70
    "sys_msgctl",                  // 71
    "sys_fcntl",                   // 72
    "sys_flock",                   // 73
    "sys_fsync",                   // 74
    "sys_fdatasync",               // 75
    "sys_truncate",                // 76
    "sys_ftruncate",               // 77
    "sys_getdents",                // 78
    "sys_getcwd",                  // 79
    "sys_chdir",                   // 80
    "sys_fchdir",                  // 81
    "sys_rename",                  // 82
    "sys_mkdir",                   // 83
    "sys_rmdir",                   // 84
    "sys_creat",                   // 85
    "sys_link",                    // 86
    "sys_unlink",                  // 87
    "sys_symlink",                 // 88
    "sys_readlink",                // 89
    "sys_chmod",                   // 90
    "sys_fchmod",                  // 91
    "sys_chown",                   // 92
    "sys_fchown",                  // 93
    "sys_lchown",                  // 94
    "sys_umask",                   // 95
    "sys_gettimeofday",            // 96
    "sys_getrlimit",               // 97
    "sys_getrusage",               // 98
    "sys_sysinfo",                 // 99
    "sys_times",                   // 100
    "sys_ptrace",                  // 101
    "sys_getuid",                  // 102
    "sys_syslog",                  // 103
    "sys_getgid",                  // 104
    "sys_setuid",                  // 105
    "sys_setgid",                  // 106
    "sys_geteuid",                 // 107
    "sys_getegid",                 // 108
    "sys_setpgid",                 // 109
    "sys_getppid",                 // 110
    "sys_getpgrp",                 // 111
    "sys_setsid",                  // 112
    "sys_setreuid",                // 113
    "sys_setregid",                // 114
    "sys_getgroups",               // 115
    "sys_setgroups",               // 116
    "sys_setresuid",               // 117
    "sys_getresuid",               // 118
    "sys_setresgid",               // 119
    "sys_getresgid",               // 120
    "sys_getpgid",                 // 121
    "sys_setfsuid",                // 122
    "sys_setfsgid",                // 123
    "sys_getsid",                  // 124
    "sys_capget",                  // 125
    "sys_capset",                  // 126
    "sys_rt_sigpending",           // 127
    "sys_rt_sigtimedwait",         // 128
    "sys_rt_sigqueueinfo",         // 129
    "sys_rt_sigsuspend",           // 130
    "sys_sigaltstack",             // 131
    "sys_utime",                   // 132
    "sys_mknod",                   // 133
    "sys_uselib",                  // 134
    "sys_personality",             // 135
    "sys_ustat",                   // 136
    "sys_statfs",                  // 137
    "sys_fstatfs",                 // 138
    "sys_sysfs",                   // 139
    "sys_getpriority",             // 140
    "sys_setpriority",             // 141
    "sys_sched_setparam",          // 142
    "sys_sched_getparam",          // 143
    "sys_sched_setscheduler",      // 144
    "sys_sched_getscheduler",      // 145
    "sys_sched_get_priority_max",  // 146
    "sys_sched_get_priority_min",  // 147
    "sys_sched_rr_get_interval",   // 148
    "sys_mlock",                   // 149
    "sys_munlock",                 // 150
    "sys_mlockall",                // 151
    "sys_munlockall",              // 152
    "sys_vhangup",                 // 153
    "sys_modify_ldt",              // 154
    "sys_pivot_root",              // 155
    "sys__sysctl",                 // 156
    "sys_prctl",                   // 157
    "sys_arch_prctl",              // 158
    "sys_adjtimex",                // 159
    "sys_setrlimit",               // 160
    "sys_chroot",                  // 161
    "sys_sync",                    // 162
    "sys_acct",                    // 163
    "sys_settimeofday",            // 164
    "sys_mount",                   // 165
    "sys_umount2",                 // 166
    "sys_swapon",                  // 167
    "sys_swapoff",                 // 168
    "sys_reboot",                  // 169
    "sys_sethostname",             // 170
    "sys_setdomainname",           // 171
    "sys_iopl",                    // 172
    "sys_ioperm",                  // 173
    "sys_create_module",           // 174
    "sys_init_module",             // 175
    "sys_delete_module",           // 176
    "sys_get_kernel_syms",         // 177
    "sys_query_module",            // 178
    "sys_quotactl",                // 179
    "sys_nfsservctl",              // 180
    "sys_getpmsg",                 // 181
    "sys_putpmsg",                 // 182
    "sys_afs_syscall",             // 183
    "sys_tuxcall",                 // 184
    "sys_security",                // 185
    "sys_gettid",                  // 186
    "sys_readahead",               // 187
    "sys_setxattr",                // 188
    "sys_lsetxattr",               // 189
    "sys_fsetxattr",               // 190
    "sys_getxattr",                // 191
    "sys_lgetxattr",               // 192
    "sys_fgetxattr",               // 193
    "sys_listxattr",               // 194
    "sys_llistxattr",              // 195
    "sys_flistxattr",              // 196
    "sys_removexattr",             // 197
    "sys_lremovexattr",            // 198
    "sys_fremovexattr",            // 199
    "sys_tkill",                   // 200
    "sys_time",                    // 201
    "sys_futex",                   // 202
    "sys_sched_setaffinity",       // 203
    "sys_sched_getaffinity",       // 204
    "sys_set_thread_area",         // 205
    "sys_io_setup",                // 206
    "sys_io_destroy",              // 207
    "sys_io_getevents",            // 208
    "sys_io_submit",               // 209
    "sys_io_cancel",               // 210
    "sys_get_thread_area",         // 211
    "sys_lookup_dcookie",          // 212
    "sys_epoll_create",            // 213
    "sys_epoll_ctl_old",           // 214
    "sys_epoll_wait_old",          // 215
    "sys_remap_file_pages",        // 216
    "sys_getdents64",              // 217
    "sys_set_tid_address",         // 218
    "sys_restart_syscall",         // 219
    "sys_semtimedop",              // 220
    "sys_fadvise64",               // 221
    "sys_timer_create",            // 222
    "sys_timer_settime",           // 223
    "sys_timer_gettime",           // 224
    "sys_timer_getoverrun",        // 225
    "sys_timer_delete",            // 226
    "sys_clock_settime",           // 227
    "sys_clock_gettime",           // 228
    "sys_clock_getres",            // 229
    "sys_clock_nanosleep",         // 230
    "sys_exit_group",              // 231
    "sys_epoll_wait",              // 232
    "sys_epoll_ctl",               // 233
    "sys_tgkill",                  // 234
    "sys_utimes",                  // 235
    "sys_vserver",                 // 236
    "sys_mbind",                   // 237
    "sys_set_mempolicy",           // 238
    "sys_get_mempolicy",           // 239
    "sys_mq_open",                 // 240
    "sys_mq_unlink",               // 241
    "sys_mq_timedsend",            // 242
    "sys_mq_timedreceive",         // 243
    "sys_mq_notify",               // 244
    "sys_mq_getsetattr",           // 245
    "sys_kexec_load",              // 246
    "sys_waitid",                  // 247
    "sys_add_key",                 // 248
    "sys_request_key",             // 249
    "sys_keyctl",                  // 250
    "sys_ioprio_set",              // 251
    "sys_ioprio_get",              // 252
    "sys_inotify_init",            // 253
    "sys_inotify_add_watch",       // 254
    "sys_inotify_rm_watch",        // 255
    "sys_migrate_pages",           // 256
    "sys_openat",                  // 257
    "sys_mkdirat",                 // 258
    "sys_mknodat",                 // 259
    "sys_fchownat",                // 260
    "sys_futimesat",               // 261
    "sys_newfstatat",              // 262
    "sys_unlinkat",                // 263
    "sys_renameat",                // 264
    "sys_linkat",                  // 265
    "sys_symlinkat",               // 266
    "sys_readlinkat",              // 267
    "sys_fchmodat",                // 268
    "sys_faccessat",               // 269
    "sys_pselect6",                // 270
    "sys_ppoll",                   // 271
    "sys_unshare",                 // 272
    "sys_set_robust_list",         // 273
    "sys_get_robust_list",         // 274
    "sys_splice",                  // 275
    "sys_tee",                     // 276
    "sys_sync_file_range",         // 277
    "sys_vmsplice",                // 278
    "sys_move_pages",              // 279
    "sys_utimensat",               // 280
    "sys_epoll_pwait",             // 281
    "sys_signalfd",                // 282
    "sys_timerfd_create",          // 283
    "sys_eventfd",                 // 284
    "sys_fallocate",               // 285
    "sys_timerfd_settime",         // 286
    "sys_timerfd_gettime",         // 287
    "sys_accept4",                 // 288
    "sys_signalfd4",               // 289
    "sys_eventfd2",                // 290
    "sys_epoll_create1",           // 291
    "sys_dup3",                    // 292
    "sys_pipe2",                   // 293
    "sys_inotify_init1",           // 294
    "sys_preadv",                  // 295
    "sys_pwritev",                 // 296
    "sys_rt_tgsigqueueinfo",       // 297
    "sys_perf_event_open",         // 298
    "sys_recvmmsg",                // 299
    "sys_fanotify_init",           // 300
    "sys_fanotify_mark",           // 301
    "sys_prlimit64",               // 302
    "sys_name_to_handle_at",       // 303
    "sys_open_by_handle_at",       // 304
    "sys_clock_adjtime",           // 305
    "sys_syncfs",                  // 306
    "sys_sendmmsg",                // 307
    "sys_setns",                   // 308
    "sys_getcpu",                  // 309
    "sys_process_vm_readv",        // 310
    "sys_process_vm_writev",       // 311
    "sys_kcmp",                    // 312
    "sys_finit_module",            // 313
    "sys_sched_setattr",           // 314
    "sys_sched_getattr",           // 315
    "sys_renameat2",               // 316
    "sys_seccomp",                 // 317
    "sys_getrandom",               // 318
    "sys_memfd_create",            // 319
    "sys_kexec_file_load",         // 320
    "sys_bpf",                     // 321
    "sys_execveat",                // 322
    "sys_userfaultfd",             // 323
    "sys_membarrier",              // 324
    "sys_mlock2",                  // 325
    "sys_copy_file_range",         // 326
    "sys_preadv2",                 // 327
    "sys_pwritev2",                // 328
    "sys_pkey_mprotect",           // 329
    "sys_pkey_alloc",              // 330
    "sys_pkey_free",               // 331
    "sys_statx",                   // 332
    "sys_io_pgetevents",           // 333
    "sys_rseq",                    // 334
    "",                            // 335
    "",                            // 336
    "",                            // 337
    "",                            // 338
    "",                            // 339
    "",                            // 340
    "",                            // 341
    "",                            // 342
    "",                            // 343
    "",                            // 344
    "",                            // 345
    "",                            // 346
    "",                            // 347
    "",                            // 348
    "",                            // 349
    "",                            // 350
    "",                            // 351
    "",                            // 352
    "",                            // 353
    "",                            // 354
    "",                            // 355
    "",                            // 356
    "",                            // 357
    "",                            // 358
    "",                            // 359
    "",                            // 360
    "",                            // 361
    "",                            // 362
    "",                            // 363
    "",                            // 364
    "",                            // 365
    "",                            // 366
    "",                            // 367
    "",                            // 368
    "",                            // 369
    "",                            // 370
    "",                            // 371
    "",                            // 372
    "",                            // 373
    "",                            // 374
    "",                            // 375
    "",                            // 376
    "",                            // 377
    "",                            // 378
    "",                            // 379
    "",                            // 380
    "",                            // 381
    "",                            // 382
    "",                            // 383
    "",                            // 384
    "",                            // 385
    "",                            // 386
    "",                            // 387
    "",                            // 388
    "",                            // 389
    "",                            // 390
    "",                            // 391
    "",                            // 392
    "",                            // 393
    "",                            // 394
    "",                            // 395
    "",                            // 396
    "",                            // 397
    "",                            // 398
    "",                            // 399
    "",                            // 400
    "",                            // 401
    "",                            // 402
    "",                            // 403
    "",                            // 404
    "",                            // 405
    "",                            // 406
    "",                            // 407
    "",                            // 408
    "",                            // 409
    "",                            // 410
    "",                            // 411
    "",                            // 412
    "",                            // 413
    "",                            // 414
    "",                            // 415
    "",                            // 416
    "",                            // 417
    "",                            // 418
    "",                            // 419
    "",                            // 420
    "",                            // 421
    "",                            // 422
    "",                            // 423
    "",                            // 424
    "",                            // 425
    "",                            // 426
    "",                            // 427
    "",                            // 428
    "",                            // 429
    "",                            // 430
    "",                            // 431
    "",                            // 432
    "",                            // 433
    "",                            // 434
    "",                            // 435
    "",                            // 436
    "",                            // 437
    "",                            // 438
    "",                            // 439
    "",                            // 440
    "",                            // 441
    "",                            // 442
    "",                            // 443
    "",                            // 444
    "",                            // 445
    "",                            // 446
    "",                            // 447
    "",                            // 448
    "",                            // 449
    "",                            // 450
    "",                            // 451
    "",                            // 452
    "",                            // 453
    "",                            // 454
    "",                            // 455
    "",                            // 456
    "",                            // 457
    "",                            // 458
    "",                            // 459
    "",                            // 460
    "",                            // 461
    "",                            // 462
    "",                            // 463
    "",                            // 464
    "",                            // 465
    "",                            // 466
    "",                            // 467
    "",                            // 468
    "",                            // 469
    "",                            // 470
    "",                            // 471
    "",                            // 472
    "",                            // 473
    "",                            // 474
    "",                            // 475
    "",                            // 476
    "",                            // 477
    "",                            // 478
    "",                            // 479
    "",                            // 480
    "",                            // 481
    "",                            // 482
    "",                            // 483
    "",                            // 484
    "",                            // 485
    "",                            // 486
    "",                            // 487
    "",                            // 488
    "",                            // 489
    "",                            // 490
    "",                            // 491
    "",                            // 492
    "",                            // 493
    "",                            // 494
    "",                            // 495
    "",                            // 496
    "",                            // 497
    "",                            // 498
    "",                            // 499
    "",                            // 500
    "",                            // 501
    "",                            // 502
    "",                            // 503
    "",                            // 504
    "",                            // 505
    "",                            // 506
    "",                            // 507
    "",                            // 508
    "",                            // 509
    "",                            // 510
    "",                            // 511
    "sys_rt_sigaction",            // 512
    "sys_rt_sigreturn",            // 513
    "sys_ioctl",                   // 514
    "sys_readv",                   // 515
    "sys_writev",                  // 516
    "sys_recvfrom",                // 517
    "sys_sendmsg",                 // 518
    "sys_recvmsg",                 // 519
    "sys_execve",                  // 520
    "sys_ptrace",                  // 521
    "sys_rt_sigpending",           // 522
    "sys_rt_sigtimedwait",         // 523
    "sys_rt_sigqueueinfo",         // 524
    "sys_sigaltstack",             // 525
    "sys_timer_create",            // 526
    "sys_mq_notify",               // 527
    "sys_kexec_load",              // 528
    "sys_waitid",                  // 529
    "sys_set_robust_list",         // 530
    "sys_get_robust_list",         // 531
    "sys_vmsplice",                // 532
    "sys_move_pages",              // 533
    "sys_preadv",                  // 534
    "sys_pwritev",                 // 535
    "sys_rt_tgsigqueueinfo",       // 536
    "sys_recvmmsg",                // 537
    "sys_sendmmsg",                // 538
    "sys_process_vm_readv",        // 539
    "sys_process_vm_writev",       // 540
    "sys_setsockopt",              // 541
    "sys_getsockopt",              // 542
    "sys_io_setup",                // 543
    "sys_io_submit",               // 544
    "sys_execveat",                // 545
    "sys_preadv2",                 // 546
    "sys_pwritev2",                // 547
};

}  // namespace
}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_SYSCALLS_X86_64_H_
