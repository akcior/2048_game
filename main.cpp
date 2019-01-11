#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include<stdarg.h>

extern "C" {
#include"./sdl-2.0.7/include/SDL.h"
#include"./sdl-2.0.7/include/SDL_main.h"
}

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480
#define FIELD_WIDTH		420
#define FIELD_HEIGHT	420
#define MENU_WIDTH		400
#define MENU_HEIGHT		400
#define FPS				60	//standard value of frames per sec
#define MAX_GAME_SIZE	8
#define ONPAGE			10 // amount of items on page



struct Surf_list {
	SDL_Surface *surf;
	Surf_list *next;
};

struct saves_list_t {
	char savename[32];
	saves_list_t *next = NULL;
};

struct mov_t {
	int m;
	int bx;
	int by;
	int lastinc;
	int secx;
	int secy;
};

// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y)
// charset to bitmapa 128x128 zawierajπca znaki
// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface *screen, int x, int y, const char *text,
	SDL_Surface *charset, int size) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = size;
	d.h = size;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y - size / 2;
		SDL_BlitScaled(charset, &s, screen, &d);
		x += size;
		text++;
	};
};


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt úrodka obrazka sprite na ekranie
// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};


// rysowanie pojedynczego pixela
// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
};


// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) 
// bπdü poziomie (gdy dx = 1, dy = 0)
// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


// rysowanie prostokπta o d≥ugoúci bokÛw l i k
// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

void FreeSurfaces(SDL_Surface *arg1, ...) {
	va_list surfaces;
	va_start(surfaces, arg1);
	for (SDL_Surface *i = arg1; i >= 0; i = va_arg(surfaces, SDL_Surface*))
		SDL_FreeSurface(i);
}


void InsertSurf(Surf_list *head, SDL_Surface *surface) {
	Surf_list *cur;
	if (head->surf == NULL) {
		head->surf = surface;
		return;
	}
	cur = head;
	while (cur->next != NULL) {
		cur = cur->next;
	}
	cur->next = (Surf_list*)malloc(sizeof(Surf_list));
	if (cur->next == NULL) {
		printf("malloc(Surf_list) error");
		return;
	}
	memset(cur->next, 0, sizeof(Surf_list));
	cur->next->surf = surface;
	return;
}

void FreeAllSurfaces(Surf_list *head) {
	Surf_list *cur;
	cur = head;
	while (cur->surf != NULL) {
		SDL_FreeSurface(cur->surf);
		if (cur->next != NULL) cur = cur->next;
		else break;
	}
}

void GetRandomTile(int field[][MAX_GAME_SIZE], int *seed, int gamesize) {
	srand(*seed);
	int x = 0, y = 0;
	do {
		x = rand() % gamesize;
		y = rand() % gamesize;
	} while (field[x][y] != 0);
	field[x][y] = 2;
	*seed = rand() + x + y;
}

void saveGame(int field[][MAX_GAME_SIZE], int *gamesize, int *points, int *seed) {
	FILE *savefile;
	FILE *saves;
	time_t timer;
	struct tm *timeinfo;
	char savename[32];
	char date[32];
	char lastsaved[32];
	time(&timer);
	timeinfo = localtime(&timer);
	strftime(date, 32, "%G%m%d%H%M%S", timeinfo);

	sprintf(savename, "saves/%s", date);
	savefile = fopen(savename, "wb");
	saves = fopen("saves/savelist", "a+b");
	if (savefile != NULL && saves != NULL) {
		for (int i = 0; i < MAX_GAME_SIZE; i++) {
			fwrite(field[i], sizeof(int), MAX_GAME_SIZE, savefile);
		}
		fwrite(gamesize, sizeof(int), 1, savefile);
		fwrite(points, sizeof(int), 1, savefile);
		fwrite(seed, sizeof(int), 1, savefile);
		fseek(saves, -32, SEEK_END);
		fread(lastsaved, sizeof(char), 32, saves);
		fseek(saves, 0, SEEK_END);
		if (strcmp(date, lastsaved)) fwrite(date, sizeof(char), 32, saves);
		fclose(savefile);
		fclose(saves);

	}
	else printf("SAVING ERROR!!");
}



