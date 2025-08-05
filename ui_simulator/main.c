
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" \
                            issue*/
#include "lv_drivers/sdl/sdl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/lvgl.h"
#include <SDL2/SDL.h>

#include "device_setting.h"
#include "gui.h"
#include "gui_api.h"
#include "gui_framework.h"
#include "gui_views.h"

// ADDED BY PEN
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include "eapdu_protocol_parser.h"
#include "virtual_usb.h"  // EXTRA HEADER FILE FOUND IN ui_simulator SUB-DIRECTORY
int listening = -1;

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init(void);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

//////////////////////////////
//  FUNCTIONS ADDED BY PEN  //
//////////////////////////////

void toDottedIP(uint32_t thisV4, char*outChars) {
    uint8_t b0 = (uint8_t)(thisV4 & 0xff);
    uint8_t b1 = (uint8_t)((thisV4 >> 8) & 0xff);
    uint8_t b2 = (uint8_t)((thisV4 >> 16) & 0xff);
    uint8_t b3 = (uint8_t)((thisV4 >> 24) & 0xff);
    sprintf(outChars, "%d.%d.%d.%d", b0, b1, b2, b3);
}

void ReceiveVirtualUSB(int thisSocket) {
    g_clientSocket = thisSocket;  // FROM virtual_usb.h, SAVE FOR USE IN eapdu_protocol_parser.c
    printf("BY PEN: main:ReceiveVirtualUSB g_clientSocket=%d\n", g_clientSocket);

    uint8_t dataRequest[4096];
    uint8_t packetBuffer[64];
    uint8_t thisCLA;
    uint16_t thisINS;
    uint16_t thisPacketCount;
    uint16_t thisPacketNumber;
    uint16_t thisRequestID;
    uint32_t dataPos = 0;
    uint8_t thisLen = 0;

  GET_MORE_DATA:
    thisLen = recv(thisSocket, packetBuffer, 64, 0);

    if (thisLen > 0) {
        thisCLA = packetBuffer[0];
        
        thisINS = extract_16bit_value((uint8_t*)packetBuffer, OFFSET_INS);
        printf("BY PEN: main:ReceiveVirtualUSB thisINS=%d\n", thisINS);
          
        thisPacketCount = extract_16bit_value((uint8_t*)packetBuffer, OFFSET_INS);
        printf("BY PEN: main:ReceiveVirtualUSB thisPacketCount=%d\n", thisPacketCount);

        thisPacketNumber = extract_16bit_value((uint8_t*)packetBuffer, OFFSET_P1);
        printf("BY PEN: main:ReceiveVirtualUSB thisPacketCount=%d\n", thisPacketCount);

        thisPacketNumber = extract_16bit_value((uint8_t*)packetBuffer, OFFSET_P2);
        printf("BY PEN: main:ReceiveVirtualUSB thisPacketNUmber=%d\n", thisPacketNumber);        
      
        thisRequestID = extract_16bit_value((uint8_t*)packetBuffer, OFFSET_LC);
        printf("BY PEN: main:ReceiveVirtualUSB thisPacketNUmber=0x%04x\n", thisRequestID);

        memcpy((uint8_t*)dataRequest + dataPos, (uint8_t*)packetBuffer + OFFSET_CDATA, thisLen - 9);
        dataPos += (thisLen - 9);

        if ((thisPacketCount - thisPacketNumber) > 1) {
            printf("BY PEN: main:ReceiveVirtualUSB getting more data\n");
        }
          
    }
              
    printf("BY PEN: mainReceiveVirtualUSB building thisRequest dataPos=%d\n", dataPos);
    EAPDURequestPayload_t *thisRequest = (EAPDURequestPayload_t *)SRAM_MALLOC(sizeof(EAPDURequestPayload_t));
    if (dataPos > 0) {
        thisRequest->data = (uint8_t *)dataRequest;
    }
    thisRequest->dataLen = dataPos;
    thisRequest->commandType = thisINS;
    thisRequest->requestID = thisRequestID;

    EApduRequestHandler(thisRequest);

    SRAM_FREE(thisRequest);
    return;
}

