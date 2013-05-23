/*---------------------------------------------------------------------------------
	
	KubKraft
	Copyright (C) 2013 Quentin Bazin <quent42340@gmail.com>
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	
---------------------------------------------------------------------------------*/
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cmath>

#include <SDL/SDL.h>
#include <GL/glew.h>

#include <png.h>

#include "types.h"
#include "config.h"
#include "cube.h"
#include "chunk.h"
#include "map.h"
#include "player.h"
#include "game.h"

#include "texutils.h"

#include "simplexnoise1234.h"

using namespace std;

Chunk *Map::currentChunk = NULL;
Chunk *Map::selectedChunk = NULL;
Cube *Map::selectedCube = NULL;

Map::Map(u16 width, u16 depth, u16 height) {
	m_width = width;
	m_depth = depth;
	m_height = height;
	
	glGenTextures(1, &m_texture);
	m_texture = loadPngTexture("textures/blocks.png");
	
	m_map = (u16*)malloc(m_width * m_depth * m_height * sizeof(u16));
	
	for(s32 z = 0 ; z < m_height ; z++) {
		for(s32 y = 0 ; y < m_depth ; y++) {
			for(s32 x = 0 ; x < m_width ; x++) {
				m_map[_MAP_POS(x, y, z)] = 0;
			}
		}
	}
	
	for (int yy = 0; yy < m_depth >> 3; yy++) {
		for (int xx = 0; xx < m_width >> 3; xx++) {
			for (int yC = 0; yC < CHUNK_DEPTH; yC++) {
				for (int xC = 0; xC < CHUNK_WIDTH; xC++) {
					int x = xx * CHUNK_WIDTH + xC;
					int y = yy * CHUNK_DEPTH + yC;
					
					float perlin = snoise2((float)x * 0.01, (float)y * 0.01); // 0.035
					
					int heightValue = int((perlin * float(CHUNK_HEIGHT)) + float(m_height / 2));
					
					for(int zz = 0 ; zz < heightValue ; zz++) {
						float cavePerlin = snoise3(x * 0.1, y * 0.1, zz * 0.1) * 2;
						
						if(cavePerlin > -2 && cavePerlin < 1) {
							int dirtHeight = (1.0 - rand()%10 / 100 - 0.20) * heightValue;
							if(zz < dirtHeight) m_map[_MAP_POS(xC + (xx << 3), yC + (yy << 3), zz)] = 1;
							else if(zz > dirtHeight && zz < dirtHeight + 3) m_map[_MAP_POS(xC + (xx << 3), yC + (yy << 3), zz)] = rand()%2 + 1;
							else m_map[_MAP_POS(xC + (xx << 3), yC + (yy << 3), zz)] = 2;
							if(zz < 16 && cavePerlin > 0.01 && cavePerlin < 0.02) m_map[_MAP_POS(xC + (xx << 3), yC + (yy << 3), zz)] = 10;
						}
						
						if(zz == 0) {
							m_map[_MAP_POS(xC + (xx << 3), yC + (yy << 3), zz)] = 5;
						}
					}
				}
			}
		}
	}
	
	m_chunks = new Chunk*[(m_width >> 3) * (m_height >> 3) * (m_depth >> 3)];
	
	for(u16 z = 0 ; z < m_height >> 3 ; z++) {
		for(u16 y = 0 ; y < m_depth >> 3 ; y++) {
			for(u16 x = 0 ; x < m_width >> 3 ; x++) {
				m_chunks[CHUNK_POS(x, y, z)] = new Chunk(x << 3, y << 3, z << 3);
			}
		}
	}
				
	for(u16 z = 0 ; z < m_height >> 3 ; z++) {
		for(u16 y = 0 ; y < m_depth >> 3 ; y++) {
			for(u16 x = 0 ; x < m_width >> 3 ; x++) {
				int chunkIndex = x + (y * (m_width >> 3)) + (z * (m_width >> 3) * (m_depth >> 3));
				if(x - 1 >= 0) m_chunks[chunkIndex]->setSurroundingChunk(0, m_chunks[chunkIndex - 1]);
				if(x + 1 < m_width >> 3) m_chunks[chunkIndex]->setSurroundingChunk(1, m_chunks[chunkIndex + 1]);
				if(y - 1 >= 0) m_chunks[chunkIndex]->setSurroundingChunk(2, m_chunks[chunkIndex - (m_width >> 3)]);
				if(y + 1 < m_depth >> 3) m_chunks[chunkIndex]->setSurroundingChunk(3, m_chunks[chunkIndex + (m_width >> 3)]);
				if(z + 1 < m_height >> 3) m_chunks[chunkIndex]->setSurroundingChunk(4, m_chunks[chunkIndex + ((m_width >> 3) * (m_depth >> 3))]);
				if(z - 1 >= 0) m_chunks[chunkIndex]->setSurroundingChunk(5, m_chunks[chunkIndex - ((m_width >> 3) * (m_depth >> 3))]);
			}
		}
	}
	
	int cubeCount = 0;
	
	for(int i = 0 ; i < ((m_width >> 3) * (m_depth >> 3) * (m_height >> 3)); i++) {
		for(s32 z = m_chunks[i]->z() ; z < m_chunks[i]->z() + CHUNK_HEIGHT ; z++) {
			for(s32 y = m_chunks[i]->y() ; y < m_chunks[i]->y() + CHUNK_DEPTH ; y++) {
				for(s32 x = m_chunks[i]->x() ; x < m_chunks[i]->x() + CHUNK_WIDTH ; x++) {
					if(m_map[_MAP_POS(x, y, z)] != 0) m_chunks[i]->addCube(new Cube(x, y, z, m_map[_MAP_POS(x, y, z)]));
					cubeCount++;
				}
			}
		}
	}
	
	uint32_t time = SDL_GetTicks();
	
	for(int i = 0 ; i < ((m_width >> 3) * (m_depth >> 3) * (m_height >> 3)); i++) {
		m_chunks[i]->refreshVBO();
		cout << "Chunks loaded: " << i+1 << "/" << ((m_width >> 3) * (m_depth >> 3) * (m_height >> 3)) << endl;
	}
	
	cout << "Chunks loading time: " << (SDL_GetTicks() - time) << " ms for " << cubeCount << " cubes" << endl;
	
	float distance = DIST_FAR;
	currentChunk = NULL;
	
	m_chunkDisplay = new long int[6];
	
	updateChunkDisplay();
	
	unsigned long long int pos;
	for(long int z = m_chunkDisplay[2] ; z < m_chunkDisplay[5] ; z++) {
		for(long int y = m_chunkDisplay[1] ; y < m_chunkDisplay[4] ; y++) {
			for(long int x = m_chunkDisplay[0] ; x < m_chunkDisplay[3] ; x++) {
				if(x < 0 || y < 0 || z < 0 || x >= (m_width >> 3) || y >= (m_depth >> 3) || z >= (m_height >> 3)) continue;
				pos = CHUNK_POS(x, y, z);
				vect3D center;
				
				center.x = m_chunks[pos]->x() + CHUNK_WIDTH / 2;
				center.y = m_chunks[pos]->y() + CHUNK_DEPTH / 2;
				center.z = m_chunks[pos]->z() + CHUNK_HEIGHT / 2;
				
				float d = sqrt(pow(center.x - Game::player->x(), 2) + pow(center.y - Game::player->y(), 2) + pow(center.z - Game::player->z(), 2));
				
				if(d < distance) {
					distance = d;
					currentChunk = m_chunks[pos];
				}
			}
		}
	}
	
	selectedCube = new Cube(-1, -1, -1, 0);
}

