cmake_minimum_required(VERSION 2.8)

include_directories(include)

set(BEDROCK_SOURCES
./src/packet/packet_entity_action.c
./src/packet/packet_keep_alive.c
./src/packet/packet_player_look.c
./src/packet/packet_column_allocation.c
./src/packet/packet_entity_head_look.c
./src/packet/packet_login_request.c
./src/packet/packet_entity_relative_move.c
./src/packet/packet.c
./src/packet/packet_chat_message.c
./src/packet/packet_held_item_change.c
./src/packet/packet_list_ping.c
./src/packet/packet_entity_equipment.c
./src/packet/packet_entity_look_and_relative_move.c
./src/packet/packet_disconnect.c
./src/packet/packet_spawn_named_entity.c
./src/packet/packet_set_slot.c
./src/packet/packet_position_and_look.c
./src/packet/packet_column.c
./src/packet/packet_handshake.c
./src/packet/packet_position.c
./src/packet/packet_entity_metadata.c
./src/packet/packet_player_list_item.c
./src/packet/packet_player.c
./src/packet/packet_destroy_entity.c
./src/packet/packet_entity_teleport.c
./src/packet/packet_close_window.c
./src/packet/packet_spawn_point.c
./src/nbt/nbt.c
./src/util/endian.c
./src/util/string.c
./src/util/timer.c
./src/util/memory.c
./src/util/list.c
./src/util/buffer.c
./src/util/fd.c
./src/server/region.c
./src/server/column.c
./src/server/listener.c
./src/server/world.c
./src/server/chunk.c
./src/server/bedrock.c
./src/server/client.c
./src/compression/compression.c
./src/io/epoll.c
./src/blocks/items.c
)

set(CMAKE_C_FLAGS "-g -pipe -Wall -Wextra -std=gnu99 -pedantic")

add_executable(bedrock ${BEDROCK_SOURCES})
target_link_libraries(bedrock z m rt)

