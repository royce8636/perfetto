
#ifndef SRC_TRACE_PROCESSOR_METRICS_METRICS_DESCRIPTOR_H_
#define SRC_TRACE_PROCESSOR_METRICS_METRICS_DESCRIPTOR_H_

#include <stddef.h>
#include <stdint.h>

#include <array>

// This file was autogenerated by tools/gen_binary_descriptors. Do not edit.

// SHA1(tools/gen_binary_descriptors)
// 750d7d8f95621b45d4b6430d6f8808087a8702e6
// SHA1(protos/perfetto/metrics/metrics.proto)
// d4bda49b11630f3175699774d467819548947998

// This is the proto Metrics encoded as a ProtoFileDescriptor to allow
// for reflection without libprotobuf full/non-lite protos.

namespace perfetto {

constexpr std::array<uint8_t, 3089> kMetricsDescriptor{
    {0x0a, 0xc6, 0x0f, 0x0a, 0x29, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74,
     0x6f, 0x2f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x2f, 0x61, 0x6e,
     0x64, 0x72, 0x6f, 0x69, 0x64, 0x2f, 0x6d, 0x65, 0x6d, 0x5f, 0x6d, 0x65,
     0x74, 0x72, 0x69, 0x63, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x12, 0x0f,
     0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f,
     0x74, 0x6f, 0x73, 0x22, 0x83, 0x0f, 0x0a, 0x13, 0x41, 0x6e, 0x64, 0x72,
     0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65, 0x74,
     0x72, 0x69, 0x63, 0x12, 0x59, 0x0a, 0x0e, 0x73, 0x79, 0x73, 0x74, 0x65,
     0x6d, 0x5f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x18, 0x04, 0x20,
     0x01, 0x28, 0x0b, 0x32, 0x32, 0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74,
     0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e,
     0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d,
     0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d,
     0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x52, 0x0d, 0x73, 0x79, 0x73,
     0x74, 0x65, 0x6d, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x12, 0x5c,
     0x0a, 0x0f, 0x70, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x5f, 0x6d, 0x65,
     0x74, 0x72, 0x69, 0x63, 0x73, 0x18, 0x03, 0x20, 0x03, 0x28, 0x0b, 0x32,
     0x33, 0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70,
     0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69,
     0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65, 0x74, 0x72, 0x69,
     0x63, 0x2e, 0x50, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x4d, 0x65, 0x74,
     0x72, 0x69, 0x63, 0x73, 0x52, 0x0e, 0x70, 0x72, 0x6f, 0x63, 0x65, 0x73,
     0x73, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x1a, 0xd0, 0x02, 0x0a,
     0x0d, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x4d, 0x65, 0x74, 0x72, 0x69,
     0x63, 0x73, 0x12, 0x4b, 0x0a, 0x0a, 0x61, 0x6e, 0x6f, 0x6e, 0x5f, 0x70,
     0x61, 0x67, 0x65, 0x73, 0x18, 0x01, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x2c,
     0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72,
     0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64,
     0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63,
     0x2e, 0x43, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x52, 0x09, 0x61, 0x6e,
     0x6f, 0x6e, 0x50, 0x61, 0x67, 0x65, 0x73, 0x12, 0x4f, 0x0a, 0x0c, 0x6d,
     0x6d, 0x61, 0x70, 0x65, 0x64, 0x5f, 0x70, 0x61, 0x67, 0x65, 0x73, 0x18,
     0x02, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x2c, 0x2e, 0x70, 0x65, 0x72, 0x66,
     0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e,
     0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72,
     0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x43, 0x6f, 0x75, 0x6e,
     0x74, 0x65, 0x72, 0x52, 0x0b, 0x6d, 0x6d, 0x61, 0x70, 0x65, 0x64, 0x50,
     0x61, 0x67, 0x65, 0x73, 0x12, 0x52, 0x0a, 0x0b, 0x69, 0x6f, 0x6e, 0x5f,
     0x62, 0x75, 0x66, 0x66, 0x65, 0x72, 0x73, 0x18, 0x04, 0x20, 0x03, 0x28,
     0x0b, 0x32, 0x31, 0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f,
     0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64, 0x72,
     0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65, 0x74,
     0x72, 0x69, 0x63, 0x2e, 0x4e, 0x61, 0x6d, 0x65, 0x64, 0x43, 0x6f, 0x75,
     0x6e, 0x74, 0x65, 0x72, 0x52, 0x0a, 0x69, 0x6f, 0x6e, 0x42, 0x75, 0x66,
     0x66, 0x65, 0x72, 0x73, 0x12, 0x47, 0x0a, 0x04, 0x6c, 0x6d, 0x6b, 0x73,
     0x18, 0x05, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x33, 0x2e, 0x70, 0x65, 0x72,
     0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73,
     0x2e, 0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f,
     0x72, 0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x4c, 0x6f, 0x77,
     0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4b, 0x69, 0x6c, 0x6c, 0x73, 0x52,
     0x04, 0x6c, 0x6d, 0x6b, 0x73, 0x4a, 0x04, 0x08, 0x03, 0x10, 0x04, 0x1a,
     0xef, 0x01, 0x0a, 0x0e, 0x4c, 0x6f, 0x77, 0x4d, 0x65, 0x6d, 0x6f, 0x72,
     0x79, 0x4b, 0x69, 0x6c, 0x6c, 0x73, 0x12, 0x1f, 0x0a, 0x0b, 0x74, 0x6f,
     0x74, 0x61, 0x6c, 0x5f, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x18, 0x01, 0x20,
     0x01, 0x28, 0x05, 0x52, 0x0a, 0x74, 0x6f, 0x74, 0x61, 0x6c, 0x43, 0x6f,
     0x75, 0x6e, 0x74, 0x12, 0x68, 0x0a, 0x09, 0x62, 0x72, 0x65, 0x61, 0x6b,
     0x64, 0x6f, 0x77, 0x6e, 0x18, 0x02, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x4a,
     0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72,
     0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64,
     0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63,
     0x2e, 0x4c, 0x6f, 0x77, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4b, 0x69,
     0x6c, 0x6c, 0x73, 0x2e, 0x4c, 0x6f, 0x77, 0x4d, 0x65, 0x6d, 0x6f, 0x72,
     0x79, 0x4b, 0x69, 0x6c, 0x6c, 0x42, 0x72, 0x65, 0x61, 0x6b, 0x64, 0x6f,
     0x77, 0x6e, 0x52, 0x09, 0x62, 0x72, 0x65, 0x61, 0x6b, 0x64, 0x6f, 0x77,
     0x6e, 0x1a, 0x52, 0x0a, 0x16, 0x4c, 0x6f, 0x77, 0x4d, 0x65, 0x6d, 0x6f,
     0x72, 0x79, 0x4b, 0x69, 0x6c, 0x6c, 0x42, 0x72, 0x65, 0x61, 0x6b, 0x64,
     0x6f, 0x77, 0x6e, 0x12, 0x22, 0x0a, 0x0d, 0x6f, 0x6f, 0x6d, 0x5f, 0x73,
     0x63, 0x6f, 0x72, 0x65, 0x5f, 0x61, 0x64, 0x6a, 0x18, 0x01, 0x20, 0x01,
     0x28, 0x05, 0x52, 0x0b, 0x6f, 0x6f, 0x6d, 0x53, 0x63, 0x6f, 0x72, 0x65,
     0x41, 0x64, 0x6a, 0x12, 0x14, 0x0a, 0x05, 0x63, 0x6f, 0x75, 0x6e, 0x74,
     0x18, 0x02, 0x20, 0x01, 0x28, 0x05, 0x52, 0x05, 0x63, 0x6f, 0x75, 0x6e,
     0x74, 0x1a, 0x88, 0x04, 0x0a, 0x0e, 0x50, 0x72, 0x6f, 0x63, 0x65, 0x73,
     0x73, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x12, 0x21, 0x0a, 0x0c,
     0x70, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x5f, 0x6e, 0x61, 0x6d, 0x65,
     0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0b, 0x70, 0x72, 0x6f, 0x63,
     0x65, 0x73, 0x73, 0x4e, 0x61, 0x6d, 0x65, 0x12, 0x65, 0x0a, 0x10, 0x6f,
     0x76, 0x65, 0x72, 0x61, 0x6c, 0x6c, 0x5f, 0x63, 0x6f, 0x75, 0x6e, 0x74,
     0x65, 0x72, 0x73, 0x18, 0x0a, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x3a, 0x2e,
     0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f,
     0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d,
     0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e,
     0x50, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x4d, 0x65, 0x6d, 0x6f, 0x72,
     0x79, 0x43, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x73, 0x52, 0x0f, 0x6f,
     0x76, 0x65, 0x72, 0x61, 0x6c, 0x6c, 0x43, 0x6f, 0x75, 0x6e, 0x74, 0x65,
     0x72, 0x73, 0x12, 0x69, 0x0a, 0x09, 0x62, 0x72, 0x65, 0x61, 0x6b, 0x64,
     0x6f, 0x77, 0x6e, 0x18, 0x0b, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x4b, 0x2e,
     0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f,
     0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d,
     0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e,
     0x50, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x4d, 0x65, 0x74, 0x72, 0x69,
     0x63, 0x73, 0x2e, 0x50, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x4d, 0x65,
     0x74, 0x72, 0x69, 0x63, 0x73, 0x42, 0x72, 0x65, 0x61, 0x6b, 0x64, 0x6f,
     0x77, 0x6e, 0x52, 0x09, 0x62, 0x72, 0x65, 0x61, 0x6b, 0x64, 0x6f, 0x77,
     0x6e, 0x12, 0x50, 0x0a, 0x0b, 0x61, 0x6e, 0x6f, 0x6e, 0x5f, 0x67, 0x72,
     0x6f, 0x77, 0x74, 0x68, 0x18, 0x0d, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x2f,
     0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72,
     0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64,
     0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63,
     0x2e, 0x53, 0x70, 0x61, 0x6e, 0x47, 0x72, 0x6f, 0x77, 0x74, 0x68, 0x52,
     0x0a, 0x61, 0x6e, 0x6f, 0x6e, 0x47, 0x72, 0x6f, 0x77, 0x74, 0x68, 0x1a,
     0xa2, 0x01, 0x0a, 0x17, 0x50, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x4d,
     0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x42, 0x72, 0x65, 0x61, 0x6b, 0x64,
     0x6f, 0x77, 0x6e, 0x12, 0x29, 0x0a, 0x10, 0x70, 0x72, 0x6f, 0x63, 0x65,
     0x73, 0x73, 0x5f, 0x70, 0x72, 0x69, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x18,
     0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0f, 0x70, 0x72, 0x6f, 0x63, 0x65,
     0x73, 0x73, 0x50, 0x72, 0x69, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x12, 0x56,
     0x0a, 0x08, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x73, 0x18, 0x02,
     0x20, 0x01, 0x28, 0x0b, 0x32, 0x3a, 0x2e, 0x70, 0x65, 0x72, 0x66, 0x65,
     0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41,
     0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79,
     0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x50, 0x72, 0x6f, 0x63, 0x65,
     0x73, 0x73, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x43, 0x6f, 0x75, 0x6e,
     0x74, 0x65, 0x72, 0x73, 0x52, 0x08, 0x63, 0x6f, 0x75, 0x6e, 0x74, 0x65,
     0x72, 0x73, 0x4a, 0x04, 0x08, 0x01, 0x10, 0x02, 0x2a, 0x04, 0x08, 0x0c,
     0x10, 0x0d, 0x4a, 0x04, 0x08, 0x02, 0x10, 0x0a, 0x1a, 0xbd, 0x02, 0x0a,
     0x15, 0x50, 0x72, 0x6f, 0x63, 0x65, 0x73, 0x73, 0x4d, 0x65, 0x6d, 0x6f,
     0x72, 0x79, 0x43, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x73, 0x12, 0x47,
     0x0a, 0x08, 0x61, 0x6e, 0x6f, 0x6e, 0x5f, 0x72, 0x73, 0x73, 0x18, 0x01,
     0x20, 0x01, 0x28, 0x0b, 0x32, 0x2c, 0x2e, 0x70, 0x65, 0x72, 0x66, 0x65,
     0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41,
     0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79,
     0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x43, 0x6f, 0x75, 0x6e, 0x74,
     0x65, 0x72, 0x52, 0x07, 0x61, 0x6e, 0x6f, 0x6e, 0x52, 0x73, 0x73, 0x12,
     0x47, 0x0a, 0x08, 0x66, 0x69, 0x6c, 0x65, 0x5f, 0x72, 0x73, 0x73, 0x18,
     0x02, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x2c, 0x2e, 0x70, 0x65, 0x72, 0x66,
     0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e,
     0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72,
     0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x43, 0x6f, 0x75, 0x6e,
     0x74, 0x65, 0x72, 0x52, 0x07, 0x66, 0x69, 0x6c, 0x65, 0x52, 0x73, 0x73,
     0x12, 0x40, 0x0a, 0x04, 0x73, 0x77, 0x61, 0x70, 0x18, 0x03, 0x20, 0x01,
     0x28, 0x0b, 0x32, 0x2c, 0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74,
     0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64,
     0x72, 0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65,
     0x74, 0x72, 0x69, 0x63, 0x2e, 0x43, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72,
     0x52, 0x04, 0x73, 0x77, 0x61, 0x70, 0x12, 0x50, 0x0a, 0x0d, 0x61, 0x6e,
     0x6f, 0x6e, 0x5f, 0x61, 0x6e, 0x64, 0x5f, 0x73, 0x77, 0x61, 0x70, 0x18,
     0x04, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x2c, 0x2e, 0x70, 0x65, 0x72, 0x66,
     0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e,
     0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72,
     0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x43, 0x6f, 0x75, 0x6e,
     0x74, 0x65, 0x72, 0x52, 0x0b, 0x61, 0x6e, 0x6f, 0x6e, 0x41, 0x6e, 0x64,
     0x53, 0x77, 0x61, 0x70, 0x1a, 0x6a, 0x0a, 0x0c, 0x4e, 0x61, 0x6d, 0x65,
     0x64, 0x43, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x12, 0x12, 0x0a, 0x04,
     0x6e, 0x61, 0x6d, 0x65, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x04,
     0x6e, 0x61, 0x6d, 0x65, 0x12, 0x46, 0x0a, 0x07, 0x63, 0x6f, 0x75, 0x6e,
     0x74, 0x65, 0x72, 0x18, 0x02, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x2c, 0x2e,
     0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f,
     0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d,
     0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e,
     0x43, 0x6f, 0x75, 0x6e, 0x74, 0x65, 0x72, 0x52, 0x07, 0x63, 0x6f, 0x75,
     0x6e, 0x74, 0x65, 0x72, 0x1a, 0x3f, 0x0a, 0x07, 0x43, 0x6f, 0x75, 0x6e,
     0x74, 0x65, 0x72, 0x12, 0x10, 0x0a, 0x03, 0x6d, 0x69, 0x6e, 0x18, 0x01,
     0x20, 0x01, 0x28, 0x01, 0x52, 0x03, 0x6d, 0x69, 0x6e, 0x12, 0x10, 0x0a,
     0x03, 0x6d, 0x61, 0x78, 0x18, 0x02, 0x20, 0x01, 0x28, 0x01, 0x52, 0x03,
     0x6d, 0x61, 0x78, 0x12, 0x10, 0x0a, 0x03, 0x61, 0x76, 0x67, 0x18, 0x03,
     0x20, 0x01, 0x28, 0x01, 0x52, 0x03, 0x61, 0x76, 0x67, 0x1a, 0x76, 0x0a,
     0x0a, 0x53, 0x70, 0x61, 0x6e, 0x47, 0x72, 0x6f, 0x77, 0x74, 0x68, 0x12,
     0x1b, 0x0a, 0x09, 0x73, 0x74, 0x61, 0x72, 0x74, 0x5f, 0x76, 0x61, 0x6c,
     0x18, 0x01, 0x20, 0x01, 0x28, 0x01, 0x52, 0x08, 0x73, 0x74, 0x61, 0x72,
     0x74, 0x56, 0x61, 0x6c, 0x12, 0x17, 0x0a, 0x07, 0x65, 0x6e, 0x64, 0x5f,
     0x76, 0x61, 0x6c, 0x18, 0x02, 0x20, 0x01, 0x28, 0x01, 0x52, 0x06, 0x65,
     0x6e, 0x64, 0x56, 0x61, 0x6c, 0x12, 0x1a, 0x0a, 0x08, 0x64, 0x75, 0x72,
     0x61, 0x74, 0x69, 0x6f, 0x6e, 0x18, 0x03, 0x20, 0x01, 0x28, 0x03, 0x52,
     0x08, 0x64, 0x75, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x12, 0x16, 0x0a,
     0x06, 0x67, 0x72, 0x6f, 0x77, 0x74, 0x68, 0x18, 0x04, 0x20, 0x01, 0x28,
     0x01, 0x52, 0x06, 0x67, 0x72, 0x6f, 0x77, 0x74, 0x68, 0x42, 0x02, 0x48,
     0x03, 0x0a, 0x8b, 0x06, 0x0a, 0x2d, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74,
     0x74, 0x6f, 0x2f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x2f, 0x61,
     0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x2f, 0x73, 0x74, 0x61, 0x72, 0x74,
     0x75, 0x70, 0x5f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x70, 0x72,
     0x6f, 0x74, 0x6f, 0x12, 0x0f, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74,
     0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x22, 0xe0, 0x01, 0x0a,
     0x12, 0x54, 0x61, 0x73, 0x6b, 0x53, 0x74, 0x61, 0x74, 0x65, 0x42, 0x72,
     0x65, 0x61, 0x6b, 0x64, 0x6f, 0x77, 0x6e, 0x12, 0x24, 0x0a, 0x0e, 0x72,
     0x75, 0x6e, 0x6e, 0x69, 0x6e, 0x67, 0x5f, 0x64, 0x75, 0x72, 0x5f, 0x6e,
     0x73, 0x18, 0x01, 0x20, 0x01, 0x28, 0x03, 0x52, 0x0c, 0x72, 0x75, 0x6e,
     0x6e, 0x69, 0x6e, 0x67, 0x44, 0x75, 0x72, 0x4e, 0x73, 0x12, 0x26, 0x0a,
     0x0f, 0x72, 0x75, 0x6e, 0x6e, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x64, 0x75,
     0x72, 0x5f, 0x6e, 0x73, 0x18, 0x02, 0x20, 0x01, 0x28, 0x03, 0x52, 0x0d,
     0x72, 0x75, 0x6e, 0x6e, 0x61, 0x62, 0x6c, 0x65, 0x44, 0x75, 0x72, 0x4e,
     0x73, 0x12, 0x3f, 0x0a, 0x1c, 0x75, 0x6e, 0x69, 0x6e, 0x74, 0x65, 0x72,
     0x72, 0x75, 0x70, 0x74, 0x69, 0x62, 0x6c, 0x65, 0x5f, 0x73, 0x6c, 0x65,
     0x65, 0x70, 0x5f, 0x64, 0x75, 0x72, 0x5f, 0x6e, 0x73, 0x18, 0x03, 0x20,
     0x01, 0x28, 0x03, 0x52, 0x19, 0x75, 0x6e, 0x69, 0x6e, 0x74, 0x65, 0x72,
     0x72, 0x75, 0x70, 0x74, 0x69, 0x62, 0x6c, 0x65, 0x53, 0x6c, 0x65, 0x65,
     0x70, 0x44, 0x75, 0x72, 0x4e, 0x73, 0x12, 0x3b, 0x0a, 0x1a, 0x69, 0x6e,
     0x74, 0x65, 0x72, 0x72, 0x75, 0x70, 0x74, 0x69, 0x62, 0x6c, 0x65, 0x5f,
     0x73, 0x6c, 0x65, 0x65, 0x70, 0x5f, 0x64, 0x75, 0x72, 0x5f, 0x6e, 0x73,
     0x18, 0x04, 0x20, 0x01, 0x28, 0x03, 0x52, 0x17, 0x69, 0x6e, 0x74, 0x65,
     0x72, 0x72, 0x75, 0x70, 0x74, 0x69, 0x62, 0x6c, 0x65, 0x53, 0x6c, 0x65,
     0x65, 0x70, 0x44, 0x75, 0x72, 0x4e, 0x73, 0x22, 0xe1, 0x03, 0x0a, 0x14,
     0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x53, 0x74, 0x61, 0x72, 0x74,
     0x75, 0x70, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x12, 0x47, 0x0a, 0x07,
     0x73, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70, 0x18, 0x01, 0x20, 0x03, 0x28,
     0x0b, 0x32, 0x2d, 0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f,
     0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e, 0x64, 0x72,
     0x6f, 0x69, 0x64, 0x53, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70, 0x4d, 0x65,
     0x74, 0x72, 0x69, 0x63, 0x2e, 0x53, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70,
     0x52, 0x07, 0x73, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70, 0x1a, 0x84, 0x01,
     0x0a, 0x0c, 0x54, 0x6f, 0x46, 0x69, 0x72, 0x73, 0x74, 0x46, 0x72, 0x61,
     0x6d, 0x65, 0x12, 0x15, 0x0a, 0x06, 0x64, 0x75, 0x72, 0x5f, 0x6e, 0x73,
     0x18, 0x01, 0x20, 0x01, 0x28, 0x03, 0x52, 0x05, 0x64, 0x75, 0x72, 0x4e,
     0x73, 0x12, 0x5d, 0x0a, 0x19, 0x6d, 0x61, 0x69, 0x6e, 0x5f, 0x74, 0x68,
     0x72, 0x65, 0x61, 0x64, 0x5f, 0x62, 0x79, 0x5f, 0x74, 0x61, 0x73, 0x6b,
     0x5f, 0x73, 0x74, 0x61, 0x74, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x0b,
     0x32, 0x23, 0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e,
     0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x54, 0x61, 0x73, 0x6b, 0x53,
     0x74, 0x61, 0x74, 0x65, 0x42, 0x72, 0x65, 0x61, 0x6b, 0x64, 0x6f, 0x77,
     0x6e, 0x52, 0x15, 0x6d, 0x61, 0x69, 0x6e, 0x54, 0x68, 0x72, 0x65, 0x61,
     0x64, 0x42, 0x79, 0x54, 0x61, 0x73, 0x6b, 0x53, 0x74, 0x61, 0x74, 0x65,
     0x1a, 0xf8, 0x01, 0x0a, 0x07, 0x53, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70,
     0x12, 0x1d, 0x0a, 0x0a, 0x73, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70, 0x5f,
     0x69, 0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x0d, 0x52, 0x09, 0x73, 0x74,
     0x61, 0x72, 0x74, 0x75, 0x70, 0x49, 0x64, 0x12, 0x21, 0x0a, 0x0c, 0x70,
     0x61, 0x63, 0x6b, 0x61, 0x67, 0x65, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x18,
     0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0b, 0x70, 0x61, 0x63, 0x6b, 0x61,
     0x67, 0x65, 0x4e, 0x61, 0x6d, 0x65, 0x12, 0x21, 0x0a, 0x0c, 0x70, 0x72,
     0x6f, 0x63, 0x65, 0x73, 0x73, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x18, 0x03,
     0x20, 0x01, 0x28, 0x09, 0x52, 0x0b, 0x70, 0x72, 0x6f, 0x63, 0x65, 0x73,
     0x73, 0x4e, 0x61, 0x6d, 0x65, 0x12, 0x2e, 0x0a, 0x13, 0x73, 0x74, 0x61,
     0x72, 0x74, 0x65, 0x64, 0x5f, 0x6e, 0x65, 0x77, 0x5f, 0x70, 0x72, 0x6f,
     0x63, 0x65, 0x73, 0x73, 0x18, 0x04, 0x20, 0x01, 0x28, 0x08, 0x52, 0x11,
     0x73, 0x74, 0x61, 0x72, 0x74, 0x65, 0x64, 0x4e, 0x65, 0x77, 0x50, 0x72,
     0x6f, 0x63, 0x65, 0x73, 0x73, 0x12, 0x58, 0x0a, 0x0e, 0x74, 0x6f, 0x5f,
     0x66, 0x69, 0x72, 0x73, 0x74, 0x5f, 0x66, 0x72, 0x61, 0x6d, 0x65, 0x18,
     0x05, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x32, 0x2e, 0x70, 0x65, 0x72, 0x66,
     0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e,
     0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x53, 0x74, 0x61, 0x72, 0x74,
     0x75, 0x70, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x54, 0x6f, 0x46,
     0x69, 0x72, 0x73, 0x74, 0x46, 0x72, 0x61, 0x6d, 0x65, 0x52, 0x0c, 0x74,
     0x6f, 0x46, 0x69, 0x72, 0x73, 0x74, 0x46, 0x72, 0x61, 0x6d, 0x65, 0x42,
     0x02, 0x48, 0x03, 0x0a, 0xb7, 0x02, 0x0a, 0x1e, 0x70, 0x65, 0x72, 0x66,
     0x65, 0x74, 0x74, 0x6f, 0x2f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73,
     0x2f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x2e, 0x70, 0x72, 0x6f,
     0x74, 0x6f, 0x12, 0x0f, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f,
     0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x1a, 0x29, 0x70, 0x65, 0x72,
     0x66, 0x65, 0x74, 0x74, 0x6f, 0x2f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63,
     0x73, 0x2f, 0x61, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x2f, 0x6d, 0x65,
     0x6d, 0x5f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x70, 0x72, 0x6f,
     0x74, 0x6f, 0x1a, 0x2d, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74, 0x74, 0x6f,
     0x2f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x2f, 0x61, 0x6e, 0x64,
     0x72, 0x6f, 0x69, 0x64, 0x2f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70,
     0x5f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x70, 0x72, 0x6f, 0x74,
     0x6f, 0x22, 0xa5, 0x01, 0x0a, 0x0c, 0x54, 0x72, 0x61, 0x63, 0x65, 0x4d,
     0x65, 0x74, 0x72, 0x69, 0x63, 0x73, 0x12, 0x45, 0x0a, 0x0b, 0x61, 0x6e,
     0x64, 0x72, 0x6f, 0x69, 0x64, 0x5f, 0x6d, 0x65, 0x6d, 0x18, 0x01, 0x20,
     0x01, 0x28, 0x0b, 0x32, 0x24, 0x2e, 0x70, 0x65, 0x72, 0x66, 0x65, 0x74,
     0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73, 0x2e, 0x41, 0x6e,
     0x64, 0x72, 0x6f, 0x69, 0x64, 0x4d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x4d,
     0x65, 0x74, 0x72, 0x69, 0x63, 0x52, 0x0a, 0x61, 0x6e, 0x64, 0x72, 0x6f,
     0x69, 0x64, 0x4d, 0x65, 0x6d, 0x12, 0x4e, 0x0a, 0x0f, 0x61, 0x6e, 0x64,
     0x72, 0x6f, 0x69, 0x64, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x75, 0x70,
     0x18, 0x02, 0x20, 0x01, 0x28, 0x0b, 0x32, 0x25, 0x2e, 0x70, 0x65, 0x72,
     0x66, 0x65, 0x74, 0x74, 0x6f, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x73,
     0x2e, 0x41, 0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x53, 0x74, 0x61, 0x72,
     0x74, 0x75, 0x70, 0x4d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x52, 0x0e, 0x61,
     0x6e, 0x64, 0x72, 0x6f, 0x69, 0x64, 0x53, 0x74, 0x61, 0x72, 0x74, 0x75,
     0x70, 0x42, 0x02, 0x48, 0x03}};

}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_METRICS_METRICS_DESCRIPTOR_H_