Map::~Map() {
	free(m_map);
	
	for(int i = 0 ; i < ((m_width >> 3) * (m_depth >> 3) * (m_height >> 3)); i++) {
		if(m_chunks[i] != NULL) delete m_chunks[i];
	}
	
	delete[] m_chunks;
	
	glDeleteTextures(1, &m_texture);
	
	delete[] m_chunkDisplay;
}

void Map::updateChunkDisplay() {
	// Get visible chunk surface
	m_chunkDisplay[0] = ((long)(Game::player->x() - DIST_FAR) >> 3);
	m_chunkDisplay[1] = ((long)(Game::player->y() - DIST_FAR) >> 3);
	m_chunkDisplay[2] = ((long)(Game::player->z() - DIST_FAR) >> 3);
	
	m_chunkDisplay[3] = ((long)(Game::player->x() + DIST_FAR) >> 3);
	m_chunkDisplay[4] = ((long)(Game::player->y() + DIST_FAR) >> 3);
	m_chunkDisplay[5] = ((long)(Game::player->z() + DIST_FAR) >> 3);
}

float frustum[6][4];

void extractFrustum() {
	float t;
	float proj[16];
	float modl[16];
	float clip[16];
	
	// Get the current PROJECTION matrix from OpenGL
	glGetFloatv(GL_PROJECTION_MATRIX, proj);
	
	// Get the current MODELVIEW matrix from OpenGL
	glGetFloatv(GL_MODELVIEW_MATRIX, modl);
	
	// Combine the two matrices (multiply projection by modelview)
	clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
	clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
	clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
	clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];
	
	clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
	clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
	clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
	clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];
	
	clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
	clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
	clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
	clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];
	
	clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
	clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
	clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
	clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];
	
	// Extract the numbers for the RIGHT plane
	frustum[0][0] = clip[ 3] - clip[ 0];
	frustum[0][1] = clip[ 7] - clip[ 4];
	frustum[0][2] = clip[11] - clip[ 8];
	frustum[0][3] = clip[15] - clip[12];
	
	// Extract the numbers for the LEFT plane
	frustum[1][0] = clip[ 3] + clip[ 0];
	frustum[1][1] = clip[ 7] + clip[ 4];
	frustum[1][2] = clip[11] + clip[ 8];
	frustum[1][3] = clip[15] + clip[12];
	
	// Extract the BOTTOM plane
	frustum[2][0] = clip[ 3] + clip[ 1];
	frustum[2][1] = clip[ 7] + clip[ 5];
	frustum[2][2] = clip[11] + clip[ 9];
	frustum[2][3] = clip[15] + clip[13];
	
	// Extract the TOP plane
	frustum[3][0] = clip[ 3] - clip[ 1];
	frustum[3][1] = clip[ 7] - clip[ 5];
	frustum[3][2] = clip[11] - clip[ 9];
	frustum[3][3] = clip[15] - clip[13];
	
	// Extract the FAR plane
	frustum[4][0] = clip[ 3] - clip[ 2];
	frustum[4][1] = clip[ 7] - clip[ 6];
	frustum[4][2] = clip[11] - clip[10];
	frustum[4][3] = clip[15] - clip[14];
	
	// Extract the NEAR plane
	frustum[5][0] = clip[ 3] + clip[ 2];
	frustum[5][1] = clip[ 7] + clip[ 6];
	frustum[5][2] = clip[11] + clip[10];
	frustum[5][3] = clip[15] + clip[14];
	
	for(int i = 0 ; i < 6 ; i++) {
		t = sqrt(frustum[i][0] * frustum[i][0] + frustum[i][1] * frustum[i][1] + frustum[i][2] * frustum[i][2]);
		frustum[i][0] /= t;
		frustum[i][1] /= t;
		frustum[i][2] /= t;
		frustum[i][3] /= t;
	}
}

