#pragma once

/*
 * layers.h — Game-specific collision layer bitflags.
 *
 * The physics module is layer-agnostic; it operates on raw int bitfields.
 * Project-specific names live here.
 *
 * Each layer is a single bit. A collider's `layer` is what it IS (one bit);
 * its `mask` is what it collides WITH (one or more bits).
 */

#define LAYER_PLAYER     (1 << 0)
#define LAYER_ENEMY      (1 << 1)
#define LAYER_PROJECTILE (1 << 2)
#define LAYER_SCENERY    (1 << 3)
#define LAYER_PICKUP     (1 << 4)
#define LAYER_TRIGGER    (1 << 5)
#define LAYER_DEPLOYABLE (1 << 6)

#define MASK_ALL         0x7F
#define MASK_NONE        0
