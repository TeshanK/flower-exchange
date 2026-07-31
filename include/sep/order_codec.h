#pragma once

#include <sep/protocol.h>
#include <sep/tcp_connection.h>
#include <expected>

enum class Parse_error {
    connection_closed,
    invalid_message_type,
};

class OrderCodec
{
public:
    static auto decode_new_order(const TcpConnection& conn) -> std::expected<New_order, Parse_error>
    {
        New_order new_order;

        if(conn.read_exact(&new_order, sizeof(New_order)) < 0)
        {
            return std::unexpected(Parse_error::connection_closed);
        }

        if(new_order.header.message_type != Message_type::new_order)
        {
            return std::unexpected(Parse_error::invalid_message_type);
        }

        return new_order;
    }
};