void * my_thread_function(void *arg) {
    printf("Hello from the new thread\n");

    // GET OUR LOCAL IP ADDRESS
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    memset(host, 0, NI_MAXHOST);
    if (getifaddrs(&ifaddr) == -1) {
        printf("BY PEN: main:my_thread_function getifaddres FAILED\n");
        pthread_exit(NULL);
    } else {
        printf("BY PEN: main:my_thread_function getifaddrs OK\n");
    }
        
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        family = ifa->ifa_addr->sa_family;

      if (family == AF_INET) {
          s= getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnamindo() failed: %s\n",gai_strerror(s));
                pthread_exit(NULL);
            }
            if (strcmp(ifa->ifa_name, "lo0") != 0) {  // EXCLUDE LOOPBACK
                break;
            }
        }  
    }

    freeifaddrs(ifaddr);
    char *ipAddress = (char *) host;
    printf("BY PEN: main:my_thread_function ipAddress=%s\n", ipAddress);

  // START LISTENING SOCKET
    listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1) {
        printf("Can't create listening socket\n");
        pthread_exit(NULL);  
    } else {
        printf("BY PEN: main:my_thread_function listening socket OK\n");
    }

    struct sockaddr_in hint;
    hint.sin_family = AF_INET;

  // FOR MAC FIREWALL
  // System Setting->network->Firewall
  // EITHER TURN OFF OF ->Firewall Opiont, "+" BUTTON TO SELECT ALL AND "Allow Incoming Connections"
  // THEN "Done" TO SAVE CHANGES

    hint.sin_port = htons(81);
    hint.sin_addr.s_addr = INADDR_ANY;
    if (bind(listening, (struct sockaddr *)&hint, sizeof(hint)) < 0) {
        printf("BY PEN: main:my_thread_function bind FAILED\n");
        close(listening);
        listening = -1;
        pthread_exit(NULL);
    } else {
        printf("BY PEN: main:my_thread_function bind OK\n");
    }

    if (listen(listening, SOMAXCONN) < 0) {
        printf("BY PEN: main:my_thread_function listen FAILED\n");
        close(listening);
        listening = -1;
         pthread_exit(NULL);
    } else {
        printf("BY PEN: main:my_thread_function listen OK\n");
    }

    struct sockaddr_in client;
    socklen_t clientSize = sizeof(client);
    int clientSocket;

    while(1) {
        clientSocket = accept(listening, (struct sockaddr *)&client, &clientSize);
        if (clientSocket == -1) {
            printf("Error accepting client connection.\n");
        } else {
            char host[NI_MAXHOST];  // Client's remote name
            char service[NI_MAXSERV];  // Port (service) the client is connected to
            char* clientIP[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            memset(host, 0, NI_MAXHOST);
            memset(service, 0, NI_MAXSERV);
            if (getnameinfo((struct sockaddr *)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
                printf("  Connection from host = %s\n", (char*)host);
                toDottedIP(client.sin_addr.s_addr, (char*)clientIP);
                printf("  clinetIP = %s\n", (char*)clientIP);
                int clientPort = ntohs(client.sin_port);
                printf("  clientPort = %d\n", clientPort);

                ReceiveVirtualUSB(clientSocket);
            } 
            close(clientSocket);
            g_clientSocket = 0;
        }
    }
    pthread_exit(NULL);  // WILL NOT BE REACHED
}

int hexstr_to_uint8_array(const char *hexstr, uint8_t *buf, size_t buf_len) {
    int count = 0;
    while (*hexstr && count < buf_len) {
        unsigned int byte;
        if (sscanf(hexstr, "%2x", &byte) != 1) {
            break;
        }
        buf[count++] = byte;
        hexstr += 2;
    }
    return count;
}

int main(int argc, char **argv)
{
    (void)argc; /*Unused*/
    (void)argv; /*Unused*/

    printf("start");

    /*Initialize LVGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LVGL*/
    hal_init();

    DeviceSettingsInit();
    GuiStyleInit();
    LanguageInit();

    GuiFrameOpenView(&g_initView);
    //  lv_example_calendar_1();
    //  lv_example_btnmatrix_2();
    //  lv_example_checkbox_1();
    //  lv_example_colorwheel_1();
    //  lv_example_chart_6();
    //  lv_example_table_2();
    //  lv_example_scroll_2();
    //  lv_example_textarea_1();
    //  lv_example_msgbox_1();
    //  lv_example_dropdown_2();
    //  lv_example_btn_1();
    //  lv_example_scroll_1();
    //  lv_example_tabview_1();
    //  lv_example_tabview_1();
    //  lv_example_flex_3();
    //  lv_example_label_1();

  // ADDED BY PEN, CREATE THE NEW LISTENING THREAD
    pthread_t new_thread;
    if (pthread_create(&new_thread, NULL, my_thread_function, NULL) != 0) {
        printf("BY PEN: main could not create thread\n");
        return -1;
    } else {
        printf("BY PEN: main thread created\n");
    }
        
    while (1)
    {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        lv_timer_handler();
        usleep(5 * 1000);
    }

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static void hal_init(void)
{
    /* Use the 'monitor' driver which creates window on PC's monitor to simulate a
     * display*/
    sdl_init();

    /*Create a display buffer*/
    static lv_disp_draw_buf_t disp_buf1;
    static lv_color_t buf1_1[SDL_HOR_RES * 100];
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, NULL, SDL_HOR_RES * 100);

    /*Create a display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv); /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = SDL_HOR_RES;
    disp_drv.ver_res = SDL_VER_RES;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    lv_theme_t *th = lv_theme_default_init(
        disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
        LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, th);

    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);

    /* Add the mouse as input device
     * Use the 'mouse' driver which reads the PC's mouse*/
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse
     * position and state*/
    indev_drv_1.read_cb = sdl_mouse_read;
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);

    static lv_indev_drv_t indev_drv_2;
    lv_indev_drv_init(&indev_drv_2); /*Basic initialization*/
    indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_2.read_cb = sdl_keyboard_read;
    lv_indev_t *kb_indev = lv_indev_drv_register(&indev_drv_2);
    lv_indev_set_group(kb_indev, g);

    static lv_indev_drv_t indev_drv_3;
    lv_indev_drv_init(&indev_drv_3); /*Basic initialization*/
    indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
    indev_drv_3.read_cb = sdl_mousewheel_read;
    lv_indev_t *enc_indev = lv_indev_drv_register(&indev_drv_3);
    lv_indev_set_group(enc_indev, g);
}
