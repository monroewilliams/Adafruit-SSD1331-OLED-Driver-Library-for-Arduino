/*!
 * @file Adafruit_SSD1331.cpp
 *
 * @mainpage Adafruit SSD1331 Arduino Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for the 0.96" 16-bit Color OLED with SSD1331 driver chip
 *
 * Pick one up today in the adafruit shop!
 * ------> http://www.adafruit.com/products/684
 *
 * These displays use SPI to communicate, 4 or 5 pins are required to
 * interface
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text above must be included in any redistribution
 */

#include "Adafruit_SSD1331.h"
#include "pins_arduino.h"
#include "wiring_private.h"

/***********************************/

/*!
  @brief   SPI displays set an address window rectangle for blitting pixels
  @param  x  Top left corner x coordinate
  @param  y  Top left corner x coordinate
  @param  w  Width of window
  @param  h  Height of window
*/
void Adafruit_SSD1331::setAddrWindow(uint16_t x, uint16_t y, uint16_t w,
                                     uint16_t h) {
  // The inputs to this will _always_ be pre-clipped

  // True if we're swapping row and column due to rotation
  bool swap = rotation & 0x01;

  SPI_DC_LOW();  // enter command mode

  spiWrite(swap?SSD1331_CMD_SETROW:SSD1331_CMD_SETCOLUMN);
  spiWrite(x);
  spiWrite(x + w - 1);

  spiWrite(swap?SSD1331_CMD_SETCOLUMN:SSD1331_CMD_SETROW); 
  spiWrite(y);
  spiWrite(y + h - 1);

  SPI_DC_HIGH(); // exit command mode
}

/**************************************************************************/
/*!
    @brief   Initialize SSD1331 chip
    Connects to the SSD1331 over SPI and sends initialization procedure commands
    @param    freq  Desired SPI clock frequency
*/
/**************************************************************************/ 
#if defined SSD1331_COLORORDER_RGB
static const uint8_t SETREMAP_COLOR_BITS = 0b01100000; // 0x72 - RGB Color
#else
static const uint8_t SETREMAP_COLOR_BITS = 0b01100100; // 0x76 - BGR Color
#endif

// These are the bits to OR into the setremap command for different rotations
static const uint8_t SETREMAP_ROTATION_0_BITS = 0b00010010;
static const uint8_t SETREMAP_ROTATION_1_BITS = 0b00000011;
static const uint8_t SETREMAP_ROTATION_2_BITS = 0b00000000;
static const uint8_t SETREMAP_ROTATION_3_BITS = 0b00010001;

void Adafruit_SSD1331::begin(uint32_t freq) {
  initSPI(freq);

  // Initialization Sequence
  sendCommand(SSD1331_CMD_DISPLAYOFF); // 0xAE
  sendCommand(SSD1331_CMD_SETREMAP);   // 0xA0
  sendCommand(SETREMAP_COLOR_BITS|SETREMAP_ROTATION_0_BITS);
  sendCommand(SSD1331_CMD_STARTLINE); // 0xA1
  sendCommand(0x0);
  sendCommand(SSD1331_CMD_DISPLAYOFFSET); // 0xA2
  sendCommand(0x0);
  sendCommand(SSD1331_CMD_NORMALDISPLAY); // 0xA4
  sendCommand(SSD1331_CMD_SETMULTIPLEX);  // 0xA8
  sendCommand(0x3F);                      // 0x3F 1/64 duty
  sendCommand(SSD1331_CMD_SETMASTER);     // 0xAD
  sendCommand(0x8E);
  sendCommand(SSD1331_CMD_POWERMODE); // 0xB0
  sendCommand(0x0B);
  sendCommand(SSD1331_CMD_PRECHARGE); // 0xB1
  sendCommand(0x31);
  sendCommand(SSD1331_CMD_CLOCKDIV); // 0xB3
  sendCommand(0xF0); // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio
                     // (A[3:0]+1 = 1..16)
  sendCommand(SSD1331_CMD_PRECHARGEA); // 0x8A
  sendCommand(0x64);
  sendCommand(SSD1331_CMD_PRECHARGEB); // 0x8B
  sendCommand(0x78);
  sendCommand(SSD1331_CMD_PRECHARGEC); // 0x8C
  sendCommand(0x64);
  sendCommand(SSD1331_CMD_PRECHARGELEVEL); // 0xBB
  sendCommand(0x3A);
  sendCommand(SSD1331_CMD_VCOMH); // 0xBE
  sendCommand(0x3E);
  sendCommand(SSD1331_CMD_MASTERCURRENT); // 0x87
  sendCommand(0x06);
  sendCommand(SSD1331_CMD_CONTRASTA); // 0x81
  sendCommand(0x91);
  sendCommand(SSD1331_CMD_CONTRASTB); // 0x82
  sendCommand(0x50);
  sendCommand(SSD1331_CMD_CONTRASTC); // 0x83
  sendCommand(0x7D);
  sendCommand(SSD1331_CMD_DISPLAYON); //--turn on oled panel
  _width = TFTWIDTH;
  _height = TFTHEIGHT;
}

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit SSD1331 driver with software SPI
    @param    cs    Chip select pin #
    @param    dc    Data/Command pin #
    @param    mosi  SPI MOSI pin #
    @param    sclk  SPI Clock pin #
    @param    rst   Reset pin # (optional, pass -1 if unused)
