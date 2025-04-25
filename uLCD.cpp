/*
 * uLCD Class
 * 
 * This is a psuedo-library for 4D Systems's Goldelox Processor powered displays.
 * The library only implements features that this game engine uses.
 * These features include most text features and most graphics commands.
 *
 * This library is loosely related to the uLCD_4DGL library, but is better optimized
 * and is updated to MBed OS 6
 *
 * (c) Daniel Cooper 2021
 */

#include "uLCD.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sstream>

#include "collectionCommon.hpp"

/*
     * With the baud rate, the library will always use 9600 for initial communication with the uLCD
     * It sets the new baud rate after starting initial communication.
     */
    uLCD::uLCD(PinName tx, PinName rx, PinName reset, uLCDBaud baud) : serial(tx, rx, 9600), resetSignal(reset), waitFunction(nullptr) {
        
        this->resetSignal.write(true);
        this->delayedWritePending = false;

        this->reset();
        
        //Clearing the screen
        this->cls();

        this->awaitResponse();
        
        //Set baud

        char buf[4];
        buf[0] = 0x0;
        buf[1] = 0xB;
        this->addIntToBuf(&buf[2], (int)baud);

        this->serial.checkBufferFree();
        this->serial.write(buf, 4);

        int baudv = 9600;
        switch (baud) {
        case BAUD_9600:
            baudv = 9600;
            break;
        case BAUD_56000:
            baudv = 56000;
            break;
        case BAUD_115200:
            baudv = 115200;
            break;
        case BAUD_128000:
            baudv = 128000;
            break;
        case BAUD_300000:
            baudv = 300000;
            break;
        case BAUD_600000:
            baudv = 600000;
            break;
        case BAUD_1000000:
            baudv = 1000000;
            break;
        case BAUD_1500000:
            baudv = 1500000;
            break;
        }

        this->serial.sync();

        this->serial.setBaud(baudv);

        //cls will await for the response before advancing since all commands are asynchonous
        this->cls(); //Just to send a command post-baud change

        this->setTextColor(0xFFFF); //making text foreground white
    }

    void uLCD::addIntToBuf(char* buf, int v) {
        int16_t sv = (int16_t)v;
        buf[0] = (unsigned char)(sv >> 8);
        buf[1] = (unsigned char)sv;
    }

    //General functions

    void uLCD::awaitResponse() {
        if (this->bypassAwait) {
            this->bypassAwait = false;
            return;
        }

        while (delayedWritePending);

        this->serial.sync();

        char resp = 0;
        while(!resp) {
            this->serial.read(&resp, sizeof(char));
        }

        if (resp != 0x6) {
            printf("Display did not respond with Status OK.\n");
        }

        this->serial.flushReceiving();
    }

    /** Clear the screen and fills the screen with the set background color. Defaults to black (0x0)*/
    void uLCD::cls() {
        this->awaitResponse();
        char* buf = (char*) malloc_safe(2);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xD7;
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 2);
    }

    /** Resets the screen, blocks for 3 seconds */
    void uLCD::reset() {
        this->resetSignal.write(false);
        wait_us(50);
        this->resetSignal.write(true);
        wait_us(3000000);
        this->bypassAwait = true;
    }

    void uLCD::writeBack() {
        if (this->delayFreeable) {
            this->serial.writeAndFree(this->delayBuffer, this->delaySize);
        } else {
            this->serial.write(this->delayBuffer, this->delaySize);
        }

        delay.detach();

        this->delayedWritePending = false;
    }

    void uLCD::waitToWrite(void* buffer, int size, int delay_us, bool freeable) {
        this->delayedWritePending = true;
        this->delayBuffer = buffer;
        this->delaySize = size;
        this->delayFreeable = freeable;
        //Of course, the non-deprecated option doesn't exist
        delay.attach(callback(this, &uLCD::writeBack), std::chrono::microseconds(delay_us));
    }

    //Text Functions

    /**
     * Sets text foreground color
     * @param color The 4DGL color to set the text foreground color
     */
    void uLCD::setTextColor(uint16_t color) {
        this->awaitResponse();
        char* buf = (char*) malloc_safe(4);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0x7F;
        
        //color already accounts for the big-endian style for transmission
        buf[2] = color;
        buf[3] = color >> 8;

        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 4);
    }

    /**
     * Sets text background color
     * @param color The 4DGL color to set the text background color
     */
    void uLCD::setTextBackground(uint16_t color) {
        this->awaitResponse();
        char* buf = (char*) malloc_safe(4);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0x7E;
        
        //color already accounts for the big-endian style for transmission
        buf[2] = color;
        buf[3] = color >> 8;

        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 4);
    }

    /**
     * Sets the font scalar values, scales by integral amounts only.
     * @param width The width scalar for the font
     * @param height The height scalar for the font
     */
    void uLCD::setFontSize(int width, int height) {
        //sending two separate commands
        this->awaitResponse();

        char* buf = (char*) malloc_safe(4);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0x7C;
        this->addIntToBuf(&buf[2], width);
        this->serial.checkBufferFree();
        this->serial.write(buf, 4);

        this->awaitResponse();

        buf[0] = 0xFF;
        buf[1] = 0x7B;
        this->addIntToBuf(&buf[2], height);
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 4);
    }

    /**
     * Sets whether the next text sent to the display gets bolded.
     * IMPORTANT: Text format commands are reset after the next text sent.
     * @param bold True to bold next text, false to reset manually
     */
    void uLCD::setTextBold(bool bold) {
        this->awaitResponse();
        char* buf = (char*) malloc_safe(4);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0x76;
        buf[2] = 0x0;
        buf[3] = bold;

        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 4);
    }

    /**
     * Sets whether the next text sent to the display gets italicized
     * IMPORTANT: Text format commands are reset after the next text sent.
     * @param italic True to italicize next text, false to reset manually
     */
    void uLCD::setTextItalic(bool italic) {
        this->awaitResponse();
        char* buf = (char*) malloc_safe(4);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0x75;
        buf[2] = 0x0;
        buf[3] = italic;

        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 4);
    }

    /**
     * Sets whether the next text sent to the display gets inverted colors
     * IMPORTANT: Text format commands are reset after the next text sent.
     * @param invert True to invert colors for next text, false to reset manually
     */
    void uLCD::setTextInverted(bool invert) {
        this->awaitResponse();
        char* buf = (char*) malloc_safe(4);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0x74;
        buf[2] = 0x0;
        buf[3] = invert;

        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 4);
    }

    /**
     * Sets whether the next text sent to the display gets underlined
     * IMPORTANT: Text format commands are reset after the next text sent.
     * @param underline True to underline next text, false to reset manually
     */
    void uLCD::setTextUnderline(bool underline) {
        this->awaitResponse();
        char* buf = (char*) malloc_safe(4);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0x73;
        buf[2] = 0x0;
        buf[3] = underline;

        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 4);
    }

    /**
     * Prints one char to the display at the location set with locate()
     * @param c The character to print
     */
    void uLCD::print(char c) {
        this->awaitResponse();
        char* buf = (char*) malloc_safe(4);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xFE;
        buf[2] = 0x0;
        buf[3] = c;

        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 4);
    }

    /**
     * Prints a string to the lcd at the location set with locate()
     * IMPORTANT: Will not automatically line wrap, this must be done manually
     * @param str The null-terminated string to print
     */
    void uLCD::print(char* str) {
        this->awaitResponse();
        char buf[2];
        buf[0] = 0x0;
        buf[1] = 0x6;

        this->serial.checkBufferFree();
        this->serial.write(buf, 2);
        this->serial.sync();

        wait_us(25); //delay to process the command header

        //getting string length
        char length = strlen(str);

        //This is a very expensive operation for the display, so will have to slow down by
        //sending only one byte at a time

        char* bufP = str;

        for (int i = 0; length + 1 > i; i++) {
            this->serial.write(bufP++, 1);
            this->serial.sync();
            if (length > 16) wait_us(40);
        }
    }

    /**
     * Prints a formatted string to the lcd at the location set with locate()
     * IMPORTANT: Will not automatically line wrap, this must be done manually
     * @param format A string with the appropriate formatting codes
     */
    void uLCD::printf(const char* str, ...) {
        va_list args;
        va_start(args, str);

        this->awaitResponse();

        char buf[258];
        buf[0] = 0x0;
        buf[1] = 0x6;

        //Generating final string
        int length = std::vsnprintf(&buf[2], 256, str, args);

        //This is a very expensive operation for the display, so will have to slow down by
        //sending only one byte at a time

        char* bufP = buf;

        this->serial.checkBufferFree();

        for (int i = 0; length + 3 > i; i++) {
            this->serial.write(bufP++, 1);
            if (length > 14) wait_us(40);
        }

        va_end(args);
    }

    /**
     * Locates the text cursor to the given text coordinates.
     * IMPORTANT: Text coordinates are not the same as screen coordinates
     * @param x The x text coordinate
     * @param y The y text coordinate
     */
    void uLCD::locate(int x, int y) {
        this->awaitResponse();

        char* buf = (char*) malloc_safe(6);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xE4;
        this->addIntToBuf(&buf[2], y);
        this->addIntToBuf(&buf[4], x);
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 6);
    }

    //Graphics Functions

    /**
     *  Draws a circle with no fill centered at (x, y)
     * @param x The x pixel coordinate
     * @param y The y pixel coordinate
     * @param r The radius measured in pixels
     * @param color The 4DGL color for the circle's outline
     */
    void uLCD::drawCircle(int x, int y, int radius, uint16_t color) {
        this->awaitResponse();

        char* buf = (char*) malloc_safe(10);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xCD;
        this->addIntToBuf(&buf[2], x);
        this->addIntToBuf(&buf[4], y);
        this->addIntToBuf(&buf[6], radius);
        buf[8] = color;
        buf[9] = color >> 8;
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 10);
    }

    /**
     *  Draws a filled circle centered at (x, y)
     * @param x The x pixel coordinate
     * @param y The y pixel coordinate
     * @param r The radius measured in pixels
     * @param color The 4DGL color for the circle's fill
     */
    void uLCD::drawCircleFilled(int x, int y, int radius, uint16_t color) {
        this->awaitResponse();

        char* buf = (char*) malloc_safe(10);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xCC;
        this->addIntToBuf(&buf[2], x);
        this->addIntToBuf(&buf[4], y);
        this->addIntToBuf(&buf[6], radius);
        buf[8] = color;
        buf[9] = color >> 8;
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 10);
    }

    /**
     * Draws a triange with no fill
     * @param x1 The x coordinate of the first vertex
     * @param y2 The y coordinate of the first vertex
     * @param x2 The x coordinate of the second vertex
     * @param y2 The y coordinate of the second vertex
     * @param x3 The x coordinate of the third vertex
     * @param y3 The y coordinate of the third vertex
     * @param color The 4DGL color for the triangle's outline
     */
    void uLCD::drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color) {
        this->awaitResponse();

        char* buf = (char*) malloc_safe(16);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xC9;
        this->addIntToBuf(&buf[2], x1);
        this->addIntToBuf(&buf[4], y1);
        this->addIntToBuf(&buf[6], x2);
        this->addIntToBuf(&buf[8], y2);
        this->addIntToBuf(&buf[10], x3);
        this->addIntToBuf(&buf[12], y3);
        buf[14] = color;
        buf[15] = color >> 8;
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 16);
    }

    /**
     * Draws a line between the two vertices
     * @param x1 The x coordinate of the first vertex
     * @param y2 The y coordinate of the first vertex
     * @param x2 The x coordinate of the second vertex
     * @param y2 The y coordinate of the second vertex
     * @param color The 4DGL color for the line
     */
    void uLCD::drawLine(int x1, int y1, int x2, int y2, uint16_t color) {
        this->awaitResponse();

        char* buf = (char*) malloc_safe(12);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xD2;
        this->addIntToBuf(&buf[2], x1);
        this->addIntToBuf(&buf[4], y1);
        this->addIntToBuf(&buf[6], x2);
        this->addIntToBuf(&buf[8], y2);
        buf[10] = color;
        buf[11] = color >> 8;
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 12);
    }

    /**
     * Draws a rectangle with no fill between two vertices
     * @param x1 The x coordinate of the first vertex
     * @param y2 The y coordinate of the first vertex
     * @param x2 The x coordinate of the second vertex
     * @param y2 The y coordinate of the second vertex
     * @param color The 4DGL color for the rectangle's outline
     */
    void uLCD::drawRectangle(int x1, int y1, int x2, int y2, uint16_t color) {
        this->awaitResponse();

        char* buf = (char*) malloc_safe(12);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xCF;
        this->addIntToBuf(&buf[2], x1);
        this->addIntToBuf(&buf[4], y1);
        this->addIntToBuf(&buf[6], x2);
        this->addIntToBuf(&buf[8], y2);
        buf[10] = color;
        buf[11] = color >> 8;
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 12);
    }

    /**
     * Draws a filled rectangle between two vertices
     * @param x1 The x coordinate of the first vertex
     * @param y2 The y coordinate of the first vertex
     * @param x2 The x coordinate of the second vertex
     * @param y2 The y coordinate of the second vertex
     * @param color The 4DGL color for the rectangle's fill
     */
    void uLCD::drawRectangleFilled(int x1, int y1, int x2, int y2, uint16_t color) {
        this->awaitResponse();

        char* buf = (char*) malloc_safe(12);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xCE;
        this->addIntToBuf(&buf[2], x1);
        this->addIntToBuf(&buf[4], y1);
        this->addIntToBuf(&buf[6], x2);
        this->addIntToBuf(&buf[8], y2);
        buf[10] = color;
        buf[11] = color >> 8;
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 12);
    }

    /**
     * Sets an individual pixel's color
     * @param x The x coordinate
     * @param y The y coordinate
     * @param color The 4DGL color for the pixel
     */
    void uLCD::setPixel(int x, int y, uint16_t color) {
        this->awaitResponse();

        char* buf = (char*) malloc_safe(8);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0xCB;
        this->addIntToBuf(&buf[2], x);
        this->addIntToBuf(&buf[4], y);
        buf[6] = color;
        buf[7] = color >> 8;
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 8);
    }

    /**
     * Draws a bitmap at the given coordinate pair from a buffer.
     * @param x The x coordinate of the top left corner
     * @param y The y coordinate of the top left corner
     * @param width The width of the bitmap
     * @param height The height of the bitmap
     * @param image The buffer containing the bitmap. Must be at least of size width * height
     */
    void uLCD::BLIT(int x, int y, int width, int height, uint16_t* image, bool freeable) {
        //ensuring valid image first
        if (width <= 0 || height <= 0) {
            if (freeable) {
                free_safe(image);
                printFree(image);
            }
            return;
        }

        this->awaitResponse();

        char buf[10];
        printMalloc(buf);
        buf[0] = 0x0;
        buf[1] = 0xA;
        this->addIntToBuf(&buf[2], x);
        this->addIntToBuf(&buf[4], y);
        this->addIntToBuf(&buf[6], width);
        this->addIntToBuf(&buf[8], height);
        this->serial.checkBufferFree();
        this->serial.write(buf, 10);
        this->serial.sync();

        this->waitToWrite(image, width * height * 2, 2500, freeable);
    }

    /**
     * Sets the outline color for applicable shapes.
     * @param color The color for the outline, zero disables outlines
     */
    void uLCD::setOutlineColor(uint16_t color) {
        this->awaitResponse();

        char* buf = (char*) malloc_safe(4);
        printMalloc(buf);
        buf[0] = 0xFF;
        buf[1] = 0x67;
        buf[2] = color;
        buf[3] = color >> 8;
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 4);
    }

    /**
     * Sets the clipping window. If all parameters are zero, then clipping is disabled.
     * @param x The x coordinate of the top left corner
     * @param y The y coordinate of the top left corner
     * @param width The width of the clipping window
     * @param height The height of the clipping window
     */
    void uLCD::setClippingWindow(int x, int y, int width, int height) {
        this->awaitResponse();    
        if (!x && !y && !width && !height) {
            //disable clipping
            char buf[4];
            buf[0] = 0xFF;
            buf[1] = 0x6C;
            buf[2] = 0x0;
            buf[3] = 0x0;

            this->serial.checkBufferFree();
            this->serial.write(buf, 4);

            this->awaitResponse();

            return;
        }

        char* buf = (char*) malloc_safe(10);
        printMalloc(buf);

        //enable clipping
        buf[0] = 0xFF;
        buf[1] = 0x6C;
        buf[2] = 0x0;
        buf[3] = 0x1;

        this->serial.checkBufferFree();
        this->serial.write(buf, 4);

        this->awaitResponse();

        //making sure clipping coordinates are contained to the screen area
        int x1 = x + width - 1;
        int y1 = y + height - 1;
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        if (x1 < 0) {
            x1 = 0;
        }
        if (y1 < 0) {
            y1 = 0;
        }
        if (x > 127) {
            x = 127;
        }
        if (x1 > 127) {
            x1 = 127;
        }
        if (y > 127) {
            y = 127;
        }
        if (y1 > 127) {
            y1 = 127;
        }

        //set clipping region

        buf[0] = 0xFF;
        buf[1] = 0xBF;
        this->addIntToBuf(&buf[2], x);
        this->addIntToBuf(&buf[4], y);
        this->addIntToBuf(&buf[6], x1);
        this->addIntToBuf(&buf[8], y1);
        this->serial.checkBufferFree();
        this->serial.writeAndFree(buf, 10);
     }

uint16_t uLCD::get4DGLColor(const char* color) {
    if (color[0] == '#') {
        color++; // just ignore the character
    }

    uint32_t normalColor = 0;
    std::stringstream ss;
    ss << std::hex << color;
    ss >> normalColor;

    uint16_t red = (normalColor >> 19) & 0x1F;
    uint16_t green = (normalColor >> 10) & 0x3F;
    uint16_t blue = (normalColor >> 3) & 0x1F;

    return (red << 3) + (green >> 3) + (green << 13) + (blue << 8);
}

uint16_t uLCD::get4DGLColor(uint32_t normalColor) {
    uint16_t red = (normalColor >> 19) & 0x1F;
    uint16_t green = (normalColor >> 10) & 0x3F;
    uint16_t blue = (normalColor >> 3) & 0x1F;

    return (red << 3) + (green >> 3) + (green << 13) + (blue << 8);
}