int cubeInFrustum(float x, float y, float z, float size) {
	int p;
	int c;
	int c2 = 0;
	
	for(p = 0 ; p < 6 ; p++) {
		c = 0;
		if((frustum[p][0] * (x - size) + frustum[p][1] * (y - size) + frustum[p][2] * (z - size) + frustum[p][3] > 0)
		|| (frustum[p][0] * (x + size) + frustum[p][1] * (y - size) + frustum[p][2] * (z - size) + frustum[p][3] > 0)
		|| (frustum[p][0] * (x - size) + frustum[p][1] * (y + size) + frustum[p][2] * (z - size) + frustum[p][3] > 0)
		|| (frustum[p][0] * (x + size) + frustum[p][1] * (y + size) + frustum[p][2] * (z - size) + frustum[p][3] > 0)
		|| (frustum[p][0] * (x - size) + frustum[p][1] * (y - size) + frustum[p][2] * (z + size) + frustum[p][3] > 0)
		|| (frustum[p][0] * (x + size) + frustum[p][1] * (y - size) + frustum[p][2] * (z + size) + frustum[p][3] > 0)
		|| (frustum[p][0] * (x - size) + frustum[p][1] * (y + size) + frustum[p][2] * (z + size) + frustum[p][3] > 0)
		|| (frustum[p][0] * (x + size) + frustum[p][1] * (y + size) + frustum[p][2] * (z + size) + frustum[p][3] > 0))
			c++;
		if(c == 0) return 0;
		else if(c == 8) c2++;
	}
	return (c2 == 6) ? 2 : 1;
}

