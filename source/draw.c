#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <3ds.h>
#include <sf2d.h>

#include "types.h"
#include "util.h"
#include "font.h"
#include "draw.h"

const Color SHADOW = {0, 0, 0, 0xC0};
const int SHADOW_X = 4;
const int SHADOW_Y = -4;

//The center of the screen
const Point SCREEN_CENTER = {TOP_WIDTH/2, SCREEN_HEIGHT/2};

//Overflow so you don't get glitchy lines between hexagons.
//This is really just some arbituary number so yeah...
const double OVERFLOW_OFFSET = TAU/900.0; 

//Amount of frames to wait when game over is shown.
const int FRAMES_PER_GAME_OVER = 60;

void drawTriangle(Color color, Point points[3]) {
	long paint = RGBA8(color.r,color.g,color.b,color.a);
	
	//draws a triangle on the correct axis
	sf2d_draw_triangle(
		points[0].x, SCREEN_HEIGHT - 1 - points[0].y,
		points[1].x, SCREEN_HEIGHT - 1 - points[1].y,
		points[2].x, SCREEN_HEIGHT - 1 - points[2].y,
		paint);
}

void drawTrap(Color color, Point points[4]) {
	Point triangle[3];
	triangle[0] = points[0];
	triangle[1] = points[1];
	triangle[2] = points[2];
	drawTriangle(color, triangle);
	triangle[1] = points[2];
	triangle[2] = points[3];
	drawTriangle(color, triangle);
}

void drawRect(Color color, Point position, Point size) {
	long paint = RGBA8(color.r,color.g,color.b,color.a);
	sf2d_draw_rectangle(position.x, position.y, size.x, size.y, paint);
}

void drawMovingWall(Color color, Point focus, LiveWall wall, double rotation, int sides) {
	double distance = wall.distance;
	double height = wall.height;
	
	if(distance + height < DEF_HEX_FULL_LEN) return; //TOO_CLOSE;
	if(distance > SCREEN_TOP_DIAG_FROM_CENTER) return; //TOO_FAR; 
	
	if(distance < DEF_HEX_FULL_LEN - 2.0) {//so the distance is never negative as it enters.
		height -= DEF_HEX_FULL_LEN - 2.0 - distance;
		distance = DEF_HEX_FULL_LEN - 2.0; //Should never be 0!!!
	}
	
	Point edges[4] = {0};
	edges[0] = calcPoint(focus, rotation, OVERFLOW_OFFSET, distance, wall.side + 1, sides);
	edges[1] = calcPoint(focus, rotation, OVERFLOW_OFFSET, distance + height, wall.side + 1, sides);
	edges[2] = calcPoint(focus, rotation, -OVERFLOW_OFFSET, distance + height, wall.side, sides);
	edges[3] = calcPoint(focus, rotation, -OVERFLOW_OFFSET, distance, wall.side, sides);
	drawTrap(color, edges);
	return; //RENDERED;
}

void drawMovingPatterns(Color color, Point focus, LiveLevel live, double manualOffset) {
	
	//for all patterns
	for(int iPattern = 0; iPattern < TOTAL_PATTERNS_AT_ONE_TIME; iPattern++) {
		LivePattern pattern = live.patterns[iPattern];
		
		//draw all walls
		for(int iWall = 0; iWall < pattern.numWalls; iWall++) {
			LiveWall wall = pattern.walls[iWall];
			wall.distance += manualOffset;
			drawMovingWall(color, focus, wall, live.rotation, live.patterns[0].sides);
		}
	}
}

void drawMainHexagon(Color color1, Color color2, Point focus, double rotation, int sides) {
	
	//This draws the center shape.
	Point triangle[3];
	triangle[0] = focus;
	
	for(int concentricHexes = 0; concentricHexes < 2; concentricHexes++) {
		double height;
		Color color;
		if(concentricHexes == 0) {
			//outer color painted first
			color = color1;
			height = DEF_HEX_FULL_LEN;
		}
		if(concentricHexes == 1) {
			//inner color painted over it all.
			color =  color2;
			height = DEF_HEX_FULL_LEN - DEF_HEX_BORDER_LEN;
		}
		
		//draw hexagon (might not actually be hexagon!) in accordance to the closest pattern
		for(int i = 0; i < sides + 1; i++) {
			triangle[(i % 2) + 1].x = (int)(height * cos(rotation + (double)i * TAU/(double)sides) + (double)(focus.x) + 0.5);
			triangle[(i % 2) + 1].y = (int)(height * sin(rotation + (double)i * TAU/(double)sides) + (double)(focus.y) + 0.5);
			if(i != 0) {
				drawTriangle(color, triangle);
			}
		}
	}
}