int PushTiles(int field[][MAX_GAME_SIZE], int gamesize, int direction, mov_t mov[][MAX_GAME_SIZE], int *points) {
	int moving = 0;
	if (direction == 3) {



		for (int i = 0; i < gamesize; i++) {
			for (int j = 0; j < gamesize; j++) {
				int k = field[i][j];
				if (j > 0 && k != 0) {
					int l = j;
					while (l - 1 >= 0 && field[i][l - 1] == 0) l--;

					if (l > 0 && field[i][l - 1] == field[i][j] && mov[i][l - 1].lastinc != 1) {
						l--;
						field[i][l] *= 2;
						mov[i][l].lastinc = 1;
						mov[i][l].secx = j;
						mov[i][l].secy = i;
						field[i][j] = 0;
						*points += k * 2;
						moving = 1;
					}
					else if (l != j) {
						field[i][l] = k;
						field[i][j] = 0;
						mov[i][l].m = 1;
						mov[i][l].bx = j;
						mov[i][l].by = i;
						mov[i][l].lastinc = 0;//
						moving = 1;
					}
					else {
						mov[i][l].m = 0;
						mov[i][l].lastinc = 0;
					}
				}
			}
		}
	}
	else if (direction == 1) {
		for (int i = 0; i < gamesize; i++) {
			for (int j = gamesize - 1; j >= 0; j--) {
				int k = field[i][j];
				if (j < gamesize - 1 && k != 0) {
					int l = j;
					while (l + 1 < gamesize && field[i][l + 1] == 0)l++;
					if (l < gamesize - 1 && field[i][l + 1] == field[i][j] && mov[i][l + 1].lastinc != 1) {
						l++;
						field[i][l] *= 2;
						mov[i][l].lastinc = 1;
						mov[i][l].secx = j;
						mov[i][l].secy = i;
						field[i][j] = 0;
						*points += k * 2;
						moving = 1;
					}
					else if (l != j) {
						field[i][l] = k;
						field[i][j] = 0;
						mov[i][l].m = 1;
						mov[i][l].bx = j;
						mov[i][l].by = i;
						mov[i][l].lastinc = 0;//;
						moving = 1;
					}
					else {
						mov[i][l].m = 0;
						mov[i][l].lastinc = 0;
					}
				}
			}
		}
	}
	else if (direction == 0) {
		for (int i = 0; i < gamesize; i++) {
			for (int j = 0; j < gamesize; j++) {
				int k = field[j][i];
				if (j > 0 && k != 0) {
					int l = j;
					while (l - 1 >= 0 && field[l - 1][i] == 0) l--;
					if (l > 0 && field[l - 1][i] == field[j][i] && mov[l - 1][i].lastinc != 1) {
						l--;
						field[l][i] *= 2;
						mov[l][i].lastinc = 1;
						mov[l][i].secx = i;
						mov[l][i].secy = j;
						field[j][i] = 0;
						*points += k * 2;
						moving = 1;
					}
					else if (l != j) {
						field[l][i] = k;
						field[j][i] = 0;
						mov[l][i].m = 1;
						mov[l][i].bx = i;
						mov[l][i].by = j;
						mov[l][i].lastinc = 0;//;
						moving = 1;
					}
					else {
						mov[l][i].m = 0;
						mov[l][i].lastinc = 0;
					}
				}
			}
		}
	}
	else if (direction == 2) {
		for (int i = 0; i < gamesize; i++) {
			for (int j = gamesize - 1; j >= 0; j--) {
				int k = field[j][i];
				if (j < gamesize - 1 && k != 0) {
					int l = j;
					while (l + 1 < gamesize && field[l + 1][i] == 0) l++;
					if (l < gamesize - 1 && field[l + 1][i] == field[j][i] && mov[l + 1][i].lastinc != 1) {
						l++;
						field[l][i] *= 2;
						mov[l][i].lastinc = 1;
						mov[l][i].secx = i;
						mov[l][i].secy = j;
						field[j][i] = 0;
						*points += k * 2;
						moving = 1;
					}
					else if (l != j) {
						field[l][i] = k;
						field[j][i] = 0;
						mov[l][i].m = 1;
						mov[l][i].bx = i;
						mov[l][i].by = j;
						mov[l][i].lastinc = 0;
						moving = 1;
					}
					else {
						mov[l][i].m = 0;
						mov[l][i].lastinc = 0;
					}
				}

			}
		}
	}
	return moving;
}


