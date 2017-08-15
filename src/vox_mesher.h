#pragma once
#ifndef _VOX_MESHER_H_
#define _VOX_MESHER_H_

#include "vox_render.h"
#include "vox_world.h"

void meshVanillaNaive(Chunk *chunk, ChunkMesh *mesh);
void meshVanillaCull(Chunk *chunk, ChunkMesh *mesh);
void meshVanillaGreedy(Chunk *chunk, ChunkMesh *mesh);

#endif // _VOX_MESHER_H_