void drawBackground(Color color1, Color color2, Point focus, double height, double rotation, int sides) {
	
	//solid background.
	Point position = {0,0};
	Point size = {TOP_WIDTH, SCREEN_HEIGHT};
	drawRect(color1, position, size);
	
	//This draws the main background.
	Point* edges = malloc(sizeof(Point) * sides);
	check(!edges, "Error drawing background!", DEF_DEBUG, 0x0);
	
	for(int i = 0; i < sides; i++) {
		edges[i].x = (int)(height * cos(rotation + (double)i * TAU/6.0) + (double)(focus.x) + 0.5);
		edges[i].y = (int)(height * sin(rotation + (double)i * TAU/6.0) + (double)(focus.y) + 0.5);
	}
	
	Point triangle[3];
	triangle[0] = focus;
	for(int i = 0; i < sides - 1; i = i + 2) {
		triangle[1] = edges[i];
		triangle[2] = edges[i + 1];
		drawTriangle(color2, triangle);
	}
	
	//if the sides is odd we need to "make up a color" to put in the gap.
	if(sides % 2) {
		triangle[1] = edges[sides - 2];
		triangle[2] = edges[sides - 1];
		drawTriangle(interpolateColor(color1, color2, 0.5f), triangle);
	}
	
	free(edges);
}

void drawHumanCursor(Color color, Point focus, double cursor, double rotation) {
	Point humanTriangle[3];
	for(int i = 0; i < 3; i++) {
		double height = 0.0;
		double position = 0.0;
		if(i == 0) {
			height = DEF_HEX_FULL_LEN + DEF_HUMAN_PADDING + DEF_HUMAN_HEIGHT;
			position = cursor + rotation;
		} else {
			height = DEF_HEX_FULL_LEN + DEF_HUMAN_PADDING;
			if(i == 1) {
				position = cursor + DEF_HUMAN_WIDTH/2 + rotation;
			} else {
				position = cursor - DEF_HUMAN_WIDTH/2 + rotation;
			}
		}
		humanTriangle[i].x = (int)(height * cos(position) + focus.x + 0.5);
		humanTriangle[i].y = (int)(height * sin(position) + focus.y + 0.5);
	}
	drawTriangle(color, humanTriangle);
}

//internal framerate drawing
void drawFramerate(double fps) {
	Color color = {0xFF,0xFF,0xFF, 0xFF};
	Point position = {4,SCREEN_HEIGHT - 20};
	char framerate[12 + 1];
	snprintf(framerate, 12 + 1, "%.2f FPS", fps);
	writeFont(color, position, framerate, FONT16);
}

void drawMainMenu(GlobalData data, MainMenu menu) {
	double percentRotated = (double)(menu.transitionFrame) / (double)DEF_FRAMES_PER_TRANSITION;
	double rotation = percentRotated * TAU/6.0;
	if(menu.transitionDirection == -1) { //if the user is going to the left, flip the radians so the animation plays backwards.
		rotation *= -1.0;
	}
	
	//Colors
	Color FG;
	Color BG1;
	Color BG2;
	Color BG3;
	Level lastLevel = data.levels[menu.lastLevel];
	Level level = data.levels[menu.level];
	if(menu.transitioning) {
		FG = interpolateColor(lastLevel.colorsFG[0], level.colorsFG[0], percentRotated);
		BG1 = interpolateColor(lastLevel.colorsBG1[0], level.colorsBG2[0], percentRotated);
		BG2 = interpolateColor(lastLevel.colorsBG2[0], level.colorsBG1[0], percentRotated);
		BG3 = interpolateColor(lastLevel.colorsBG2[0], level.colorsBG2[0], percentRotated); //Real BG2 transition
	} else {
		FG = level.colorsFG[0];
		BG1 = level.colorsBG1[0];
		BG2 = level.colorsBG2[0];
		BG3 = level.colorsBG2[0]; //same as BG2
	} 
	
	Point focus = {TOP_WIDTH/2, SCREEN_HEIGHT/2 - 60};
	Point offsetFocus = {focus.x + SHADOW_X, focus.y + SHADOW_Y};
	
	//home screen always has 6 sides.
	drawBackground(BG1, BG2, focus, SCREEN_TOP_DIAG_FROM_CENTER, rotation, 6); 
	drawMainHexagon(SHADOW, SHADOW, offsetFocus, rotation, 6);
	drawMainHexagon(FG, BG3, focus, rotation, 6);
	drawHumanCursor(SHADOW, offsetFocus, TAU/4.0, 0);
	drawHumanCursor(FG, focus, TAU/4.0, 0); //Draw cursor fixed quarter circle, no movement.

	Color white = {0xFF, 0xFF, 0xFF, 0xFF};
	Color grey = {0xA0, 0xA0, 0xA0, 0xFF};
	Point title = {4, 4};
	Point difficulty = {4, 40};
	Point mode = {4, 56};
	Point creator = {4, 72};
	Point time = {4, SCREEN_HEIGHT - 18};
	
	Color black = {0,0,0, 0xA0};
	Point infoPos = {0, 0};
	Point infoSize = {TOP_WIDTH, creator.y + 16 + 2};
	drawRect(black, infoPos, infoSize);
	
	Point timePos = {0, time.y - 4};
	Point timeSize = {11/*chars*/ * 16, 16 + 8};
	drawRect(black, timePos, timeSize);
	
	writeFont(white, title, level.name.str, FONT32);
	writeFont(grey, difficulty, level.difficulty.str, FONT16);
	writeFont(grey, mode, level.mode.str, FONT16);
	writeFont(grey, creator, level.creator.str, FONT16);
	writeFont(white, time, "SCORE: ??????", FONT16);
}

