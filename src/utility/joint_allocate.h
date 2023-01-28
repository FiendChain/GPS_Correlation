#pragma once

#include <stdint.h>
#include <new>
#include <assert.h>
#include "aligned_vector.h"
#include "span.h"

// Define the specification for each buffer inside the joint block
struct BufferParameters {
    size_t length;
    size_t alignment;
    // By default we align buffer to the size of the type
    BufferParameters(size_t _length, size_t _alignment=0)
    : length(_length), alignment(_alignment) {}
};

static AlignedVector<uint8_t> AllocateJoint(size_t curr_size, size_t align_size) {
    return AlignedVector<uint8_t>(curr_size, align_size);
}

template <typename T, typename ... Ts>
static AlignedVector<uint8_t> AllocateJoint(size_t curr_joint_size, size_t joint_alignment, tcb::span<T>& buf, BufferParameters params, Ts&& ... args) {
    // create padding so that next buffer has its alignment
    const auto align_elem_size = params.alignment ? params.alignment : sizeof(T);
    const size_t padded_joint_size = ((curr_joint_size+align_elem_size-1) / align_elem_size) * align_elem_size;

    // joint block must be aligned to the most demanding buffer alignment
    joint_alignment = (align_elem_size > joint_alignment) ? align_elem_size : joint_alignment;

    // get offset into our aligned and padded block
    const size_t buf_size = params.length*sizeof(T);
    const size_t new_joint_size = padded_joint_size + buf_size;
    auto data = AllocateJoint(new_joint_size, joint_alignment, args...);

    const auto data_offset = tcb::span(data).subspan(padded_joint_size, buf_size);

    // Check that alignment was met
    const auto addr_val = reinterpret_cast<uintptr_t>(data_offset.data());
    const bool is_aligned = (addr_val % align_elem_size) == 0;
    assert(is_aligned);

    buf = tcb::span<T>(reinterpret_cast<T*>(data_offset.data()), params.length);
    return data;
}

// Recursive template for creating a jointly allocated block 
template <typename T, typename ... Ts>
static AlignedVector<uint8_t> AllocateJoint(tcb::span<T>& buf, BufferParameters params, Ts&& ... args) {
    return AllocateJoint(0, 1, buf, params, args...);
}