void Map::render() {
	glColor3ub(0, 0, 0);
	glBegin(GL_QUADS);
		glVertex3f(0, 0, -0.01);
		glVertex3f(m_width, 0, -0.01);
		glVertex3f(m_width, m_depth, -0.01);
		glVertex3f(0, m_depth, -0.01);
	glEnd();
	glColor3ub(255, 255, 255);
	
	glBindTexture(GL_TEXTURE_2D, m_texture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	extractFrustum();
	
	updateChunkDisplay();
	
	uint32_t time = SDL_GetTicks();
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	float distance = DIST_FAR;
	currentChunk = NULL;
	
	unsigned long long int pos;
	for(long int z = m_chunkDisplay[2] ; z < m_chunkDisplay[5] ; z++) {
		for(long int y = m_chunkDisplay[1] ; y < m_chunkDisplay[4] ; y++) {
			for(long int x = m_chunkDisplay[0] ; x < m_chunkDisplay[3] ; x++) {
				if(x < 0 || y < 0 || z < 0 || x >= (m_width >> 3) || y >= (m_depth >> 3) || z >= (m_height >> 3)) continue;
				pos = CHUNK_POS(x, y, z);
				if(cubeInFrustum(m_chunks[pos]->x(), m_chunks[pos]->y(), m_chunks[pos]->z(), 8)) {
					m_chunks[pos]->render();
				}
				
				vect3D center;
				
				center.x = m_chunks[pos]->x() + CHUNK_WIDTH / 2;
				center.y = m_chunks[pos]->y() + CHUNK_DEPTH / 2;
				center.z = m_chunks[pos]->z() + CHUNK_HEIGHT / 2;
				
				float d = sqrt(pow(center.x - Game::player->x(), 2) + pow(center.y - Game::player->y(), 2) + pow(center.z - Game::player->z(), 2));
				
				if(d < distance) {
					distance = d;
					currentChunk = m_chunks[pos];
				}
			}
		}
	}
	
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	
	time = SDL_GetTicks() - time;
	cout << "Render time: " << time << " ms" << endl;
	
	testCubes();
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	if ((currentChunk != NULL) && (selectedCube->x() != -1) && (selectedCube->selectedFace() >= 0) && (selectedCube->selectedFace() < 6)) {
		float cubeCoords[6 * 12] = {
			1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, // FR | 0
			1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, // FL | 1
			0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, // BR | 2
			0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, // BL | 3
			0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, // T  | 4
			1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0  // B  | 5
		};
		
		int xx = selectedCube->x();
		int yy = selectedCube->y();
		int zz = selectedCube->z();
		
		glColor4ub(255, 255, 255, 64);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0, -1.0);
		
		glBegin(GL_QUADS);
			glVertex3f(xx + cubeCoords[(selectedCube->selectedFace() * 12) + 0], yy + cubeCoords[(selectedCube->selectedFace() * 12) + 1], zz + cubeCoords[(selectedCube->selectedFace() * 12) + 2]);
			glVertex3f(xx + cubeCoords[(selectedCube->selectedFace() * 12) + 3], yy + cubeCoords[(selectedCube->selectedFace() * 12) + 4], zz + cubeCoords[(selectedCube->selectedFace() * 12) + 5]);
			glVertex3f(xx + cubeCoords[(selectedCube->selectedFace() * 12) + 6], yy + cubeCoords[(selectedCube->selectedFace() * 12) + 7], zz + cubeCoords[(selectedCube->selectedFace() * 12) + 8]);
			glVertex3f(xx + cubeCoords[(selectedCube->selectedFace() * 12) + 9], yy + cubeCoords[(selectedCube->selectedFace() * 12) + 10], zz + cubeCoords[(selectedCube->selectedFace() * 12) + 11]);
		glEnd();
		
		glDisable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_ALPHA_TEST);
		glColor4ub(255, 255, 255, 255);
	}
}

