#ifndef GAMESTATE_H_
#define GAMESTATE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "net/sock/udp.h"
#include "net/gcoap.h"
#include "net/utils.h"
#include "custom_resources.h"
#include <stdbool.h>

#define MAX_RESOURCES (4)
#define RESOURCE_LENGTH (IPV6_ADDR_MAX_STR_LEN*2)

typedef struct GameState
{
	int my_id;
	bool ready_to_duel;

	uint32_t heartbeat_timer;
	bool is_lobby;
	char lobby_id[8];
	int ball_distance;

	bool in_game;

	int num_lobbies;
	char lobby_arr[MAX_RESOURCES][RESOURCE_LENGTH];

	bool rd_valid;
	char rd_path[64];
	sock_udp_ep_t rd_remote;

	char partner_uri[RESOURCE_LENGTH];
	sock_udp_ep_t partner_udp;
	char partner_start_duel_uri[RESOURCE_LENGTH];
	char partner_hit_happend_uri[RESOURCE_LENGTH];
	char partner_end_duel_uri[RESOURCE_LENGTH];
	char partner_lobby_id_uri[RESOURCE_LENGTH];
	char heartbeat_uri[RESOURCE_LENGTH];

	coap_resource_t my_resources[6];
	
} GameState;

extern GameState gs;

#ifdef __cplusplus
}
#endif

#endif // GAMESTATE_H_