*/
/**************************************************************************/
Adafruit_SSD1331::Adafruit_SSD1331(int8_t cs, int8_t dc, int8_t mosi,
                                   int8_t sclk, int8_t rst)
    : Adafruit_SPITFT(TFTWIDTH, TFTHEIGHT, cs, dc, mosi, sclk, rst, -1) , scroll(false) {}

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit SSD1331 driver with hardware SPI
    @param    cs    Chip select pin #
    @param    dc    Data/Command pin #
    @param    rst   Reset pin # (optional, pass -1 if unused)
*/
/**************************************************************************/
Adafruit_SSD1331::Adafruit_SSD1331(int8_t cs, int8_t dc, int8_t rst)
    : Adafruit_SPITFT(TFTWIDTH, TFTHEIGHT, cs, dc, rst) , scroll(false) {}

/**************************************************************************/
/*!
    @brief  Instantiate Adafruit SSD1331 driver with hardware SPI
    @param    spi   Pointer to an existing SPIClass instance (e.g. &SPI, the
                    microcontroller's primary SPI bus).
    @param    cs    Chip select pin #
    @param    dc    Data/Command pin #
    @param    rst   Reset pin # (optional, pass -1 if unused)
*/
/**************************************************************************/
Adafruit_SSD1331::Adafruit_SSD1331(SPIClass *spi, int8_t cs, int8_t dc,
                                   int8_t rst)
    :
#if defined(ESP8266)
      Adafruit_SPITFT(TFTWIDTH, TFTHEIGHT, cs, dc, rst)
#else
      Adafruit_SPITFT(TFTWIDTH, TFTHEIGHT, spi, cs, dc, rst)
#endif
, scroll(false){}

/**************************************************************************/
/*!
    @brief  Change whether display is on or off
    @param	 enable True if you want the display ON, false OFF
*/
/**************************************************************************/
void Adafruit_SSD1331::enableDisplay(boolean enable) {
  sendCommand(enable ? SSD1331_CMD_DISPLAYON : SSD1331_CMD_DISPLAYOFF);
}

inline void Adafruit_SSD1331::spiWriteXY(int16_t x, int16_t y)
{
  bool swap = rotation & 0x01;
  spiWrite(swap?y:x);
  spiWrite(swap?x:y);
}

inline void Adafruit_SSD1331::spiWriteColor(int16_t color)
{
  spiWrite((color >> 10) & 0x3E); // red
  spiWrite((color >> 5) & 0x3F);  // green
  spiWrite((color << 1) & 0x3E);  // blue
}

void Adafruit_SSD1331::setRotation(uint8_t r)
{
  rotation = (r & 0x03);
  if (rotation & 1) {
    // rotated 90 or 270 degrees -- swap width and height
    _width = HEIGHT;
    _height = WIDTH;
  } else {
    // rotated 0 or 180 -- width and height are as set
    _width = WIDTH;
    _height = HEIGHT;
  }
  
  uint8_t remap_bits = SETREMAP_COLOR_BITS;
  switch (rotation) {
  case 0:
    // normal
    remap_bits |= SETREMAP_ROTATION_0_BITS;
  break;
  case 1:
    // rotated right 90 degrees
    remap_bits |= SETREMAP_ROTATION_1_BITS;
  break;
  case 2:
    // rotated right 180 degrees
    remap_bits |= SETREMAP_ROTATION_2_BITS;
  break;
  case 3:
    // rotated right 270 degrees
    remap_bits |= SETREMAP_ROTATION_3_BITS;
  break;
  }

  sendCommand(SSD1331_CMD_SETREMAP);   // 0xA0
  sendCommand(remap_bits);
}