void drawMainMenuBot(double fps) {
	Color black = {0,0,0, 0xFF};
	Point topLeft = {0,0};
	Point screenSize = {BOT_WIDTH, SCREEN_HEIGHT};
	drawRect(black, topLeft, screenSize);
	drawFramerate(fps);
}


void drawPlayGame(Level level, LiveLevel liveLevel, double offset) {
	
	//calculate colors
	double percentTween = (double)(liveLevel.tweenFrame) / (double)(level.speedPulse);
	Color FG = interpolateColor(level.colorsFG[liveLevel.indexFG], level.colorsFG[liveLevel.nextIndexFG], percentTween);
	Color BG1 = interpolateColor(level.colorsBG1[liveLevel.indexBG1], level.colorsBG1[liveLevel.nextIndexBG1], percentTween);
	Color BG2 = interpolateColor(level.colorsBG2[liveLevel.indexBG2], level.colorsBG2[liveLevel.nextIndexBG2], percentTween);
	
	drawBackground(BG1, BG2, SCREEN_CENTER, SCREEN_TOP_DIAG_FROM_CENTER, liveLevel.rotation, liveLevel.patterns[0].sides);
	
	//draw shadows
	Point offsetFocus = {SCREEN_CENTER.x + SHADOW_X, SCREEN_CENTER.y + SHADOW_Y};
	drawMovingPatterns(SHADOW, offsetFocus, liveLevel, 0);
	drawMainHexagon(SHADOW, SHADOW, offsetFocus, liveLevel.rotation, liveLevel.patterns[0].sides);
	drawHumanCursor(SHADOW, offsetFocus, liveLevel.cursorPos, liveLevel.rotation);
	
	//draw real thing
	drawMovingPatterns(FG, SCREEN_CENTER, liveLevel, 0);
	drawMainHexagon(FG, BG2, SCREEN_CENTER, liveLevel.rotation, liveLevel.patterns[0].sides);
	drawHumanCursor(FG, SCREEN_CENTER, liveLevel.cursorPos, liveLevel.rotation);
}

void drawPlayGameBot(FileString name, int score, double fps) {
	Color black = {0,0,0, 0xFF};
	Point topLeft = {0,0};
	Point screenSize = {BOT_WIDTH, SCREEN_HEIGHT};
	drawRect(black, topLeft, screenSize);
	
	Point scoreSize = {BOT_WIDTH, 22};
	drawRect(black, topLeft, scoreSize);
	
	Color white = {0xFF, 0xFF,  0xFF, 0xFF};
	Point levelUpPosition = {4,4};
	writeFont(white, levelUpPosition, "POINT", FONT16);
	
	Point scorePosition = {230,4};
	char buffer[6 + 1]; //null term
	int scoreInt = (int)((double)score/60.0);
	int decimalPart = (int)(((double)score/60.0 - (double)scoreInt) * 100.0);
	snprintf(buffer, 6 + 1, "%03d:%02d", scoreInt, decimalPart); //Emergency stack overflow prevention
	writeFont(white, scorePosition, buffer, FONT16);
	
	drawFramerate(fps);
}

void drawGameOverBot(LiveLevel level, int score, double fps, int frame) {
	Color black = {0, 0, 0, 0xFF};
	Point topLeft = {0,0};
	Point screenSize = {BOT_WIDTH, SCREEN_HEIGHT};
	drawRect(black, topLeft, screenSize);
	
	Point overSize = {BOT_WIDTH, 112};
	drawRect(black, topLeft, overSize);

	Color white = {0xFF,0xFF,0xFF, 0xFF};
	Point gameOverPosition = {44, 4};
	writeFont(white, gameOverPosition, "GAME OVER", FONT32);
	
	Point timePosition = {90, 40};
	char buffer[12+1];
	int scoreInt = (int)((double)score/60.0);
	int decimalPart = (int)(((double)score/60.0 - (double)scoreInt) * 100.0);
	snprintf(buffer, 12+1, "TIME: %03d:%02d", scoreInt, decimalPart);
	writeFont(white, timePosition, buffer, FONT16);
	
	if(frame > FRAMES_PER_GAME_OVER) {
		Point aPosition = {70, 70};
		writeFont(white, aPosition, "PRESS A TO PLAY", FONT16);
		Point bPosition = {70, 86};
		writeFont(white, bPosition, "PRESS B TO QUIT", FONT16);
	}
	
	drawFramerate(fps);
}