Chunk* Map::getChunk(int x, int y, int z) {
	if ((x < 0) || (x >= m_width >> 3) || (y < 0) || (y >= m_depth >> 3) || (z < 0) || (z >= m_height >> 3)) {
		return NULL;
	}
	
	int coords = x + (y * (m_width >> 3)) + (z * (m_width >> 3) * (m_depth >> 3));
	
	return m_chunks[coords];
}

bool Map::intersectionLinePlane(vect3D normal, vect3D planePoint, vect3D lineOrigPoint, vect3D directionVector, float *distance) {
	float p1 = directionVector.x * normal.x + directionVector.y * normal.y + directionVector.z * normal.z; // First point to be tested
	
	if(p1 == 0) return false; // Degenerate case
	
	vect3D u = vect3D(planePoint.x - lineOrigPoint.x,
					  planePoint.y - lineOrigPoint.y,
					  planePoint.z - lineOrigPoint.z);
	
	float p2 = u.x * normal.x + u.y * normal.y + u.z * normal.z; // Second point to be tested
	
	float k = p2 / p1;
	
	if((k < 0) || (k > 5)) return false;
	
	// Intersection point
	vect3D i = vect3D(lineOrigPoint.x + k * directionVector.x,
					  lineOrigPoint.y + k * directionVector.y,
					  lineOrigPoint.z + k * directionVector.z);
	
	vect3D v = vect3D(i.x - planePoint.x,
					  i.y - planePoint.y,
					  i.z - planePoint.z);
	
	float size = 0.5;
	
	if(v.x >= -size && v.x <= size && v.y >= -size && v.y <= size && v.z >= -size && v.z <= size) {
		if(distance != NULL) *distance = k;
		return true;
	} else {
		return false;
	}
}

