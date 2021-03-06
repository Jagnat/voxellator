#pragma once

// Backend-agnostic rendering API for generic meshes
// Try not to expose any OpenGL/Vulkan specific stuff in here

#include "platform.h"
#include "main.h"

#include "thirdparty/j_threedee.h"

typedef struct
{
	union
	{
		struct
		{
			int16 x, y, z, w;
		};
		int16 element[4];
	};
	Color color;
	uint normal;
} VertexColorNormal10;


typedef union
{
	struct
	{
		uint8 x, y, z, voxel_type;
	};
	uint8 elem[4];
} VoxelVertex;

typedef enum
{
	INDEX_QUADS,
	INDEX_TRIS,
	INDEX_CUSTOM
} IndexMode;

typedef struct ChunkMesh
{
	int allocatedVertices;
	int usedVertices;
	VertexColorNormal10 *vertices;

	IndexMode indexMode;
	int numIndices;
	uint *indices;

	JMat4 modelMatrix;

	int uploaded;

	// TODO: Sub-struct this, decouple from OpenGL
	union
	{
		uint ids[3];
		struct
		{
			uint vboId, iboId, vaoId;
		};
	};
} ChunkMesh;

typedef enum
{
	SHADER_VERT,
	SHADER_FRAG
} ShaderType;

void initRender();
void resizeRender(int w, int h);

ChunkMesh* createChunkMesh();
void uploadChunkMesh(ChunkMesh *mesh);
void renderChunkMesh(ChunkMesh *mesh);
void deleteChunkMesh(ChunkMesh *mesh);

void setCam(Movement mov);
ChunkMesh *createSampleMesh();
