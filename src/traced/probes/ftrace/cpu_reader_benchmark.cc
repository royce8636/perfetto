// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <benchmark/benchmark.h>

#include "perfetto/ext/base/utils.h"
#include "perfetto/protozero/root_message.h"
#include "perfetto/protozero/scattered_stream_null_delegate.h"
#include "perfetto/protozero/scattered_stream_writer.h"
#include "protos/perfetto/trace/ftrace/ftrace_event_bundle.pbzero.h"
#include "src/traced/probes/ftrace/cpu_reader.h"
#include "src/traced/probes/ftrace/ftrace_config_muxer.h"
#include "src/traced/probes/ftrace/ftrace_print_filter.h"
#include "src/traced/probes/ftrace/proto_translation_table.h"
#include "src/traced/probes/ftrace/test/cpu_reader_support.h"

namespace perfetto {
namespace {

using ::perfetto::protos::pbzero::FtraceEventBundle;
using ::protozero::ScatteredStreamWriter;
using ::protozero::ScatteredStreamWriterNullDelegate;

// A page full of "sched/sched_switch" events.
ExamplePage g_full_page_sched_switch{
    "synthetic",
    R"(
00000000: 31f2 7622 1a00 0000 b40f 0000 0000 0000  1.v"............
00000010: 1e00 0000 0000 0000 1000 0000 2f00 0103  ............/...
00000020: 140d 0000 4a69 7420 7468 7265 6164 2070  ....Jit thread p
00000030: 6f6f 6c00 140d 0000 8100 0000 0008 0000  ool.............
00000040: 0000 0000 4576 656e 7454 6872 6561 6400  ....EventThread.
00000050: 6572 0000 7002 0000 6100 0000 f057 0e00  er..p...a....W..
00000060: 2f00 0103 7002 0000 4576 656e 7454 6872  /...p...EventThr
00000070: 6561 6400 6572 0000 7002 0000 6100 0000  ead.er..p...a...
00000080: 0100 0000 0000 0000 4a69 7420 7468 7265  ........Jit thre
00000090: 6164 2070 6f6f 6c00 140d 0000 8100 0000  ad pool.........
000000a0: 50c2 0910 2f00 0103 140d 0000 4a69 7420  P.../.......Jit
000000b0: 7468 7265 6164 2070 6f6f 6c00 140d 0000  thread pool.....
000000c0: 8100 0000 0100 0000 0000 0000 7377 6170  ............swap
000000d0: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
000000e0: 7800 0000 901a c80e 2f00 0103 0000 0000  x......./.......
000000f0: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000100: 0000 0000 7800 0000 0000 0000 0000 0000  ....x...........
00000110: 4469 7370 5379 6e63 0069 6e67 6572 0000  DispSync.inger..
00000120: 6f02 0000 6100 0000 1064 1e00 2f00 0103  o...a....d../...
00000130: 6f02 0000 4469 7370 5379 6e63 0069 6e67  o...DispSync.ing
00000140: 6572 0000 6f02 0000 6100 0000 0100 0000  er..o...a.......
00000150: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000160: 0000 0000 0000 0000 7800 0000 9074 8600  ........x....t..
00000170: 2f00 0103 0000 0000 7377 6170 7065 722f  /.......swapper/
00000180: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
00000190: 0000 0000 0000 0000 4576 656e 7454 6872  ........EventThr
000001a0: 6561 6400 6572 0000 7002 0000 6100 0000  ead.er..p...a...
000001b0: d071 0b00 2f00 0103 7002 0000 4576 656e  .q../...p...Even
000001c0: 7454 6872 6561 6400 6572 0000 7002 0000  tThread.er..p...
000001d0: 6100 0000 0100 0000 0000 0000 7377 6170  a...........swap
000001e0: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
000001f0: 7800 0000 10cd 4504 2f00 0103 0000 0000  x.....E./.......
00000200: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000210: 0000 0000 7800 0000 0000 0000 0000 0000  ....x...........
00000220: 7375 676f 763a 3000 0000 0000 0000 0000  sugov:0.........
00000230: 3802 0000 3100 0000 30d6 1300 2f00 0103  8...1...0.../...
00000240: 3802 0000 7375 676f 763a 3000 0000 0000  8...sugov:0.....
00000250: 0000 0000 3802 0000 3100 0000 0100 0000  ....8...1.......
00000260: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000270: 0000 0000 0000 0000 7800 0000 3049 a202  ........x...0I..
00000280: 2f00 0103 0000 0000 7377 6170 7065 722f  /.......swapper/
00000290: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
000002a0: 0000 0000 0000 0000 4469 7370 5379 6e63  ........DispSync
000002b0: 0069 6e67 6572 0000 6f02 0000 6100 0000  .inger..o...a...
000002c0: d07a 1000 2f00 0103 6f02 0000 4469 7370  .z../...o...Disp
000002d0: 5379 6e63 0069 6e67 6572 0000 6f02 0000  Sync.inger..o...
000002e0: 6100 0000 0100 0000 0000 0000 7377 6170  a...........swap
000002f0: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000300: 7800 0000 d085 1100 2f00 0103 0000 0000  x......./.......
00000310: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000320: 0000 0000 7800 0000 0000 0000 0000 0000  ....x...........
00000330: 7375 7266 6163 6566 6c69 6e67 6572 0000  surfaceflinger..
00000340: 4b02 0000 6200 0000 907a f000 2f00 0103  K...b....z../...
00000350: 4b02 0000 7375 7266 6163 6566 6c69 6e67  K...surfacefling
00000360: 6572 0000 4b02 0000 6200 0000 0100 0000  er..K...b.......
00000370: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000380: 0000 0000 0000 0000 7800 0000 305a 6400  ........x...0Zd.
00000390: 2f00 0103 0000 0000 7377 6170 7065 722f  /.......swapper/
000003a0: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
000003b0: 0000 0000 0000 0000 6d64 7373 5f66 6230  ........mdss_fb0
000003c0: 0000 0000 0000 0000 5714 0000 5300 0000  ........W...S...
000003d0: 10b1 9e03 2f00 0103 5714 0000 6d64 7373  ..../...W...mdss
000003e0: 5f66 6230 0000 0000 0000 0000 5714 0000  _fb0........W...
000003f0: 5300 0000 0200 0000 0000 0000 6b73 6f66  S...........ksof
00000400: 7469 7271 642f 3000 0000 0000 0300 0000  tirqd/0.........
00000410: 7800 0000 90bb 9900 2f00 0103 0300 0000  x......./.......
00000420: 6b73 6f66 7469 7271 642f 3000 0000 0000  ksoftirqd/0.....
00000430: 0300 0000 7800 0000 0100 0000 0000 0000  ....x...........
00000440: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000450: 0000 0000 7800 0000 701e 5305 2f00 0103  ....x...p.S./...
00000460: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000470: 0000 0000 0000 0000 7800 0000 0000 0000  ........x.......
00000480: 0000 0000 6b77 6f72 6b65 722f 7531 363a  ....kworker/u16:
00000490: 3600 0000 6401 0000 7800 0000 90a1 2900  6...d...x.....).
000004a0: 2f00 0103 6401 0000 6b77 6f72 6b65 722f  /...d...kworker/
000004b0: 7531 363a 3600 0000 6401 0000 7800 0000  u16:6...d...x...
000004c0: 0200 0000 0000 0000 7377 6170 7065 722f  ........swapper/
000004d0: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
000004e0: b0e5 4f04 2f00 0103 0000 0000 7377 6170  ..O./.......swap
000004f0: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000500: 7800 0000 0000 0000 0000 0000 4269 6e64  x...........Bind
00000510: 6572 3a32 3136 385f 3135 0000 e614 0000  er:2168_15......
00000520: 7800 0000 b0bd 7c00 2f00 0103 e614 0000  x.....|./.......
00000530: 4269 6e64 6572 3a32 3136 385f 3135 0000  Binder:2168_15..
00000540: e614 0000 7800 0000 0100 0000 0000 0000  ....x...........
00000550: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000560: 0000 0000 7800 0000 d0bd 7e01 2f00 0103  ....x.....~./...
00000570: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000580: 0000 0000 0000 0000 7800 0000 0000 0000  ........x.......
00000590: 0000 0000 6b77 6f72 6b65 722f 7531 363a  ....kworker/u16:
000005a0: 3900 0000 e204 0000 7800 0000 7016 0800  9.......x...p...
000005b0: 2f00 0103 e204 0000 6b77 6f72 6b65 722f  /.......kworker/
000005c0: 7531 363a 3900 0000 e204 0000 7800 0000  u16:9.......x...
000005d0: 0100 0000 0000 0000 7377 6170 7065 722f  ........swapper/
000005e0: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
000005f0: 1004 5200 2f00 0103 0000 0000 7377 6170  ..R./.......swap
00000600: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000610: 7800 0000 0000 0000 0000 0000 6b77 6f72  x...........kwor
00000620: 6b65 722f 7531 363a 3900 0000 e204 0000  ker/u16:9.......
00000630: 7800 0000 d0db 0700 2f00 0103 e204 0000  x......./.......
00000640: 6b77 6f72 6b65 722f 7531 363a 3900 0000  kworker/u16:9...
00000650: e204 0000 7800 0000 0100 0000 0000 0000  ....x...........
00000660: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000670: 0000 0000 7800 0000 b0a2 8c00 2f00 0103  ....x......./...
00000680: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000690: 0000 0000 0000 0000 7800 0000 0000 0000  ........x.......
000006a0: 0000 0000 6b77 6f72 6b65 722f 7531 363a  ....kworker/u16:
000006b0: 3900 0000 e204 0000 7800 0000 d02b 0400  9.......x....+..
000006c0: 2f00 0103 e204 0000 6b77 6f72 6b65 722f  /.......kworker/
000006d0: 7531 363a 3900 0000 e204 0000 7800 0000  u16:9.......x...
000006e0: 0100 0000 0000 0000 7377 6170 7065 722f  ........swapper/
000006f0: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
00000700: d064 ef05 2f00 0103 0000 0000 7377 6170  .d../.......swap
00000710: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000720: 7800 0000 0000 0000 0000 0000 4469 7370  x...........Disp
00000730: 5379 6e63 0069 6e67 6572 0000 6f02 0000  Sync.inger..o...
00000740: 6100 0000 f07d 1b00 2f00 0103 6f02 0000  a....}../...o...
00000750: 4469 7370 5379 6e63 0069 6e67 6572 0000  DispSync.inger..
00000760: 6f02 0000 6100 0000 0100 0000 0000 0000  o...a...........
00000770: 6b73 6f66 7469 7271 642f 3000 0000 0000  ksoftirqd/0.....
00000780: 0300 0000 7800 0000 304c 2000 2f00 0103  ....x...0L ./...
00000790: 0300 0000 6b73 6f66 7469 7271 642f 3000  ....ksoftirqd/0.
000007a0: 0000 0000 0300 0000 7800 0000 0100 0000  ........x.......
000007b0: 0000 0000 6465 7832 6f61 7400 3935 5f33  ....dex2oat.95_3
000007c0: 0000 0000 341f 0000 8200 0000 700b 0700  ....4.......p...
000007d0: 2f00 0103 341f 0000 6465 7832 6f61 7400  /...4...dex2oat.
000007e0: 3935 5f33 0000 0000 341f 0000 8200 0000  95_3....4.......
000007f0: 0000 0000 0000 0000 7375 676f 763a 3000  ........sugov:0.
00000800: 0000 0000 0000 0000 3802 0000 3100 0000  ........8...1...
00000810: 50b0 0600 2f00 0103 3802 0000 7375 676f  P.../...8...sugo
00000820: 763a 3000 0000 0000 0000 0000 3802 0000  v:0.........8...
00000830: 3100 0000 0008 0000 0000 0000 6d69 6772  1...........migr
00000840: 6174 696f 6e2f 3000 0000 0000 0d00 0000  ation/0.........
00000850: 0000 0000 d09c 0600 2f00 0103 0d00 0000  ......../.......
00000860: 6d69 6772 6174 696f 6e2f 3000 0000 0000  migration/0.....
00000870: 0d00 0000 0000 0000 0100 0000 0000 0000  ................
00000880: 7375 676f 763a 3000 0000 0000 0000 0000  sugov:0.........
00000890: 3802 0000 3100 0000 7061 1900 2f00 0103  8...1...pa../...
000008a0: 3802 0000 7375 676f 763a 3000 0000 0000  8...sugov:0.....
000008b0: 0000 0000 3802 0000 3100 0000 0100 0000  ....8...1.......
000008c0: 0000 0000 6465 7832 6f61 7400 3935 5f33  ....dex2oat.95_3
000008d0: 0000 0000 341f 0000 8200 0000 f03c 5600  ....4........<V.
000008e0: 2f00 0103 341f 0000 6465 7832 6f61 7400  /...4...dex2oat.
000008f0: 3935 5f33 0000 0000 341f 0000 8200 0000  95_3....4.......
00000900: 0200 0000 0000 0000 7377 6170 7065 722f  ........swapper/
00000910: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
00000920: 5013 c400 2f00 0103 0000 0000 7377 6170  P.../.......swap
00000930: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000940: 7800 0000 0000 0000 0000 0000 616e 6472  x...........andr
00000950: 6f69 642e 6861 7264 7761 7200 d20a 0000  oid.hardwar.....
00000960: 7800 0000 30c9 1300 2f00 0103 d20a 0000  x...0.../.......
00000970: 616e 6472 6f69 642e 6861 7264 7761 7200  android.hardwar.
00000980: d20a 0000 7800 0000 0100 0000 0000 0000  ....x...........
00000990: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
000009a0: 0000 0000 7800 0000 7097 c000 2f00 0103  ....x...p.../...
000009b0: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
000009c0: 0000 0000 0000 0000 7800 0000 0000 0000  ........x.......
000009d0: 0000 0000 616e 6472 6f69 642e 6861 7264  ....android.hard
000009e0: 7761 7200 d20a 0000 7800 0000 305c 0c00  war.....x...0\..
000009f0: 2f00 0103 d20a 0000 616e 6472 6f69 642e  /.......android.
00000a00: 6861 7264 7761 7200 d20a 0000 7800 0000  hardwar.....x...
00000a10: 0100 0000 0000 0000 7377 6170 7065 722f  ........swapper/
00000a20: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
00000a30: d0aa 1401 2f00 0103 0000 0000 7377 6170  ..../.......swap
00000a40: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000a50: 7800 0000 0000 0000 0000 0000 616e 6472  x...........andr
00000a60: 6f69 642e 6861 7264 7761 7200 d20a 0000  oid.hardwar.....
00000a70: 7800 0000 903b 0c00 2f00 0103 d20a 0000  x....;../.......
00000a80: 616e 6472 6f69 642e 6861 7264 7761 7200  android.hardwar.
00000a90: d20a 0000 7800 0000 0100 0000 0000 0000  ....x...........
00000aa0: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000ab0: 0000 0000 7800 0000 f024 5401 2f00 0103  ....x....$T./...
00000ac0: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000ad0: 0000 0000 0000 0000 7800 0000 0000 0000  ........x.......
00000ae0: 0000 0000 616e 6472 6f69 642e 6861 7264  ....android.hard
00000af0: 7761 7200 d20a 0000 7800 0000 f0f3 0b00  war.....x.......
00000b00: 2f00 0103 d20a 0000 616e 6472 6f69 642e  /.......android.
00000b10: 6861 7264 7761 7200 d20a 0000 7800 0000  hardwar.....x...
00000b20: 0100 0000 0000 0000 7377 6170 7065 722f  ........swapper/
00000b30: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
00000b40: d0b5 bf02 2f00 0103 0000 0000 7377 6170  ..../.......swap
00000b50: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000b60: 7800 0000 0000 0000 0000 0000 4469 7370  x...........Disp
00000b70: 5379 6e63 0069 6e67 6572 0000 6f02 0000  Sync.inger..o...
00000b80: 6100 0000 90cd 1400 2f00 0103 6f02 0000  a......./...o...
00000b90: 4469 7370 5379 6e63 0069 6e67 6572 0000  DispSync.inger..
00000ba0: 6f02 0000 6100 0000 0100 0000 0000 0000  o...a...........
00000bb0: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000bc0: 0000 0000 7800 0000 50a6 1100 2f00 0103  ....x...P.../...
00000bd0: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000be0: 0000 0000 0000 0000 7800 0000 0000 0000  ........x.......
00000bf0: 0000 0000 7375 7266 6163 6566 6c69 6e67  ....surfacefling
00000c00: 6572 0000 4b02 0000 6200 0000 b04c 4200  er..K...b....LB.
00000c10: 2f00 0103 4b02 0000 7375 7266 6163 6566  /...K...surfacef
00000c20: 6c69 6e67 6572 0000 4b02 0000 6200 0000  linger..K...b...
00000c30: 0100 0000 0000 0000 7377 6170 7065 722f  ........swapper/
00000c40: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
00000c50: b025 060a 2f00 0103 0000 0000 7377 6170  .%../.......swap
00000c60: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000c70: 7800 0000 0000 0000 0000 0000 6b77 6f72  x...........kwor
00000c80: 6b65 722f 7531 363a 3600 0000 6401 0000  ker/u16:6...d...
00000c90: 7800 0000 d0b6 0600 2f00 0103 6401 0000  x......./...d...
00000ca0: 6b77 6f72 6b65 722f 7531 363a 3600 0000  kworker/u16:6...
00000cb0: 6401 0000 7800 0000 0100 0000 0000 0000  d...x...........
00000cc0: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000cd0: 0000 0000 7800 0000 f0a0 5800 2f00 0103  ....x.....X./...
00000ce0: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000cf0: 0000 0000 0000 0000 7800 0000 0000 0000  ........x.......
00000d00: 0000 0000 6b77 6f72 6b65 722f 7531 363a  ....kworker/u16:
00000d10: 3600 0000 6401 0000 7800 0000 f07a 1300  6...d...x....z..
00000d20: 2f00 0103 6401 0000 6b77 6f72 6b65 722f  /...d...kworker/
00000d30: 7531 363a 3600 0000 6401 0000 7800 0000  u16:6...d...x...
00000d40: 0100 0000 0000 0000 7377 6170 7065 722f  ........swapper/
00000d50: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
00000d60: b080 b101 2f00 0103 0000 0000 7377 6170  ..../.......swap
00000d70: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000d80: 7800 0000 0000 0000 0000 0000 6b77 6f72  x...........kwor
00000d90: 6b65 722f 7531 363a 3600 0000 6401 0000  ker/u16:6...d...
00000da0: 7800 0000 103c 1200 2f00 0103 6401 0000  x....<../...d...
00000db0: 6b77 6f72 6b65 722f 7531 363a 3600 0000  kworker/u16:6...
00000dc0: 6401 0000 7800 0000 0100 0000 0000 0000  d...x...........
00000dd0: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000de0: 0000 0000 7800 0000 50ea 3800 2f00 0103  ....x...P.8./...
00000df0: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000e00: 0000 0000 0000 0000 7800 0000 0000 0000  ........x.......
00000e10: 0000 0000 6b77 6f72 6b65 722f 7531 363a  ....kworker/u16:
00000e20: 3600 0000 6401 0000 7800 0000 5032 0400  6...d...x...P2..
00000e30: 2f00 0103 6401 0000 6b77 6f72 6b65 722f  /...d...kworker/
00000e40: 7531 363a 3600 0000 6401 0000 7800 0000  u16:6...d...x...
00000e50: 0100 0000 0000 0000 7377 6170 7065 722f  ........swapper/
00000e60: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
00000e70: 70f5 9000 2f00 0103 0000 0000 7377 6170  p.../.......swap
00000e80: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000e90: 7800 0000 0000 0000 0000 0000 6b77 6f72  x...........kwor
00000ea0: 6b65 722f 7531 363a 3600 0000 6401 0000  ker/u16:6...d...
00000eb0: 7800 0000 10d7 0300 2f00 0103 6401 0000  x......./...d...
00000ec0: 6b77 6f72 6b65 722f 7531 363a 3600 0000  kworker/u16:6...
00000ed0: 6401 0000 7800 0000 0100 0000 0000 0000  d...x...........
00000ee0: 7377 6170 7065 722f 3000 0000 0000 0000  swapper/0.......
00000ef0: 0000 0000 7800 0000 907c 0900 2f00 0103  ....x....|../...
00000f00: 0000 0000 7377 6170 7065 722f 3000 0000  ....swapper/0...
00000f10: 0000 0000 0000 0000 7800 0000 0000 0000  ........x.......
00000f20: 0000 0000 6b77 6f72 6b65 722f 7531 363a  ....kworker/u16:
00000f30: 3600 0000 6401 0000 7800 0000 7082 0300  6...d...x...p...
00000f40: 2f00 0103 6401 0000 6b77 6f72 6b65 722f  /...d...kworker/
00000f50: 7531 363a 3600 0000 6401 0000 7800 0000  u16:6...d...x...
00000f60: 0100 0000 0000 0000 7377 6170 7065 722f  ........swapper/
00000f70: 3000 0000 0000 0000 0000 0000 7800 0000  0...........x...
00000f80: f0ec 2100 2f00 0103 0000 0000 7377 6170  ..!./.......swap
00000f90: 7065 722f 3000 0000 0000 0000 0000 0000  per/0...........
00000fa0: 7800 0000 0000 0000 0000 0000 6b77 6f72  x...........kwor
00000fb0: 6b65 722f 7531 363a 3600 0000 6401 0000  ker/u16:6...d...
00000fc0: 7800 0000 0000 0000 0000 0000 0000 0000  x...............
00000fd0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000fe0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00000ff0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
    )",
};

