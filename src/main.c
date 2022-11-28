
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <raylib.h>

#include "1802core.h"
#include "keys.h"

#define min(a,b) ((a)<(b)?(a):(b))

int main(int argc, char *argv[]) {
	Emu1802 *emu;
	Image ScreenImage, CharacterImage;
	Texture2D ScreenTexture;
	// Font MemViewFont;
	// uint16_t memoryViewAddress;
	int ScreenWidth, ScreenHeight, keycode;
	bool iskeypressed;

	if (argc <= 1) {
		printf("Usage: %s image.rom\n", argv[0]);
		return 0;
	}
	
	SetTraceLogLevel(LOG_WARNING);

	CharacterImage = LoadImage("data/charmap.png");
	ImageFormat(&CharacterImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
	// MemViewFont = LoadFont("data/RobotoMono-Regular.ttf");

	if ((emu = emu_Init1802()) == NULL) {
		printf("Failed to init emulator: not enough memory!");
		return 1;
	}
	
	if (emu_LoadRomFile(emu, argv[1])) {
		printf("Failed to locate rom file: %s\n", argv[1]);
		return -1;
	}
	
	InitWindow(512, 512, "Beck's 1802 Emulator");
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetWindowMinSize(512, 512);
	SetExitKey(-1);
	SetTargetFPS(60);

	ScreenImage.data = malloc(512 * 512 * sizeof(uint32_t));
	ScreenImage.width = 512;
	ScreenImage.height = 512;
	ScreenImage.mipmaps = 1;
	ScreenImage.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

	keycode = 0;
	iskeypressed = false;
	// memoryViewAddress = 0;

	while (!WindowShouldClose()) {
		uint16_t terminalAddr;
		ScreenWidth = GetScreenWidth();
		ScreenHeight = GetScreenHeight();
		terminalAddr = emu_ReadWord(emu, EMU_TERMINAL_ADDR_LOC);
		for (int y=0; y<50; y++) {
			for (int x=0; x<64; x++) {
				for (int y2=0; y2<8; y2++) {
					for (int x2=0; x2<8; x2++) {
						Vector4 color;
						uint8_t ch, colorbyte, cx, cy;
						ch = emu_ReadByte(emu, terminalAddr + (x + y*64) * 2);
						if (ch < ' ')
							ch = ' ';
						ch -= ' ';
						cx = (ch % 13) * 8 + x2;
						cy = (ch / 13) * 8 + y2;
						colorbyte = emu_ReadByte(emu, terminalAddr + (x + y*64) * 2 + 1);
						color = ColorNormalize(GetPixelColor(&(((uint32_t*)CharacterImage.data)[cx + cy*128]), PIXELFORMAT_UNCOMPRESSED_R8G8B8A8));
						color.x	*= ((float)((colorbyte >> 4) & 3)) / 3.0;
						color.y	*= ((float)((colorbyte >> 2) & 3)) / 3.0;
						color.z	*= ((float)(colorbyte & 3)) / 3.0;
						SetPixelColor(&(((uint32_t*)ScreenImage.data)[x*8 + x2 + (y*10 + y2)*512]), ColorFromNormalized(color), PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
						// if (x == 0 && y == 0)
							// printf("%d, %d, %d, %d, %f, %f, %f, %f\n", ch, colorbyte, cx, cy, color.x, color.y, color.z, color.w);
					}
				}
			}
		}

		BeginDrawing();
		ClearBackground(BLACK);
		ScreenTexture = LoadTextureFromImage(ScreenImage);
		DrawTextureEx(ScreenTexture, (Vector2){0, 0}, 0, min(ScreenWidth, ScreenHeight) / 512.0f, WHITE);
		// if (ScreenWidth > 1156) {
			// uint8_t value;
			// char valuetext[4];
			// for (int y=0; y<16; y++) {
				// for (int x=0; x<16; x++) {
					// value = emu_ReadByte(emu, memoryViewAddress+x+y*32);
					// sprintf(&valuetext, "%X%X", value>>4, value&0xF);
					// DrawTextEx(GetFontDefault(), valuetext, (Vector2){514+x*32, 2+y*20}, 16, 1, (Color) {180, 180, 180, 255});
					// sprintf(&valuetext, "%c", (char)value);
					// DrawTextEx(GetFontDefault(), valuetext, (Vector2){1026+x*10, 2+y*20}, 16, 1, (Color) {180, 180, 255, 255});
				// }
			// }
		// }
		EndDrawing();
		UnloadTexture(ScreenTexture);


		if (iskeypressed) {
			if (!IsKeyDown(keycode)) {
				keycode = 0;
				iskeypressed = false;
				emu_Input(emu, 0);
			}
			// printf("%d\n", KEY_MAPPING[KEY_ENTER]);
		} else {
			for (unsigned int j=1; j<sizeof(KEY_MAPPING); j++) {
				if (IsKeyDown(j)) {
					iskeypressed = true;
					emu_Input(emu, KEY_MAPPING[(keycode = j)]);
					goto step;
				}
			}
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
			emu_Reset(emu);
			continue;
		}

		step:;
		emu_Step(emu, (int)(4194304 * GetFrameTime()));
	}
	free(ScreenImage.data);
	UnloadImage(CharacterImage);
	CloseWindow();
	return 0;
}

