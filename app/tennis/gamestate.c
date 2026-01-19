#include "gamestate.h"

GameState gs =
{
	.ready_to_duel = true,
	.my_resources = 
	{
		{ "/end_duel", COAP_POST, _end_duel, NULL},
		{ "/start_duel", COAP_POST, _start_duel, NULL},
		{ "/lobby_id", COAP_GET, _lobby_id, NULL},
		{ "/lobby", COAP_GET, _ready_to_duel, NULL},
		{ "/hit", COAP_POST, _hit_received, NULL},
		{ "/heartbeat", COAP_POST, _heartbeat, NULL},
	},
};