// A page full of "ftrace/print" events.
ExamplePage g_full_page_print{
    "synthetic",
    R"(
00000000: df34 b6e2 e2a9 0000 e00f 0000 0000 0000  .4..............
00000010: 0700 0000 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000020: ffff ffff 3333 3333 0a00 0000 f348 0700  ....3333.....H..
00000030: 47d0 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000040: ffff ffff 3434 3434 0a00 722f 7531 3435  ....4444..r/u145
00000050: 07ab 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000060: ffff ffff 3535 3535 0a00 0700 6b77 6f72  ....5555....kwor
00000070: c7b0 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000080: ffff ffff 3636 3636 0a00 0100 4001 0103  ....6666....@...
00000090: a7b8 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000000a0: ffff ffff 3737 3737 0a00 0000 0100 0000  ....7777........
000000b0: 47ac 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
000000c0: ffff ffff 3838 3838 0a00 0000 f348 0700  ....8888.....H..
000000d0: 47d9 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
000000e0: ffff ffff 3939 3939 0a00 722f 7531 3435  ....9999..r/u145
000000f0: c7b9 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000100: ffff ffff 4141 4141 0a00 0700 6b77 6f72  ....AAAA....kwor
00000110: c7b6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000120: ffff ffff 4242 4242 0a00 0100 4001 0103  ....BBBB....@...
00000130: c7a6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000140: ffff ffff 4343 4343 0a00 0000 0100 0000  ....CCCC........
00000150: c7ab 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000160: ffff ffff 4444 4444 0a00 0000 f348 0700  ....DDDD.....H..
00000170: 27b3 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000180: ffff ffff 4545 4545 0a00 722f 7531 3435  ....EEEE..r/u145
00000190: 27b0 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
000001a0: ffff ffff 4646 4646 0a00 0700 6b77 6f72  ....FFFF....kwor
000001b0: c778 0c00 0500 0000 4f56 0700 84fc 9a9f  .x......OV......
000001c0: ffff ffff 3030 3030 0a00 0100 4001 0103  ....0000....@...
000001d0: 27b8 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
000001e0: ffff ffff 3131 3131 0a00 0000 0100 0000  ....1111........
000001f0: 27af 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000200: ffff ffff 3232 3232 0a00 0000 f348 0700  ....2222.....H..
00000210: 27a5 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000220: ffff ffff 3333 3333 0a00 722f 7531 3435  ....3333..r/u145
00000230: 07b7 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000240: ffff ffff 3434 3434 0a00 0700 6b77 6f72  ....4444....kwor
00000250: 47ac 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000260: ffff ffff 3535 3535 0a00 0100 4001 0103  ....5555....@...
00000270: 27ad 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000280: ffff ffff 3636 3636 0a00 0000 0100 0000  ....6666........
00000290: e7b6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000002a0: ffff ffff 3737 3737 0a00 0000 f348 0700  ....7777.....H..
000002b0: 47b1 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
000002c0: ffff ffff 3838 3838 0a00 722f 7531 3435  ....8888..r/u145
000002d0: a7a7 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000002e0: ffff ffff 3939 3939 0a00 0700 6b77 6f72  ....9999....kwor
000002f0: 87a9 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000300: ffff ffff 4141 4141 0a00 0100 4001 0103  ....AAAA....@...
00000310: 27cb 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000320: ffff ffff 4242 4242 0a00 0000 0100 0000  ....BBBB........
00000330: 47c0 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000340: ffff ffff 4343 4343 0a00 0000 f348 0700  ....CCCC.....H..
00000350: 67ae 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000360: ffff ffff 4444 4444 0a00 722f 7531 3435  ....DDDD..r/u145
00000370: 47b0 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000380: ffff ffff 4545 4545 0a00 0700 6b77 6f72  ....EEEE....kwor
00000390: 87a6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000003a0: ffff ffff 4646 4646 0a00 0100 4001 0103  ....FFFF....@...
000003b0: 07a2 0c00 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000003c0: ffff ffff 3030 3030 0a00 0000 0100 0000  ....0000........
000003d0: e71f 0a00 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000003e0: ffff ffff 3131 3131 0a00 0000 f348 0700  ....1111.....H..
000003f0: 67c6 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000400: ffff ffff 3232 3232 0a00 722f 7531 3435  ....2222..r/u145
00000410: 67b2 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000420: ffff ffff 3333 3333 0a00 0700 6b77 6f72  ....3333....kwor
00000430: 27af 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000440: ffff ffff 3434 3434 0a00 0100 4001 0103  ....4444....@...
00000450: 07d8 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000460: ffff ffff 3535 3535 0a00 0000 0100 0000  ....5555........
00000470: 87ba 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000480: ffff ffff 3636 3636 0a00 0000 f348 0700  ....6666.....H..
00000490: 27c3 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
000004a0: ffff ffff 3737 3737 0a00 722f 7531 3435  ....7777..r/u145
000004b0: 27bb 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
000004c0: ffff ffff 3838 3838 0a00 0700 6b77 6f72  ....8888....kwor
000004d0: c7a6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000004e0: ffff ffff 3939 3939 0a00 0100 4001 0103  ....9999....@...
000004f0: e7ac 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000500: ffff ffff 4141 4141 0a00 0000 0100 0000  ....AAAA........
00000510: e7a5 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000520: ffff ffff 4242 4242 0a00 0000 f348 0700  ....BBBB.....H..
00000530: c7ab 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000540: ffff ffff 4343 4343 0a00 722f 7531 3435  ....CCCC..r/u145
00000550: 07aa 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000560: ffff ffff 4444 4444 0a00 0700 6b77 6f72  ....DDDD....kwor
00000570: 27b1 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000580: ffff ffff 4545 4545 0a00 0100 4001 0103  ....EEEE....@...
00000590: e7ae 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000005a0: ffff ffff 4646 4646 0a00 0000 0100 0000  ....FFFF........
000005b0: 876e 0c00 0500 0000 4f56 0700 84fc 9a9f  .n......OV......
000005c0: ffff ffff 3030 3030 0a00 0000 f348 0700  ....0000.....H..
000005d0: 87b0 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000005e0: ffff ffff 3131 3131 0a00 722f 7531 3435  ....1111..r/u145
000005f0: a7de 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000600: ffff ffff 3232 3232 0a00 0700 6b77 6f72  ....2222....kwor
00000610: c7bf 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000620: ffff ffff 3333 3333 0a00 0100 4001 0103  ....3333....@...
00000630: e7ad 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000640: ffff ffff 3434 3434 0a00 0000 2e00 0000  ....4444........
00000650: 67c2 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000660: ffff ffff 3535 3535 0a00 0000 f348 0700  ....5555.....H..
00000670: 87ec 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000680: ffff ffff 3636 3636 0a00 722f 7531 3435  ....6666..r/u145
00000690: e7b9 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000006a0: ffff ffff 3737 3737 0a00 0700 6b77 6f72  ....7777....kwor
000006b0: e7aa 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000006c0: ffff ffff 3838 3838 0a00 0100 4001 0103  ....8888....@...
000006d0: 07a9 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000006e0: ffff ffff 3939 3939 0a00 0000 0100 0000  ....9999........
000006f0: 87a6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000700: ffff ffff 4141 4141 0a00 0000 f348 0700  ....AAAA.....H..
00000710: c7a6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000720: ffff ffff 4242 4242 0a00 722f 7531 3435  ....BBBB..r/u145
00000730: 67d0 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000740: ffff ffff 4343 4343 0a00 0700 6b77 6f72  ....CCCC....kwor
00000750: 27b6 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000760: ffff ffff 4444 4444 0a00 0100 4001 0103  ....DDDD....@...
00000770: e7ba 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000780: ffff ffff 4545 4545 0a00 0000 0100 0000  ....EEEE........
00000790: 07ce 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000007a0: ffff ffff 4646 4646 0a00 0000 f348 0700  ....FFFF.....H..
000007b0: c768 0c00 0500 0000 4f56 0700 84fc 9a9f  .h......OV......
000007c0: ffff ffff 3030 3030 0a00 722f 7531 3435  ....0000..r/u145
000007d0: 67de 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
000007e0: ffff ffff 3131 3131 0a00 0700 6b77 6f72  ....1111....kwor
000007f0: 27b9 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000800: ffff ffff 3232 3232 0a00 0100 4001 0103  ....2222....@...
00000810: c7b2 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000820: ffff ffff 3333 3333 0a00 0000 0100 0000  ....3333........
00000830: 67b1 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000840: ffff ffff 3434 3434 0a00 0000 f348 0700  ....4444.....H..
00000850: c7b2 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000860: ffff ffff 3535 3535 0a00 722f 7531 3435  ....5555..r/u145
00000870: c7c1 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000880: ffff ffff 3636 3636 0a00 0700 6b77 6f72  ....6666....kwor
00000890: 87ac 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000008a0: ffff ffff 3737 3737 0a00 0100 4001 0103  ....7777....@...
000008b0: c7b2 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000008c0: ffff ffff 3838 3838 0a00 0000 0100 0000  ....8888........
000008d0: 879e 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000008e0: ffff ffff 3939 3939 0a00 0000 f348 0700  ....9999.....H..
000008f0: 67ae 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000900: ffff ffff 4141 4141 0a00 722f 7531 3435  ....AAAA..r/u145
00000910: 07a6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000920: ffff ffff 4242 4242 0a00 0700 6b77 6f72  ....BBBB....kwor
00000930: a7a2 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000940: ffff ffff 4343 4343 0a00 0100 4001 0103  ....CCCC....@...
00000950: 67a4 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000960: ffff ffff 4444 4444 0a00 0000 0100 0000  ....DDDD........
00000970: 87a5 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000980: ffff ffff 4545 4545 0a00 0000 5417 0700  ....EEEE....T...
00000990: 47b1 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
000009a0: ffff ffff 4646 4646 0a00 722f 7531 3435  ....FFFF..r/u145
000009b0: e75e 0c00 0500 0000 4f56 0700 84fc 9a9f  .^......OV......
000009c0: ffff ffff 3030 3030 0a00 0700 6b77 6f72  ....0000....kwor
000009d0: a7bc 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
000009e0: ffff ffff 3131 3131 0a00 0100 4001 0103  ....1111....@...
000009f0: c7a8 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000a00: ffff ffff 3232 3232 0a00 0000 2e00 0000  ....2222........
00000a10: c7af 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000a20: ffff ffff 3333 3333 0a00 0000 5417 0700  ....3333....T...
00000a30: 67aa 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000a40: ffff ffff 3434 3434 0a00 722f 7531 3435  ....4444..r/u145
00000a50: 07bd 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000a60: ffff ffff 3535 3535 0a00 0700 6b77 6f72  ....5555....kwor
00000a70: 67b2 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000a80: ffff ffff 3636 3636 0a00 0100 4001 0103  ....6666....@...
00000a90: 27b2 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000aa0: ffff ffff 3737 3737 0a00 0000 2e00 0000  ....7777........
00000ab0: a731 0a00 0500 0000 4f56 0700 84fc 9a9f  .1......OV......
00000ac0: ffff ffff 3838 3838 0a00 0000 5417 0700  ....8888....T...
00000ad0: e7bd 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000ae0: ffff ffff 3939 3939 0a00 722f 7531 3435  ....9999..r/u145
00000af0: c7ad 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000b00: ffff ffff 4141 4141 0a00 0700 6b77 6f72  ....AAAA....kwor
00000b10: 47af 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000b20: ffff ffff 4242 4242 0a00 0100 4001 0103  ....BBBB....@...
00000b30: c7d2 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000b40: ffff ffff 4343 4343 0a00 0000 2e00 0000  ....CCCC........
00000b50: 67a6 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000b60: ffff ffff 4444 4444 0a00 0000 5417 0700  ....DDDD....T...
00000b70: 47aa 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000b80: ffff ffff 4545 4545 0a00 722f 7531 3435  ....EEEE..r/u145
00000b90: 07b6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000ba0: ffff ffff 4646 4646 0a00 0700 6b77 6f72  ....FFFF....kwor
00000bb0: 876b 0c00 0500 0000 4f56 0700 84fc 9a9f  .k......OV......
00000bc0: ffff ffff 3030 3030 0a00 0100 4001 0103  ....0000....@...
00000bd0: 47bc 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000be0: ffff ffff 3131 3131 0a00 0000 2e00 0000  ....1111........
00000bf0: a7b1 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000c00: ffff ffff 3232 3232 0a00 0000 5417 0700  ....2222....T...
00000c10: a7bb 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000c20: ffff ffff 3333 3333 0a00 722f 7531 3435  ....3333..r/u145
00000c30: 47b4 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000c40: ffff ffff 3434 3434 0a00 0700 6b77 6f72  ....4444....kwor
00000c50: 0710 0a00 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000c60: ffff ffff 3535 3535 0a00 0100 4001 0103  ....5555....@...
00000c70: 27cb 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000c80: ffff ffff 3636 3636 0a00 0000 2e00 0000  ....6666........
00000c90: c7a8 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000ca0: ffff ffff 3737 3737 0a00 0000 5417 0700  ....7777....T...
00000cb0: 47c6 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000cc0: ffff ffff 3838 3838 0a00 722f 7531 3435  ....8888..r/u145
00000cd0: 07c1 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000ce0: ffff ffff 3939 3939 0a00 0700 6b77 6f72  ....9999....kwor
00000cf0: 47b3 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000d00: ffff ffff 4141 4141 0a00 0300 4001 0103  ....AAAA....@...
00000d10: a7ad 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000d20: ffff ffff 4242 4242 0a00 0000 2e00 0000  ....BBBB........
00000d30: 87b3 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000d40: ffff ffff 4343 4343 0a00 0000 5417 0700  ....CCCC....T...
00000d50: 07b8 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000d60: ffff ffff 4444 4444 0a00 722f 7531 3435  ....DDDD..r/u145
00000d70: 07b6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000d80: ffff ffff 4545 4545 0a00 0700 6b77 6f72  ....EEEE....kwor
00000d90: 27b4 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000da0: ffff ffff 4646 4646 0a00 0100 4001 0103  ....FFFF....@...
00000db0: 2761 0c00 0500 0000 4f56 0700 84fc 9a9f  'a......OV......
00000dc0: ffff ffff 3030 3030 0a00 0000 2e00 0000  ....0000........
00000dd0: 87b6 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000de0: ffff ffff 3131 3131 0a00 0000 5417 0700  ....1111....T...
00000df0: 87ae 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000e00: ffff ffff 3232 3232 0a00 722f 7531 3435  ....2222..r/u145
00000e10: c7ac 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000e20: ffff ffff 3333 3333 0a00 0700 6b77 6f72  ....3333....kwor
00000e30: 27c0 0900 0500 0000 4f56 0700 84fc 9a9f  '.......OV......
00000e40: ffff ffff 3434 3434 0a00 0100 4001 0103  ....4444....@...
00000e50: 07b2 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000e60: ffff ffff 3535 3535 0a00 0000 2e00 0000  ....5555........
00000e70: 67ae 0900 0500 0000 4f56 0700 84fc 9a9f  g.......OV......
00000e80: ffff ffff 3636 3636 0a00 0000 5417 0700  ....6666....T...
00000e90: a7af 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000ea0: ffff ffff 3737 3737 0a00 722f 7531 3435  ....7777..r/u145
00000eb0: e7aa 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000ec0: ffff ffff 3838 3838 0a00 0700 6b77 6f72  ....8888....kwor
00000ed0: 07ae 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000ee0: ffff ffff 3939 3939 0a00 0100 4001 0103  ....9999....@...
00000ef0: 87af 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000f00: ffff ffff 4141 4141 0a00 0000 2e00 0000  ....AAAA........
00000f10: 07ae 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000f20: ffff ffff 4242 4242 0a00 0000 5417 0700  ....BBBB....T...
00000f30: 47a0 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000f40: ffff ffff 4343 4343 0a00 722f 7531 3435  ....CCCC..r/u145
00000f50: e7d5 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000f60: ffff ffff 4444 4444 0a00 0700 6b77 6f72  ....DDDD....kwor
00000f70: 87bc 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000f80: ffff ffff 4545 4545 0a00 0100 4001 0103  ....EEEE....@...
00000f90: e7c3 0900 0500 0000 4f56 0700 84fc 9a9f  ........OV......
00000fa0: ffff ffff 4646 4646 0a00 0000 2e00 0000  ....FFFF........
00000fb0: 475a 0c00 0500 0000 4f56 0700 84fc 9a9f  GZ......OV......
00000fc0: ffff ffff 3030 3030 0a00 0000 5417 0700  ....0000....T...
00000fd0: 47bc 0900 0500 0000 4f56 0700 84fc 9a9f  G.......OV......
00000fe0: ffff ffff 3131 3131 0a00 722f 7531 3435  ....1111..r/u145
00000ff0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
    )",
};

