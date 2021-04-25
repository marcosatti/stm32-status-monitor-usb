#include <stdio.h>
#include <stdbool.h>
#include <ssd1306.h>
#include "cmsis_os.h"
#include "display.h"
#include "main.h"
#include "status.pb.h"

extern osMessageQueueId_t messageQueueHandle;
    
static char str_buffer[32];

void display_task_main(void) {
    Status status;

    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Ready", Font_7x10, White);
    ssd1306_UpdateScreen();

    printf("Info: SSD1306 display initialized\n");

    while (true) {
        if (osMessageQueueGet(messageQueueHandle, &status, NULL, osWaitForever) != osOK) {
            printf("Fatal: Failed to get a message from the queue\n");
            Error_Handler();
        }

        ssd1306_Fill(Black);

        sprintf(str_buffer, "CPU: %lu%%", status.cpu);
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString(str_buffer, Font_7x10, White);

        sprintf(str_buffer, "Power: %luW", status.cpu_power);
        ssd1306_SetCursor(0, 10);
        ssd1306_WriteString(str_buffer, Font_7x10, White);

        sprintf(str_buffer, "Temp: %luoC", status.temperature);
        ssd1306_SetCursor(0, 20);
        ssd1306_WriteString(str_buffer, Font_7x10, White);

        sprintf(str_buffer, "Network: %luMiB/s", status.network);
        ssd1306_SetCursor(0, 30);
        ssd1306_WriteString(str_buffer, Font_7x10, White);

        sprintf(str_buffer, "Users: %lu", status.samba_users_connected);
        ssd1306_SetCursor(0, 40);
        ssd1306_WriteString(str_buffer, Font_7x10, White);

        sprintf(str_buffer, "Files: %lu", status.samba_files_opened);
        ssd1306_SetCursor(0, 50);
        ssd1306_WriteString(str_buffer, Font_7x10, White);

        ssd1306_UpdateScreen();
    }
}