int checkPossibility(int field[][MAX_GAME_SIZE], int gamesize) {
	for (int i = 0; i < gamesize; i++) {
		for (int j = 0; j < gamesize; j++) {
			for (int a = -1; a < 2; a++) {
				for (int b = -1; b < 2; b++) {
					if ((b == 0 && (a == -1 || a == 1)) || (a == 0 && (b == -1 || b == 1))) {
						if (i + a >= 0 && i + a < gamesize && j + b >= 0 && j + b < gamesize) {
							if (field[i + a][j + b] == 0 || field[i + a][j + b] == field[i][j]) return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}

saves_list_t *LoadSaves(int *pages) {
	saves_list_t *list;
	FILE *savelistfile;
	saves_list_t *cur;
	savelistfile = fopen("saves/savelist", "rb");
	if (savelistfile != NULL) {
		fseek(savelistfile, 0, SEEK_END);
		int size = ftell(savelistfile);
		*pages = ceil((double)size / 32 / ONPAGE);
		printf("%d", size);
		fseek(savelistfile, 0, SEEK_SET);
		list = (saves_list_t*)malloc(sizeof(saves_list_t));
		memset(list, 0, sizeof(saves_list_t));
		fread(list->savename, sizeof(char), 32, savelistfile);
		if (ftell(savelistfile) < size) {
			list->next = (saves_list_t*)malloc(sizeof(saves_list_t));
			memset(list->next, 0, sizeof(saves_list_t));
			fread(list->next->savename, sizeof(char), 32, savelistfile);
			cur = list->next;
			while (ftell(savelistfile) < size) {
				cur->next = (saves_list_t*)malloc(sizeof(saves_list_t));
				memset(cur->next, 0, sizeof(saves_list_t));
				fread(cur->next->savename, sizeof(char), 32, savelistfile);
				printf("hello\n");
				cur = cur->next;
			}
		}
		fclose(savelistfile);
		return list;

	}
	else {
		printf("SAVE LIST LOADING FAILURE !");
		return 0;
	}
}

int stepBack(int field[][MAX_GAME_SIZE], int lastvalue[][MAX_GAME_SIZE], int *points,int lastpoints, mov_t mov[][MAX_GAME_SIZE], mov_t lastmov[][MAX_GAME_SIZE], int gamesize) {
	//int lastvalue[MAX_GAME_SIZE][MAX_GAME_SIZE];
	int moving = 0;
	//memcpy(lastvalue, field, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
	memset(field, 0, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
	memset(mov, 0, sizeof(mov_t)*MAX_GAME_SIZE*MAX_GAME_SIZE);
	memcpy(field, lastvalue, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
	*points = lastpoints;

	//for (int i = 0; i < gamesize; i++) {
	//	for (int j = 0; j < gamesize; j++) {
	//		if (lastmov[i][j].lastinc) {
	//			field[lastmov[i][j].secy][lastmov[i][j].secx] = lastvalue[i][j] / 2;//
	//			mov[lastmov[i][j].secx][lastmov[i][j].secy].m = 1;
	//			mov[lastmov[i][j].secx][lastmov[i][j].secy].bx = j;
	//			mov[lastmov[i][j].secx][lastmov[i][j].secy].by = i;

	//			if (lastmov[i][j].m) {
	//				field[lastmov[i][j].by][lastmov[i][j].bx] = lastvalue[i][j] / 2;//
	//				mov[lastmov[i][j].bx][lastmov[i][j].by].m = 1;
	//				mov[lastmov[i][j].bx][lastmov[i][j].by].bx = j;
	//				mov[lastmov[i][j].bx][lastmov[i][j].by].by = i;
	//			}
	//			else field[i][j] = lastvalue[i][j] / 2;
	//			moving = 1;


	//		}
	//		else if (lastmov[i][j].m) {
	//			field[lastmov[i][j].by][lastmov[i][j].bx] = lastvalue[i][j];
	//			mov[lastmov[i][j].bx][lastmov[i][j].by].m = 1;
	//			mov[lastmov[i][j].bx][lastmov[i][j].by].bx = j;
	//			mov[lastmov[i][j].bx][lastmov[i][j].by].by = i;
	//			moving = 1;
	//		}
	//		else {
	//			mov[lastmov[i][j].bx][lastmov[i][j].by].m = 0;
	//			mov[lastmov[i][j].bx][lastmov[i][j].by].lastinc = 0;
	//			field[i][j] = lastvalue[i][j];
	//		}
	//	}
	//}
	return moving;
}

void Game(int field[][MAX_GAME_SIZE], int gamesize, int seed, int points, SDL_Surface *screen, SDL_Texture *scrtex, SDL_Renderer *renderer, SDL_Surface *charset) {
	SDL_Event event;
	SDL_Surface *gamefield, *gamefieldbmptmpl, *gamefieldbmp, *tilescolor, *tile, *emptile, *numbers, *number, *tilenumber, *endgame;
	Surf_list gamesurfaces;
	SDL_Rect cutile, cunumber, cunumbers, cufield;
	memset(&gamesurfaces, 0, sizeof(Surf_list));
	int escape = 0,
		back = 1,
		lastvalue[MAX_GAME_SIZE][MAX_GAME_SIZE],
		lastlastvalue[MAX_GAME_SIZE][MAX_GAME_SIZE],
		lastpoints = 0,
		lastlastpoints = 0;
	double t1 = 0,
		t2 = 0,
		delta = 0,
		gametime = 0,
		movduration = 0,
		animtime = 0.3,
		moving = 0,
		endbuff = 0;
	mov_t mov[MAX_GAME_SIZE][MAX_GAME_SIZE];
	mov_t lastmov[MAX_GAME_SIZE][MAX_GAME_SIZE];
	memset(mov, 0, sizeof(mov_t)*MAX_GAME_SIZE*MAX_GAME_SIZE);

	gamefieldbmptmpl = SDL_LoadBMP("./tlobig.bmp");
	if (gamefieldbmptmpl == NULL) {
		FreeAllSurfaces(&gamesurfaces);
		return;
	}
	InsertSurf(&gamesurfaces, gamefieldbmptmpl);


	tilescolor = SDL_LoadBMP("./tilescolor1.bmp");
	if (tilescolor == NULL) {
		FreeAllSurfaces(&gamesurfaces);
		return;
	}
	InsertSurf(&gamesurfaces, tilescolor);

	emptile = SDL_LoadBMP("./emptile.bmp");
	if (emptile == NULL) {
		FreeAllSurfaces(&gamesurfaces);
		return;
	}
	InsertSurf(&gamesurfaces, emptile);

	numbers = SDL_LoadBMP("./numbers.bmp");
	if (numbers == NULL) {
		FreeAllSurfaces(&gamesurfaces);
		return;
	}
	InsertSurf(&gamesurfaces, numbers);

	gamefieldbmp = SDL_CreateRGBSurface(0, 840, 840, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	InsertSurf(&gamesurfaces, gamefieldbmp);

	tile = SDL_CreateRGBSurface(0, gamefieldbmp->w / gamesize - (gamefieldbmp->w / (10 * gamesize)), gamefieldbmp->h / gamesize - (gamefieldbmp->h / (10 * gamesize)), 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	InsertSurf(&gamesurfaces, tile);

	gamefield = SDL_CreateRGBSurface(0, FIELD_WIDTH, FIELD_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	InsertSurf(&gamesurfaces, gamefield);

	number = SDL_CreateRGBSurface(0, 64, 64, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	InsertSurf(&gamesurfaces, number);

	tilenumber = SDL_CreateRGBSurface(0, gamefieldbmp->w / gamesize - (gamefieldbmp->w / (10 * gamesize)), 64, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	InsertSurf(&gamesurfaces, tilenumber);

	endgame = SDL_CreateRGBSurface(0, FIELD_WIDTH, FIELD_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	InsertSurf(&gamesurfaces, endgame);

	int bialy = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
	int czarny = SDL_MapRGB(screen->format, 0x000, 0x00, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xF0, 0x00, 0x00);

	Uint8 alpha = 128;
	SDL_FillRect(endgame, NULL, czerwony);

	int k;
	cufield.h = gamefieldbmp->h - (gamefieldbmp->h / (10 * gamesize));
	cufield.w = gamefieldbmp->w - (gamefieldbmp->w / (10 * gamesize));
	cufield.x = (gamefieldbmp->w / (10 * gamesize)) / 2;
	cufield.y = (gamefieldbmp->w / (10 * gamesize)) / 2;

	while (escape != 1) {
		t2 = SDL_GetTicks();
		delta = (t2 - t1)*0.001;
		t1 = t2;
		gametime += delta;

		int canMove = checkPossibility(field, gamesize);
		if (!canMove) {
			endbuff = 1;
		}


		SDL_BlitSurface(gamefieldbmptmpl, NULL, gamefieldbmp, NULL);
		SDL_FillRect(screen, NULL, bialy);
		SDL_BlitScaled(emptile, NULL, tile, NULL);
		for (int i = 0; i < gamesize; i++) {
			for (int j = 0; j < gamesize; j++) {
				DrawSurface(gamefieldbmp, tile, ((gamefieldbmp->w / gamesize / 2)* (1 + j * 2)), ((gamefieldbmp->h / gamesize / 2) * (1 + i * 2)));
			}
		}

		cutile.h = tilescolor->h / 8;
		cutile.w = tilescolor->w / 8;

		if (moving) {
			movduration += delta;
			if (movduration >= animtime) {
				movduration = 0;
				moving = 0;
				/*memcpy(lastmov, mov, sizeof(mov_t)*MAX_GAME_SIZE*MAX_GAME_SIZE);*/
				/*memcpy(lastvalue, field, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);*/
				memset(mov, 0, sizeof(mov_t)*MAX_GAME_SIZE*MAX_GAME_SIZE);
				if(!back) GetRandomTile(field, &seed, gamesize);
			}
		}
		char pointstring[32];
		sprintf(pointstring, "POINTS:%d", points);
		DrawString(screen, 20, 32, pointstring, charset, 16);

		for (int i = 0; i < gamesize; i++) {
			for (int j = 0; j < gamesize; j++) {
				if (field[i][j] != 0) {
					char num[32];
					memset(num, 0, sizeof(num));
					k = 1;
					while ((field[i][j]) != pow(2, k)) k++; 
					cutile.x = ((k - 1) % 8) * 120;
					cutile.y = floor(k / 8) * 120;
					SDL_BlitScaled(tilescolor, &cutile, tile, NULL);
					if (mov[i][j].lastinc && moving) sprintf(num, "%d", field[i][j] / 2);
					else sprintf(num, "%d", field[i][j]);
					int size;
					if (strlen(num) < 6) size = tile->w / 5;
					else size = tile->w / strlen(num);
					DrawString(tile, tile->w / 2 - strlen(num) * size / 2, tile->h / 2, num, charset, size); // draw value of tile

					if (movduration < animtime && mov[i][j].lastinc == 1 && moving) {
						double tilespeedx = (mov[i][j].secx - j) / animtime,
							tilespeedy = (mov[i][j].secy - i) / animtime;
						DrawSurface(gamefieldbmp, tile, ((gamefieldbmp->w / gamesize / 2)* (1 + mov[i][j].secx * 2)) - tilespeedx * movduration*(gamefieldbmp->w / gamesize), ((gamefieldbmp->h / gamesize / 2) * (1 + mov[i][j].secy * 2)) - tilespeedy * movduration*(gamefieldbmp->w / gamesize));
					}
					if (mov[i][j].m == 0) { // draw true position of tile
						DrawSurface(gamefieldbmp, tile, ((gamefieldbmp->w / gamesize / 2)* (1 + j * 2)), ((gamefieldbmp->h / gamesize / 2) * (1 + i * 2))); // draw tile
					}
					else if (movduration < animtime && moving) { // animate move of tile
						if (mov[i][j].m) {
							double tilespeedx = (mov[i][j].bx - j) / animtime,
								tilespeedy = (mov[i][j].by - i) / animtime;
							DrawSurface(gamefieldbmp, tile, ((gamefieldbmp->w / gamesize / 2)* (1 + mov[i][j].bx * 2)) - tilespeedx * movduration*(gamefieldbmp->w / gamesize), ((gamefieldbmp->h / gamesize / 2) * (1 + mov[i][j].by * 2)) - tilespeedy * movduration*(gamefieldbmp->w / gamesize));
						}
					}
					/*else {
						mov[i][j].m = 0;
						mov[i][j].bx = 0;
						mov[i][j].by = 0;
						if (mov[i][j].lastinc) field[i][j] *= 2;
						mov[i][j].lastinc = 0;
						mov[i][j].secx = 0;
						mov[i][j].secy = 0;
					}*/
				}
			}
		}


		SDL_BlitScaled(gamefieldbmp, NULL, gamefield, NULL);
		SDL_SetSurfaceAlphaMod(endgame, alpha);
		if (endbuff) {
			SDL_BlitSurface(endgame, NULL, gamefield, NULL);
			char endstring[12] = "GAME OVER";
			DrawString(gamefield, gamefield->w / 2 - strlen(endstring) * 40 / 2, gamefield->h / 2, endstring, charset, 40);
		}

		DrawSurface(screen, gamefield, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 24);
		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (!moving) {
					if (endbuff == 1) {
						printf("END OF GAME");
						return;
					}

					if (event.key.keysym.sym == SDLK_ESCAPE) escape = 1;
					else if (event.key.keysym.sym == SDLK_UP) {
						memcpy(lastlastvalue, field, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
						lastlastpoints = points;
						moving = PushTiles(field, gamesize, 0, mov, &points);
						if (moving) {
							memcpy(lastvalue, lastlastvalue, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
							lastpoints = lastlastpoints;
						}
						back = 0;
					}
					else if (event.key.keysym.sym == SDLK_RIGHT) {
						memcpy(lastlastvalue, field, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
						moving = PushTiles(field, gamesize, 1, mov, &points);
						if (moving) {
							memcpy(lastvalue, lastlastvalue, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
							lastpoints = lastlastpoints;
						}
						back = 0;
					}
					else if (event.key.keysym.sym == SDLK_DOWN) {
						memcpy(lastlastvalue, field, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
						moving = PushTiles(field, gamesize, 2, mov, &points);
						if (moving) {
							memcpy(lastvalue, lastlastvalue, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
							lastpoints = lastlastpoints;
						}
						back = 0;
					}
					else if (event.key.keysym.sym == SDLK_LEFT) {
						memcpy(lastlastvalue, field, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
						moving = PushTiles(field, gamesize, 3, mov, &points);
						if (moving) {
							memcpy(lastvalue, lastlastvalue, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
							lastpoints = lastlastpoints;
						}
						back = 0;
					}
					else if (event.key.keysym.sym == SDLK_s) {
						saveGame(field, &gamesize, &points, &seed);
					}
					else if (event.key.keysym.sym == SDLK_u && !back) {
						stepBack(field, lastvalue, &points, lastpoints, mov, lastmov, gamesize);
						back = 1;
					}
				}
				break;
			}
		}
	}
	FreeAllSurfaces(&gamesurfaces);
}

void LoadGame(char *loadname, SDL_Surface *screen, SDL_Texture *scrtex, SDL_Renderer *renderer, SDL_Surface *charset) {
	FILE *loadfile;
	int gamesize = 0,
		points,
		seed;
	int field[MAX_GAME_SIZE][MAX_GAME_SIZE];
	char path[32];
	sprintf(path, "saves/%s", loadname);
	loadfile = fopen(path, "rb");
	if (loadfile != NULL) {
		for (int i = 0; i < MAX_GAME_SIZE; i++) {
			fread(field[i], sizeof(int), MAX_GAME_SIZE, loadfile);
		}
		fread(&gamesize, sizeof(int), 1, loadfile);
		fread(&points, sizeof(int), 1, loadfile);
		fread(&seed, sizeof(int), 1, loadfile);
		Game(field, gamesize, seed, points, screen, scrtex, renderer, charset);
		fclose(loadfile);
	}
	else printf("LOADING ERROR!!");
}

void NewGame(int gamesize, SDL_Surface *screen, SDL_Texture *scrtex, SDL_Renderer *renderer, SDL_Surface *charset) {
	int field[MAX_GAME_SIZE][MAX_GAME_SIZE];
	memset(field, 0, sizeof(int)*MAX_GAME_SIZE*MAX_GAME_SIZE);
	srand(time(NULL));
	int seed = rand();
	GetRandomTile(field, &seed, gamesize);
	GetRandomTile(field, &seed, gamesize);
	Game(field, gamesize, seed, 0, screen, scrtex, renderer, charset);

};

void ShowSavedGames(SDL_Surface *screen, SDL_Texture *scrtex, SDL_Renderer *renderer, SDL_Surface *charset, SDL_Surface *left, SDL_Surface *right) {
	SDL_Surface *gamefield;
	Surf_list loadsurfaces;
	saves_list_t *list;
	saves_list_t * cur;
	SDL_Event event;
	memset(&loadsurfaces, 0, sizeof(Surf_list));
	int page = 1,
		pages = 0,
		mouseX = 0,
		mouseY = 0;

	list = LoadSaves(&pages);

	gamefield = SDL_CreateRGBSurface(0, FIELD_WIDTH, FIELD_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	InsertSurf(&loadsurfaces, gamefield);
	int bialy = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
	int exit = 0;
	int chosenum = 0;
	SDL_FillRect(gamefield, NULL, bialy);
	SDL_FillRect(screen, NULL, bialy);

	if (list != NULL) {
		while (!exit) {
			int k = 1;
			char num[12];
			char name[32];
			char txt[32];

			SDL_FillRect(gamefield, NULL, bialy);

			if (page == 1) {
				sprintf(num, "%d", k);
				DrawString(gamefield, 10, 24 * k, num, charset, 16);
				DrawString(gamefield, gamefield->w / 4, 24 * k, list->savename, charset, 16);
			}
			k++;
			cur = list;
			while (cur->next != NULL && cur->next->savename != NULL) {
				cur = cur->next;
				if (k > (page - 1)*ONPAGE && k <= page * ONPAGE) {
					sprintf(num, "%d", k);
					DrawString(gamefield, 10, 24 * (k - (page - 1)*ONPAGE), num, charset, 16);
					DrawString(gamefield, gamefield->w / 4, 24 * (k - (page - 1)*ONPAGE), cur->savename, charset, 16);
				}
				k++;
			}
			sprintf(txt, "LOAD SAVE NR:%d", chosenum);
			DrawString(gamefield, 10, gamefield->h / 4 * 3, txt, charset, 16);

			if (page < pages) {
				DrawSurface(gamefield, right, gamefield->w - 32, gamefield->h - 32);
			}
			if (page > 1) {
				DrawSurface(gamefield, left, 32, gamefield->h - 32);
			}

			DrawSurface(screen, gamefield, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 24);
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);


			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE) exit = 1;
					else if (event.key.keysym.sym == SDLK_LEFT && page > 1) page--;
					else if (event.key.keysym.sym == SDLK_RIGHT && page < pages) page++;
					else if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9 && chosenum <= 99999999) {
						chosenum *= 10;
						chosenum += event.key.keysym.sym - 48;
					}
					else if (event.key.keysym.sym == SDLK_BACKSPACE) {
						chosenum -= chosenum % 10;
						chosenum /= 10;
					}
					else if (event.key.keysym.sym == SDLK_RETURN) {
						if (chosenum < k) {
							int l = 1;
							cur = list;
							while (l != chosenum) {
								l++;
								cur = cur->next;
							}
							LoadGame(cur->savename, screen, scrtex, renderer, charset);
							return;
						}
					}
					break;
				case SDL_QUIT:
					exit = 1;
					break;
				case SDL_MOUSEMOTION:
					mouseX = event.motion.x;
					mouseY = event.motion.y;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT) {
						if (mouseY >= (SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + gamefield->h - 48 + 24 && mouseY <= (SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + gamefield->h -16 + 24) {
							if (mouseX >= (SCREEN_WIDTH - SCREEN_HEIGHT) / 2 + 16 && mouseX <= (SCREEN_WIDTH - FIELD_WIDTH) / 2 + 48 && page > 1) {
								page--;
							}
							if (mouseX >= (SCREEN_WIDTH - FIELD_HEIGHT) / 2 + gamefield->w - 48 && mouseX <= (SCREEN_WIDTH - FIELD_WIDTH) / 2 + gamefield->w - 16 && page < pages) {
								page++;
							}
						}
					}
				}
			}
		}
	}
	FreeAllSurfaces(&loadsurfaces);
}

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	int t1, t2, quit, frames, rc, gamesize = 3;
	double delta, worldTime, fpsTimer, fps, distance, etiSpeed;
	SDL_Event event;
	SDL_Surface *screen, *charset;
	SDL_Surface *eti, *gamefield = 0, *eti_final = 0, *tile = 0, *emptile = 0, *tilescolor = 0, *logo = 0, *menu = 0, *sizeslogo, *gamesizes = 0, *left = 0, *right = 0, *newgame = 0, *loadgame = 0, *curgamesize = 0, *scores = 0;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;
	Surf_list surfaces;
	memset(&surfaces, 0, sizeof(Surf_list));
	SDL_Rect cutter;
	memset(&cutter, 0, sizeof(SDL_Rect));

	// okno konsoli nie jest widoczne, jeøeli chcemy zobaczyÊ
	// komunikaty wypisywane printf-em trzeba w opcjach:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// zmieniÊ na "Console"
	// console window is not visible, to see the printf output
	// the option:
	// project -> szablon2 properties -> Linker -> System -> Subsystem
	// must be changed to "Console"
	printf("wyjscie printfa trafia do tego okienka\n");
	printf("printf output goes here\n");

	//saves_list_t *savelist = LoadSaves();

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	// tryb pe≥noekranowy / fullscreen mode
	//rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
	//                                 &window, &renderer);
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&window, &renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "2048");

	curgamesize = SDL_CreateRGBSurface(0, 146, 36, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	InsertSurf(&surfaces, curgamesize);

	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	InsertSurf(&surfaces, screen);


	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);


	// w≥πczenie widocznoúci kursora myszy
	SDL_ShowCursor(SDL_ENABLE);

	// wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if (charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		//FreeSurfaces(screen, tile, gamesize);
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0xFFFFFF);
	InsertSurf(&surfaces, charset);

	eti = SDL_LoadBMP("./eti.bmp");
	if (eti == NULL) {
		printf("SDL_LoadBMP(eti.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, eti);

	gamefield = SDL_LoadBMP("./tlo1.bmp");
	if (gamefield == NULL) {
		printf("SDL_LoadBMP(tlo.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, gamefield);

	emptile = SDL_LoadBMP("./emptile1.bmp");
	if (emptile == NULL) {
		printf("SDL_LoadBMP(emptile1.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, emptile);

	tilescolor = SDL_LoadBMP("./tilescolor.bmp");
	if (tilescolor == NULL) {
		printf("SDL_LoadBMP(emptile1.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, tilescolor);

	logo = SDL_LoadBMP("./logo.bmp");
	if (logo == NULL) {
		printf("SDL_LoadBMP(logo.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, logo);

	menu = SDL_LoadBMP("./menu1.bmp");
	if (menu == NULL) {
		printf("SDL_LoadBMP(menu.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, menu);

	sizeslogo = SDL_LoadBMP("./sizeslogo.bmp");
	if (sizeslogo == NULL) {
		printf("SDL_LoadBMP(sizeslogo.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	}
	InsertSurf(&surfaces, sizeslogo);

	gamesizes = SDL_LoadBMP("./gamesizes.bmp");
	if (gamesizes == NULL) {
		printf("SDL_LoadBMP(gamesizes.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, gamesizes);

	right = SDL_LoadBMP("./right.bmp");
	if (right == NULL) {
		printf("SDL_LoadBMP(right.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, right);

	left = SDL_LoadBMP("./left.bmp");
	if (left == NULL) {
		printf("SDL_LoadBMP(left.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, left);

	newgame = SDL_LoadBMP("./newgame.bmp");
	if (newgame == NULL) {
		printf("SDL_LoadBMP(newgame.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, newgame);

	loadgame = SDL_LoadBMP("./loadgame.bmp");
	if (loadgame == NULL) {
		printf("SDL_LoadBMP(loadgame.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, loadgame);

	scores = SDL_LoadBMP("./scores.bmp");
	if (scores == NULL) {
		printf("SDL_LoadBMP(loadgame.bmp) error: %s\n", SDL_GetError());
		FreeAllSurfaces(&surfaces);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	InsertSurf(&surfaces, scores);

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int bialy = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
	int tlo = SDL_MapRGB(screen->format, 0xA5, 0xCC, 0xAA);
	int ciemnyszary = SDL_MapRGB(screen->format, 0x5A, 0x5E, 0x4A);
	int szary = SDL_MapRGB(screen->format, 0x7B, 0x81, 0x65);

	t1 = SDL_GetTicks();

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;
	distance = 0;
	etiSpeed = 1;
	//SDL_BlitScaled(eti, NULL, eti,NULL);
	SDL_BlitScaled(emptile, NULL, tile, NULL);

	int mouseX = 0, mouseY = 0;
	time_t timer;
	while (!quit) {
		t2 = SDL_GetTicks();

		// w tym momencie t2-t1 to czas w milisekundach,
		// jaki uplyna≥ od ostatniego narysowania ekranu
		// delta to ten sam czas w sekundach
		// here t2-t1 is the time in milliseconds since
		// the last screen was drawn
		// delta is the same time in seconds
		delta = (t2 - t1) * 0.001;
		t1 = t2;

		worldTime += delta;

		distance += etiSpeed * delta;
		if (worldTime *FPS >= 1) {

			SDL_FillRect(screen, NULL, bialy);
			//SDL_FillRect(gamefield, NULL, ciemnyszary);
			//SDL_FillRect(tile, NULL, szary);
			/*for (int i = 0; i < GAME_SIZE; i++) {
				for (int j = 0; j < GAME_SIZE; j++) {
					DrawSurface(gamefield, tile, FIELD_WIDTH / GAME_SIZE / 2 * (1 + i * 2), FIELD_HEIGHT / GAME_SIZE / 2 * (1 + j * 2));
				}

			}*/
			cutter.x = 0;
			cutter.y = (gamesize - 3) * 36;
			cutter.w = 146;
			cutter.h = 36;
			SDL_FillRect(curgamesize, 0, bialy);
			SDL_BlitSurface(gamesizes, &cutter, curgamesize, NULL);

			DrawSurface(gamefield, logo, MENU_WIDTH / 2, 80);
			DrawSurface(gamefield, menu, MENU_WIDTH / 2, 260);
			DrawSurface(gamefield, sizeslogo, MENU_WIDTH / 2, 170);
			DrawSurface(gamefield, curgamesize, MENU_WIDTH / 2, 210);
			DrawSurface(gamefield, left, MENU_WIDTH / 2 - 50, 210);
			DrawSurface(gamefield, right, MENU_WIDTH / 2 + 50, 210);
			DrawSurface(gamefield, newgame, MENU_WIDTH / 2, 250);
			DrawSurface(gamefield, loadgame, MENU_WIDTH / 2, 300);
			DrawSurface(gamefield, scores, MENU_WIDTH / 2, 350);
			DrawSurface(screen, gamefield,
				SCREEN_WIDTH / 2,
				SCREEN_HEIGHT / 2);

			/*DrawSurface(screen, eti,
				SCREEN_WIDTH / 2 + sin(distance) * SCREEN_HEIGHT / 3,
				SCREEN_HEIGHT / 2 + cos(distance) * SCREEN_HEIGHT / 3);*/
			worldTime = 0;
		}
		//DrawRectangle(screen, SCREEN_WIDTH / 4 + cos(distance*3)*150, 100 + SCREEN_HEIGHT / 4 + sin(distance/2)*150, 100, 100, zielony, czerwony);
		//DrawLine(screen, SCREEN_WIDTH / 4 + cos(distance * 3) * 150 + 100, 200 + SCREEN_HEIGHT / 4 + sin(distance / 2) * 150, 50*(cos(distance * 3) + 1), 1, 0, niebieski);
		fpsTimer += delta;
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
			timer = time(NULL);
		};

		// tekst informacyjny / info text
		DrawRectangle(screen, 2, 2, SCREEN_WIDTH - 8, 36, czerwony, niebieski);
		//            "template for the second project, elapsed time = %.1lf s  %.0lf frames / s"
		sprintf(text, "Jakub Lecki, czas trwania = %.1lf s  %.0lf klatek / s", worldTime, fps);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset, 8);
		//	      "Esc - exit, \030 - faster, \031 - slower"
		//sprintf(text, "Esc - wyjscie, \030 - przyspieszenie, \031 - zwolnienie");
		//DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);
		sprintf(text, "X: %d, Y: %d", mouseX, mouseY);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset, 8);

		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		//		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		// obs≥uga zdarzeÒ (o ile jakieú zasz≥y) / handling of events (if there were any)
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
				else if (event.key.keysym.sym == SDLK_n) NewGame(gamesize, screen, scrtex, renderer, charset);
				else if (event.key.keysym.sym == SDLK_LEFT && gamesize > 3) gamesize--;
				else if (event.key.keysym.sym == SDLK_RIGHT && gamesize < 8) gamesize++;
				else if (event.key.keysym.sym == SDLK_l) ShowSavedGames(screen, scrtex, renderer, charset,left,right);
				break;
			case SDL_KEYUP:
				etiSpeed = 1.0;
				break;
			case SDL_QUIT:
				quit = 1;
				break;
			case SDL_MOUSEMOTION:
				mouseX = event.motion.x;
				mouseY = event.motion.y;
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (event.button.button == SDL_BUTTON_LEFT) {
					if (mouseX >= SCREEN_WIDTH / 2 - newgame->w / 2 && mouseX <= SCREEN_WIDTH / 2 + newgame->w / 2) {
						if (mouseY >= ((SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + 260 - newgame->h / 2) && mouseY <= ((SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + 260 + newgame->h / 2)) {
							printf("NEW GAME !!\n");
							NewGame(gamesize, screen, scrtex, renderer, charset);
						}
						if (mouseY >= ((SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + 310 - loadgame->h / 2) && mouseY <= ((SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + 310 + loadgame->h / 2)) {
							printf("SHOWING SAVED GAMES\n");
							ShowSavedGames(screen, scrtex, renderer, charset,left,right);
						}
						if (mouseY >= ((SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + 360 - scores->h / 2) && mouseY <= ((SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + 360 + scores->h / 2)) {
							printf("SHOWING SAVED SCORES\n");
						}
					}
					if (mouseY >= ((SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + 220 - left->h / 2) && mouseY <= ((SCREEN_HEIGHT - FIELD_HEIGHT) / 2 + 220 + left->h / 2)) {
						if (mouseX >= SCREEN_WIDTH / 2 - 50 - left->w / 2 && mouseX <= SCREEN_WIDTH / 2 - 50 + left->w / 2 && gamesize > 3) gamesize--;
						else if (mouseX >= SCREEN_WIDTH / 2 + 50 - right->w / 2 && mouseX <= SCREEN_WIDTH / 2 + 50 + right->w / 2 && gamesize < 8) gamesize++;
					}
					break;
				}
			};
		};
		frames++;
	}
	FreeAllSurfaces(&surfaces);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
	return 1;
}