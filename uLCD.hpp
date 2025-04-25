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

#ifndef ULCD_INCLUDED
#define ULCD_INCLUDED

#include "mbed.h"
#include <cstdint>
#include "serialAsync.hpp"

//us is the minimum length of the delay
typedef void (*WaitFunction)(int us);

class uLCD {
    private:

    SerialAsync serial;
    DigitalOut resetSignal;
    WaitFunction waitFunction;
    bool bypassAwait;
    volatile bool delayedWritePending;
    Timeout delay;
    void* delayBuffer;
    int delaySize;
    int delayFreeable;

    void addIntToBuf(char* buf, int v);

    void awaitResponse();

    void writeBack();

    void waitToWrite(void* buffer, int size, int delay_us, bool freeable);

    public:

    enum uLCDBaud {
        BAUD_9600 = 312,
        BAUD_56000 = 53,
        BAUD_115200 = 25,
        BAUD_128000 = 22,
        BAUD_300000 = 10,
        BAUD_600000 = 4,
        BAUD_1000000 = 2,
        BAUD_1500000 = 1
    };

    /*
     * With the baud rate, the library will always use 9600 for initial communication with the uLCD
     * It sets the new baud rate after starting initial communication.
     */
    uLCD(PinName tx, PinName rx, PinName reset, uLCDBaud baud);

    //General functions
    
    /** Clear the screen and fills the screen with the set background color. Defaults to black (0x0)*/
    void cls();

    /** Resets the screen, blocks for 3 seconds */
    void reset();

    //Text Functions

    /**
     * Sets text foreground color
     * @param color The 4DGL color to set the text foreground color
     */
    void setTextColor(uint16_t color);

    /**
     * Sets text background color
     * @param color The 4DGL color to set the text background color
     */
    void setTextBackground(uint16_t color);

    /**
     * Sets the font scalar values, scales by integral amounts only.
     * @param width The width scalar for the font
     * @param height The height scalar for the font
     */
    void setFontSize(int width, int height);

    /**
     * Sets whether the next text sent to the display gets bolded.
     * IMPORTANT: Text format commands are reset after the next text sent.
     * @param bold True to bold next text, false to reset manually
     */
    void setTextBold(bool bold);

    /**
     * Sets whether the next text sent to the display gets italicized
     * IMPORTANT: Text format commands are reset after the next text sent.
     * @param italic True to italicize next text, false to reset manually
     */
    void setTextItalic(bool italic);

    /**
     * Sets whether the next text sent to the display gets inverted colors
     * IMPORTANT: Text format commands are reset after the next text sent.
     * @param invert True to invert colors for next text, false to reset manually
     */
    void setTextInverted(bool invert);

    /**
     * Sets whether the next text sent to the display gets underlined
     * IMPORTANT: Text format commands are reset after the next text sent.
     * @param underline True to underline next text, false to reset manually
     */
    void setTextUnderline(bool underline);

    /**
     * Prints one char to the display at the location set with locate()
     * @param c The character to print
     */
    void print(char c);

    /**
     * Prints a string to the lcd at the location set with locate()
     * IMPORTANT: Will not automatically line wrap, this must be done manually
     * @param str The null-terminated string to print
     */
    void print(char* str);

    /**
     * Prints a formatted string to the lcd at the location set with locate()
     * IMPORTANT: Will not automatically line wrap, this must be done manually
     * @param format A string with the appropriate formatting codes
     */
    void printf(const char* str, ...);

    /**
     * Locates the text cursor to the given text coordinates.
     * IMPORTANT: Text coordinates are not the same as screen coordinates
     * @param x The x text coordinate
     * @param y The y text coordinate
     */
    void locate(int x, int y);

    //Graphics Functions

    /**
     *  Draws a circle with no fill centered at (x, y)
     * @param x The x pixel coordinate
     * @param y The y pixel coordinate
     * @param r The radius measured in pixels
     * @param color The 4DGL color for the circle's outline
     */
    void drawCircle(int x, int y, int radius, uint16_t color);

    /**
     *  Draws a filled circle centered at (x, y)
     * @param x The x pixel coordinate
     * @param y The y pixel coordinate
     * @param r The radius measured in pixels
     * @param color The 4DGL color for the circle's fill
     */
    void drawCircleFilled(int x, int y, int radius, uint16_t color);

    /**
     * Draws a triangle with no fill
     * @param x1 The x coordinate of the first vertex
     * @param y2 The y coordinate of the first vertex
     * @param x2 The x coordinate of the second vertex
     * @param y2 The y coordinate of the second vertex
     * @param x3 The x coordinate of the third vertex
     * @param y3 The y coordinate of the third vertex
     * @param color The 4DGL color for the triangle's outline
     */
    void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color);

    /**
     * Draws a line between the two vertices
     * @param x1 The x coordinate of the first vertex
     * @param y2 The y coordinate of the first vertex
     * @param x2 The x coordinate of the second vertex
     * @param y2 The y coordinate of the second vertex
     * @param color The 4DGL color for the line
     */
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color);

    /**
     * Draws a rectangle with no fill between two vertices
     * @param x1 The x coordinate of the first vertex
     * @param y2 The y coordinate of the first vertex
     * @param x2 The x coordinate of the second vertex
     * @param y2 The y coordinate of the second vertex
     * @param color The 4DGL color for the rectangle's outline
     */
    void drawRectangle(int x1, int y1, int x2, int y2, uint16_t color);

    /**
     * Draws a filled rectangle between two vertices
     * @param x1 The x coordinate of the first vertex
     * @param y2 The y coordinate of the first vertex
     * @param x2 The x coordinate of the second vertex
     * @param y2 The y coordinate of the second vertex
     * @param color The 4DGL color for the rectangle's fill
     */
    void drawRectangleFilled(int x1, int y1, int x2, int y2, uint16_t color);

    /**
     * Sets an individual pixel's color
     * @param x The x coordinate
     * @param y The y coordinate
     * @param color The 4DGL color for the pixel
     */
    void setPixel(int x, int y, uint16_t color);

    /**
     * Draws a bitmap at the given coordinate pair from a buffer.
     * @param x The x coordinate of the top left corner
     * @param y The y coordinate of the top left corner
     * @param width The width of the bitmap
     * @param height The height of the bitmap
     * @param image The buffer containing the bitmap. Must be at least of size width * height
     */
    void BLIT(int x, int y, int width, int height, uint16_t* image, bool freeable);

    /**
     * Sets the outline color for applicable shapes.
     * @param color The color for the outline, zero disables outlines
     */
    void setOutlineColor(uint16_t color);

    /**
     * Sets the clipping window. If all parameters are zero, then clipping is disabled.
     * @param x The x coordinate of the top left corner
     * @param y The y coordinate of the top left corner
     * @param width The width of the clipping window
     * @param height The height of the clipping window
     */
     void setClippingWindow(int x, int y, int width, int height);

    /**
     * Static function for converting hex codes to 4DGL colors
     * @param color The hex code string to convert
     * @returns The 4DGL color
     */
    static uint16_t get4DGLColor(const char* color);

    /**
     * Static function for converting 32 bit integer colors to 4DGL colors
     * @param color The ARGB packed color as a 32-bit integer
     * @returns The 4DGL color
     */
    static uint16_t get4DGLColor(uint32_t color);
};

#endif //ULCD_INCLUDED