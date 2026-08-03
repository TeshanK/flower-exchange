#include "../server/sequencer.h"
#include "../server/systypes.h"
#include <sep/protocol.h>
#include <common/types.h>
#include <gtest/gtest.h>

TEST(SequencerTest, SequenceNumberIncreaseByOne) {
    constexpr New_order new_order = {
        .header = {},
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 100,
    };

    Sequencer sequencer;

    const Order sequenced_order1 = sequencer.sequence(new_order);
    EXPECT_EQ(sequenced_order1.sequence_number, 1);

    const Order sequenced_order2 = sequencer.sequence(new_order);
    EXPECT_EQ(sequenced_order2.sequence_number, 2);

    const Order sequenced_order3 = sequencer.sequence(new_order);
    EXPECT_EQ(sequenced_order3.sequence_number, 3);

}