/**************************************************************************/
/*!
    @brief      Invert the display (ideally using built-in hardware command)
    @param   i  True if you want to invert, false to make 'normal'
*/
/**************************************************************************/
void Adafruit_SSD1331::invertDisplay(bool i) {
  sendCommand(i?SSD1331_CMD_INVERTDISPLAY:SSD1331_CMD_NORMALDISPLAY);
}

/**************************************************************************/
/*!
   @brief    Write a rectangle completely with one color, overwrite in
   subclasses if startWrite is defined!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Adafruit_SSD1331::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                 uint16_t color) {
  int16_t x1 = x + w;
  int16_t y1 = y + h;

  // Bail out if completely off-screen
  if (x1 < 0 || x >= _width || y1 < 0 || y >= _height) {
    return;
  }

  if (x < 0)
    x = 0;
  if (x1 > _width)
    x1 = _width;
  if (y < 0)
    y = 0;
  if (y1 > _height)
    y1 = _height;
  
  // Bail out if clipped rect is empty
  if (x1 - 1 < x || y1 - 1 < y) {
    return;
  }

  SPI_DC_LOW();  // enter command mode

  if (color == 0)
  {
    // if filling zero, we can use the less expensive clear command (writes 5 bytes over SPI).
    spiWrite(SSD1331_CMD_CLEAR);
    spiWriteXY(x, y); // starting column/row
    spiWriteXY(x1 - 1, y1 - 1); // finishing column/row
  }
  else if (x == x1 || y == y1)
  {
    // If the rect is 1 pixel wide or high, we can use the less expensive line-drawing command (writes 8 bytes over SPI)
    writeLine(x, y, x1, y1, color);
  }
  else
  {
    // Use the actual fill-rect command (13 bytes over SPI)
    spiWrite(SSD1331_CMD_FILL); // enable fill
    spiWrite(0x01);
    spiWrite(SSD1331_CMD_DRAWRECT); // enter "draw rectangle" mode
    spiWriteXY(x, y); // starting column/row
    spiWriteXY(x1 - 1, y1 - 1); // finishing column/row
    spiWriteColor(color);
    spiWriteColor(color);
  }

  SPI_DC_HIGH(); // exit command mode

  // Filling the pixels sometimes takes long enough that it may be interrupted by subsequent commands, 
  // and corrupt the data as a result.
  // Calculate a delay based on the number of pixels written.
  // For a full-screen fill, we want to delay somewhere above 1000us. 
  // A full-screen fill is 96 * 64 = 6144 pixels.
  // Dividing this by 4 gives us 1536us, which is close enough.
  int delay = (w * h) >> 2;
  delayMicroseconds(delay);
}

void Adafruit_SSD1331::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  // writeLine() is the fastest way to do this.
  writeLine(x, y, x, y + h, color);
}
void Adafruit_SSD1331::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
  // writeLine() is the fastest way to do this.
  writeLine(x, y, x + w, y, color);
}

/**************************************************************************/
/*!
   @brief    Draw a line
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Adafruit_SSD1331::writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            uint16_t color) {

  // Simple, dumb clip. If either point is outside the screen, don't draw.
  if (x0 < 0 || x0 >= _width || 
      x1 < 0 || x1 >= _width || 
      y0 < 0 || y0 >= _height || 
      y1 < 0 || y1 >= _height) {
    return;
  }

  SPI_DC_LOW();  // enter command mode

  spiWrite(SSD1331_CMD_DRAWLINE); // enter "draw rectangle" mode
  spiWriteXY(x0, y0); // starting column/row
  spiWriteXY(x1, y1); // finishing column/row
  spiWriteColor(color);

  SPI_DC_HIGH(); // exit command mode
}

void Adafruit_SSD1331::drawPixel(int16_t x, int16_t y, uint16_t color) {
  startWrite();
  writePixel(x, y, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly vertical line (this is often optimized in a
   subclass!)
    @param    x   Top-most x coordinate
    @param    y   Top-most y coordinate
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Adafruit_SSD1331::drawFastVLine(int16_t x, int16_t y, int16_t h,
                                 uint16_t color) {
  startWrite();
  writeFastVLine(x, y, h, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief    Draw a perfectly horizontal line (this is often optimized in a
   subclass!)
    @param    x   Left-most x coordinate
    @param    y   Left-most y coordinate
    @param    w   Width in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Adafruit_SSD1331::drawFastHLine(int16_t x, int16_t y, int16_t w,
                                 uint16_t color) {
  startWrite();
  writeFastHLine(x, y, w, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief    Fill a rectangle completely with one color. Update in subclasses if
   desired!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Adafruit_SSD1331::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color) {
  startWrite();
  writeFillRect(x, y, w, h, color);
  endWrite();
}

/**************************************************************************/
/*!
   @brief    Fill the screen completely with one color. Update in subclasses if
   desired!
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void Adafruit_SSD1331::fillScreen(uint16_t color) {
  fillRect(0, 0, _width, _height, color);
}

/**************************************************************************/
/*!
   @brief    Draw a line
    @param    x0  Start point x coordinate
    @param    y0  Start point y coordinate
    @param    x1  End point x coordinate
    @param    y1  End point y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Adafruit_SSD1331::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                            uint16_t color) {
  startWrite();
  writeLine(x0, y0, x1, y1, color);
  endWrite();
}
/**************************************************************************/
/*!
   @brief   Draw a rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void Adafruit_SSD1331::drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color) {
  if (x < 0 || x >= _width || 
      y < 0 || y >= _height ||
      w <= 0 || h <= 0)
    return;

  int16_t x1 = x + w;
  int16_t y1 = y + h;

  if (x1 > _width)
    x1 = _width;
  if (y1 >= _height)
    y1 = _height;

  startWrite();

  SPI_DC_LOW();  // enter command mode
  
  spiWrite(SSD1331_CMD_FILL); // disble fill
  spiWrite(0x00);
  spiWrite(SSD1331_CMD_DRAWRECT); // enter "draw rectangle" mode
  spiWriteXY(x, y); // starting column/row
  spiWriteXY(x1 - 1, y1 - 1); // finishing column/row
  spiWriteColor(color);
  spiWriteColor(color);

  SPI_DC_HIGH(); // exit command mode

  endWrite();
}

#ifdef SSD1331_EXTRAS

void Adafruit_SSD1331::copyBits(int16_t x, int16_t y, int16_t w, int16_t h,
              int16_t dx, int16_t dy, bool invert)
{
  // Clip such that both source and destination are completely contained within the screen bounds.
  int min_x = min(x, dx);
  int max_x = max(x, dx) + w;
  int clip = - min_x;
  if (clip > 0) {
    x += clip;
    dx += clip;
    w -= clip;
  }
  clip = max_x - _width;
  if (clip > 0) {
    w -= clip;
  }
  if (w <= 0){
    // Empty rect
    return;
  }

  int min_y = min(y, dy);
  int max_y = max(y, dy) + h;
  clip = -min_y;
  if (clip > 0) {
    y += clip;
    dy += clip;
    h -= clip;
  }
  clip = max_y - _height;
  if (clip > 0) {
    h -= clip;
  }
  if (h <= 0){
    // Empty rect
    return;
  }

  startWrite();

  SPI_DC_LOW();  // enter command mode
  
  spiWrite(SSD1331_CMD_FILL); // configure invert
  spiWrite(invert?0x10:0x00);
  spiWrite(SSD1331_CMD_COPY); // enter "draw rectangle" mode
  spiWriteXY(x, y); // starting column/row
  spiWriteXY(x + w - 1, y + h - 1); // finishing column/row
  spiWriteXY(dx, dy); // destination column/row
  
  SPI_DC_HIGH(); // exit command mode

  endWrite();

  // Copying the bits sometimes takes long enough that it may be interrupted by subsequent commands, 
  // and corrupt the data as a result.
  // Calculate a delay based on the number of pixels to be blitted.
  // For a full-screen blit, we want to delay somewhere above 1000us. 
  // A full-screen blit is 96 * 64 = 6144 pixels.
  // Dividing this by 4 gives us 1536us, which is close enough.
  int delay = (w * h) >> 2;
  delayMicroseconds(delay);
}

size_t Adafruit_SSD1331::write(uint8_t c) {
  // If scrolling is enabled and the character about to be drawn would be partly below the bottom of the screen, scroll up.
  if (scroll) {
    int line_height = textsize_y * (gfxFont?(uint8_t)pgm_read_byte(&gfxFont->yAdvance):8);
    if (cursor_y + line_height >= _height) {
      copyBits(0, 0, _width, _height, 0, -line_height);
      fillRect(0, _height - line_height, _width, line_height, 0);
      cursor_y -= line_height;
    }
  }

  // call through to superclass for the actual drawing.
  return Adafruit_GFX::write(c);
}

#endif