void DoParse(const ExamplePage& test_case,
             const std::vector<GroupAndName>& enabled_events,
             base::Optional<FtraceConfig::PrintFilter> print_filter,
             benchmark::State& state) {
  ScatteredStreamWriterNullDelegate delegate(base::kPageSize);
  ScatteredStreamWriter stream(&delegate);
  protozero::RootMessage<FtraceEventBundle> writer;

  ProtoTranslationTable* table = GetTable(test_case.name);
  auto page = PageFromXxd(test_case.data);

  FtraceDataSourceConfig ds_config{EventFilter{},
                                   EventFilter{},
                                   DisabledCompactSchedConfigForTesting(),
                                   base::nullopt,
                                   {},
                                   {},
                                   false /*symbolize_ksyms*/,
                                   false /*preserve_ftrace_buffer*/,
                                   {}};
  if (print_filter.has_value()) {
    ds_config.print_filter =
        FtracePrintFilterConfig::Create(print_filter.value(), table);
    PERFETTO_CHECK(ds_config.print_filter.has_value());
  }
  for (const GroupAndName& enabled_event : enabled_events) {
    ds_config.event_filter.AddEnabledEvent(
        table->EventToFtraceId(enabled_event));
  }

  FtraceMetadata metadata{};
  while (state.KeepRunning()) {
    writer.Reset(&stream);

    std::unique_ptr<CompactSchedBuffer> compact_buffer(
        new CompactSchedBuffer());
    const uint8_t* parse_pos = page.get();
    base::Optional<CpuReader::PageHeader> page_header =
        CpuReader::ParsePageHeader(&parse_pos, table->page_header_size_len());

    if (!page_header.has_value())
      return;

    CpuReader::ParsePagePayload(parse_pos, &page_header.value(), table,
                                &ds_config, compact_buffer.get(), &writer,
                                &metadata);

    metadata.Clear();
  }
}

void BM_ParsePageFullOfSchedSwitch(benchmark::State& state) {
  DoParse(g_full_page_sched_switch, {GroupAndName("sched", "sched_switch")},
          base::nullopt, state);
}
BENCHMARK(BM_ParsePageFullOfSchedSwitch);

void BM_ParsePageFullOfPrint(benchmark::State& state) {
  DoParse(g_full_page_print, {GroupAndName("ftrace", "print")}, base::nullopt,
          state);
}
BENCHMARK(BM_ParsePageFullOfPrint);

void BM_ParsePageFullOfPrintWithFilterRules(benchmark::State& state) {
  FtraceConfig::PrintFilter filter_conf;
  for (int i = 0; i < state.range(0); i++) {
    auto* rule = filter_conf.add_rules();
    rule->set_prefix("X");  // This rule will not match
    rule->set_allow(false);
  }
  DoParse(g_full_page_print, {GroupAndName("ftrace", "print")}, filter_conf,
          state);
}
BENCHMARK(BM_ParsePageFullOfPrintWithFilterRules)->DenseRange(0, 16, 1);

}  // namespace
}  // namespace perfetto
