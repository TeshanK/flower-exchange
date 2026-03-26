#pragma once

#include "common/macros.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T> class MemPool final {
public:
    // Initializes pool with one chunk and a populated free list.
    explicit MemPool(std::size_t initial_capacity)
        : initial_capacity_(initial_capacity > 0 ? initial_capacity : 256) {
        addChunk(initial_capacity_);
    }

    // Allocates one object from free list, growing pool when exhausted.
    template <typename... Args> T* allocate(Args&&... args) {
        if (UNLIKELY(!free_list_head_)) {
            grow();
        }

        Node* block = free_list_head_;
        free_list_head_ = block->next;
        block->next = nullptr;
        block->is_free = false;

        T* ptr = &(block->obj);
        return new (ptr) T(std::forward<Args>(args)...);
    }

    // Returns stable global index for a pointer that belongs to this pool.
    std::size_t getIndex(const T* ptr) const {
        const auto* node = reinterpret_cast<const Node*>(ptr);
        for (std::size_t ci = 0; ci < chunks_.size(); ++ci) {
            const Node* base = chunks_[ci].data();
            if (LIKELY(node >= base && node < base + chunks_[ci].size())) {
                return chunk_offsets_[ci] + static_cast<std::size_t>(node - base);
            }
        }
        throw std::invalid_argument("Pointer not in this MemPool");
    }

    // Accesses object by stable index.
    T& get(std::size_t idx) {
        auto [ci, local] = resolveIndex(idx);
        return chunks_[ci][local].obj;
    }

    const T& get(std::size_t idx) const {
        auto [ci, local] = resolveIndex(idx);
        return chunks_[ci][local].obj;
    }

    // Deallocates object by pointer. Throws on invalid or double free.
    void deallocate(const T* ptr) { deallocateImpl(ptr); }

    // Deallocates object by stable index. Throws on invalid index/free.
    void deallocate(std::size_t idx) {
        auto [ci, local] = resolveIndex(idx);
        deallocateImpl(&(chunks_[ci][local].obj));
    }

private:
    struct Node {
        T obj;
        bool is_free{true};
        Node* next{nullptr};
    };

    // Resolves global index to chunk-local coordinate.
    [[nodiscard]] std::pair<std::size_t, std::size_t> resolveIndex(std::size_t idx) const {
        if (UNLIKELY(idx >= total_capacity_)) {
            throw std::out_of_range("MemPool index out of bounds");
        }

        std::size_t lo = 0;
        std::size_t hi = chunks_.size();
        while (lo + 1 < hi) {
            const std::size_t mid = lo + (hi - lo) / 2;
            if (chunk_offsets_[mid] <= idx) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        return {lo, idx - chunk_offsets_[lo]};
    }

    // Shared pointer deallocation logic with ownership checks.
    void deallocateImpl(const T* ptr) {
        if (UNLIKELY(!ptr)) {
            return;
        }

        static_assert(offsetof(Node, obj) == 0,
                      "MemPool assumes Node::obj is at offset 0");

        const auto ptr_addr = reinterpret_cast<std::uintptr_t>(ptr);
        bool found = false;
        Node* freed_node = nullptr;

        for (const auto& chunk : chunks_) {
            if (chunk.empty()) {
                continue;
            }

            const auto* base = chunk.data();
            const auto base_addr = reinterpret_cast<std::uintptr_t>(base);
            const auto end_addr = base_addr + chunk.size() * sizeof(Node);
            if (ptr_addr < base_addr || ptr_addr >= end_addr) {
                continue;
            }

            const auto byte_offset = ptr_addr - base_addr;
            if (byte_offset % sizeof(Node) != 0) {
                continue;
            }

            const auto idx = static_cast<std::size_t>(byte_offset / sizeof(Node));
            if (idx >= chunk.size()) {
                continue;
            }

            auto* candidate = const_cast<Node*>(base + idx);
            if (&(candidate->obj) != ptr) {
                continue;
            }

            freed_node = candidate;
                found = true;
                break;
        }

        if (UNLIKELY(!found)) {
            throw std::invalid_argument("Element not in this Memory pool");
        }

        if (UNLIKELY(freed_node->is_free)) {
            throw std::invalid_argument("Double free detected");
        }

        ptr->~T();
        freed_node->next = free_list_head_;
        free_list_head_ = freed_node;
        freed_node->is_free = true;
    }

    // Adds a new chunk and links all nodes into the free list.
    void addChunk(std::size_t count) {
        chunks_.emplace_back(count);
        chunk_offsets_.push_back(total_capacity_);
        auto& chunk = chunks_.back();

        for (std::size_t i = 0; i + 1 < count; ++i) {
            chunk[i].next = &chunk[i + 1];
        }
        chunk[count - 1].next = free_list_head_;
        free_list_head_ = &chunk[0];

        total_capacity_ += count;
    }

    // Doubles capacity growth curve up to a capped chunk size.
    void grow() {
        constexpr std::size_t kMaxChunkSize = 4'000'000;
        const std::size_t next = std::min(
            kMaxChunkSize,
            std::max<std::size_t>(initial_capacity_, total_capacity_));
        addChunk(next);
    }

    std::size_t initial_capacity_;
    std::size_t total_capacity_{0};
    std::vector<std::vector<Node>> chunks_;
    std::vector<std::size_t> chunk_offsets_;
    Node* free_list_head_{nullptr};
};
