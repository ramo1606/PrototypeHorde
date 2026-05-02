#pragma once

#include "game_types.h"
#include "level.h"
#include "arena.h"
#include "camera.h"
#include "physics.h"
#include "renderer.h"
#include "level_manager.h"

bool GameInit(Game* game, Level* initialLevel);
void GameShutdown(Game* game);
void GameRun(Game* game);