// Front right = 0 | Front left = 1
// Back right = 2 | Back left = 3
// Top = 4 | Bottom = 5
bool Map::intersectionLineCube(int cubeX, int cubeY, int cubeZ, vect3D lineOrigPoint, vect3D directionVector, float *distance, s8 *face) {
	vect3D planePoint[6] = {
		vect3D(cubeX + 0.5, cubeY + 1, cubeZ + 0.5), // back
		vect3D(cubeX + 1, cubeY + 0.5, cubeZ + 0.5), // right
		vect3D(cubeX, cubeY + 0.5, cubeZ + 0.5), // left
		vect3D(cubeX + 0.5, cubeY, cubeZ + 0.5), // front
		vect3D(cubeX + 0.5, cubeY + 0.5, cubeZ + 1), // top
		vect3D(cubeX + 0.5, cubeY + 0.5, cubeZ) // bottom
	};
	
	vect3D normal[6] = {
		vect3D(0, 1, 0), // back
		vect3D(1, 0, 0), // right
		vect3D(-1, 0, 0), // left
		vect3D(0, -1, 0), // front
		vect3D(0, 0, 1), // top
		vect3D(0, 0, -1) // bottom
	};
	
	float shortestDistance = DIST_FAR;
	float dist = DIST_FAR + 1.0;
	int nearestFace = -1;
	
	for (int i = 0; i < 6; i++) {
		bool result = intersectionLinePlane(normal[i], planePoint[i], lineOrigPoint, directionVector, &dist);
		if (result && (dist < shortestDistance)) {
			shortestDistance = dist;
			nearestFace = i;
		}
	}
	
	if (nearestFace < 0) {
		return false;
	} else {
		if (distance != NULL) *distance = shortestDistance;
		if (face != NULL) *face = nearestFace;
		return true;
	}
}

void Map::testCubes() {
	vect3D linePoint = vect3D(Game::player->x(),
							  Game::player->y(),
							  Game::player->z());
	
	vect3D directionVector = vect3D(Game::player->pointTargetedx() - Game::player->x(),
									Game::player->pointTargetedy() - Game::player->y(),
									Game::player->pointTargetedz() - Game::player->z());
	
	float distance = DIST_FAR;
	Cube *cube = NULL;
	int face = -1;
	Chunk *chunk = NULL;
	Chunk *c = NULL;
	unordered_map<int, Cube*> *cubes;
	for(unsigned short i = 0 ; i < 7 + 20 ; i++) {
		if(i == 6) cubes = currentChunk->cubes();
		else if(i < 6) {
			if(currentChunk->surroundingChunks()[i] == NULL) continue;
			cubes = currentChunk->surroundingChunks()[i]->cubes();
		} else {
			int coords[20 * 3] = {
				1, 0, 1,	-1, 0, 1,	
				1, 1, 0,	-1, 1, 0,
				1, -1, 0,	-1, -1, 0,
				1, 1, 1,	-1, 1, 1,	
				1, -1, 1,	-1, -1, 1,
				1, 0, -1,	-1, 0, -1,
				1, 1, -1,	-1, 1, -1,
				1, -1, -1,	-1, -1, -1,
				0, 1, 1,	0, -1, 1,
				0, 1, -1,	0, -1, -1
			};
			
			c = getChunk((currentChunk->x() >> 3) + coords[(i-7)*3],
						 (currentChunk->y() >> 3) + coords[(i-7)*3 + 1],
						 (currentChunk->z() >> 3) + coords[(i-7)*3 + 2]);
			
			if(c == NULL) continue;
			
			cubes = c->cubes();
		}
		
		for(std::unordered_map<int, Cube*>::iterator it = cubes->begin() ; it != cubes->end() ; it++) {
			if(it->second == NULL) continue;
			
			it->second->setSelected(false, -1);
			
			float d = -1;
			s8 f = -1;
			
			bool result = intersectionLineCube(it->second->x(), it->second->y(), it->second->z(), linePoint, directionVector, &d, &f);
			
			if(result && (d < distance) && (d < 5)) {
				distance = d;
				cube = it->second;
				face = f;
				if(i == 6) chunk = currentChunk;
				else if(i < 6) chunk = currentChunk->surroundingChunks()[i];
				else if(c != NULL) chunk = c;
			}
		}
	}
	
	if(cube != NULL) {
		selectedCube = cube;
		cube->setSelected(true, face);
		face = -1;
	} else {
		selectedCube->setSelected(false, -1);
	}
	
	if(chunk != NULL) {
		selectedChunk = chunk;
	} else {
		selectedChunk = NULL;
	}
}

