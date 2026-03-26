#include <gtest/gtest.h>

#include <vector>

#include "common/mempool.h"

struct PoolObj {
  int v;
  PoolObj() : v(0) {}
  explicit PoolObj(int x) : v(x) {}
};

TEST(MemPoolTest, AllocateDeallocate) {
  MemPool<PoolObj> pool(2);
  PoolObj *a = pool.allocate(1);
  PoolObj *b = pool.allocate(2);

  EXPECT_EQ(a->v, 1);
  EXPECT_EQ(b->v, 2);

  pool.deallocate(a);
  pool.deallocate(b);
}

TEST(MemPoolTest, DetectsDoubleFree) {
  MemPool<PoolObj> pool(1);
  const PoolObj *const a = pool.allocate(1);
  pool.deallocate(a);
  EXPECT_THROW(pool.deallocate(a), std::invalid_argument);
}

TEST(MemPoolTest, GrowthKeepsIndexStable) {
  MemPool<PoolObj> pool(2);
  const PoolObj *const a = pool.allocate(10);
  const std::size_t a_idx = pool.getIndex(a);

  std::vector<PoolObj *> many;
  many.reserve(500);
  for (int i = 0; i < 500; ++i) {
    many.push_back(pool.allocate(i));
  }

  EXPECT_EQ(pool.getIndex(a), a_idx);
  EXPECT_EQ(pool.get(a_idx).v, 10);

  pool.deallocate(a);
  for (const PoolObj *ptr : many) {
    pool.deallocate(ptr);
  }
}

TEST(MemPoolTest, GetOutOfBoundsThrows) {
  MemPool<PoolObj> pool(2);
  pool.allocate(1);
  EXPECT_THROW(pool.get(1000), std::out_of_range);
}

TEST(MemPoolTest, DeallocateForeignPointerThrows) {
  MemPool<PoolObj> pool(2);
  PoolObj foreign(7);
  EXPECT_THROW(pool.deallocate(&foreign), std::invalid_argument);
}

TEST(MemPoolTest, ChurnAllocateDeallocate) {
  MemPool<PoolObj> pool(8);
  std::vector<PoolObj *> live;

  for (int round = 0; round < 100; ++round) {
    for (int i = 0; i < 128; ++i) {
      live.push_back(pool.allocate(round * 1000 + i));
    }

    for (std::size_t i = 0; i < live.size(); i += 2) {
      pool.deallocate(live[i]);
    }

    std::vector<PoolObj *> survivors;
    survivors.reserve(live.size() / 2 + 1);
    for (std::size_t i = 1; i < live.size(); i += 2) {
      survivors.push_back(live[i]);
    }
    live.swap(survivors);
  }

  for (const PoolObj *ptr : live) {
    pool.deallocate(ptr);
  }
}
