/*----------------------------------------------------------------------------/
  Lovyan GFX library - LCD graphics library .
  
  support platform:
    ESP32 (SPI/I2S) with Arduino/ESP-IDF
    ATSAMD51 (SPI) with Arduino
  
Original Source:  
 https://github.com/lovyan03/LovyanGFX/  

Licence:  
 [BSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)  

Author:  
 [lovyan03](https://twitter.com/lovyan03)  

Contributors:  
 [ciniml](https://github.com/ciniml)  
 [mongonta0716](https://github.com/mongonta0716)  
 [tobozo](https://github.com/tobozo)  
/----------------------------------------------------------------------------*/
#ifndef LGFX_DEVICE_HPP_
#define LGFX_DEVICE_HPP_

#include "LGFXBase.hpp"
#include "LGFX_Sprite.hpp"
#include "touch/TouchCommon.hpp"
#include "panel/PanelCommon.hpp"

namespace lgfx
{

//----------------------------------------------------------------------------

  class LGFX_Device : public LovyanGFX
  {
  public:
    // Write single byte as COMMAND
    virtual void writeCommand(std::uint_fast8_t cmd) = 0; // AdafruitGFX compatible
    __attribute__ ((always_inline))
    inline  void writecommand(std::uint_fast8_t cmd) { writeCommand(cmd); } // TFT_eSPI compatible

    // Write single bytes as DATA
    virtual void writeData(std::uint_fast8_t data) = 0; // TFT_eSPI compatible
    __attribute__ ((always_inline))
    inline  void spiWrite( std::uint_fast8_t data) { writeData(data); } // AdafruitGFX compatible
    __attribute__ ((always_inline))
    inline  void writedata(std::uint_fast8_t data) { writeData(data); } // TFT_eSPI compatible

    void writeBytes(const std::uint8_t* data, std::int32_t length, bool use_dma = true) { writeBytes_impl(data, length, use_dma); }

    virtual std::uint32_t readCommand(std::uint_fast8_t cmd, std::uint_fast8_t index=0, std::uint_fast8_t len=4) = 0;

    std::uint8_t  readCommand8( std::uint_fast8_t cmd, std::uint_fast8_t index=0) { return readCommand(cmd, index, 1); }
    std::uint8_t  readcommand8( std::uint_fast8_t cmd, std::uint_fast8_t index=0) { return readCommand(cmd, index, 1); }
    std::uint16_t readCommand16(std::uint_fast8_t cmd, std::uint_fast8_t index=0) { return __builtin_bswap16(readCommand(cmd, index, 2)); }
    std::uint16_t readcommand16(std::uint_fast8_t cmd, std::uint_fast8_t index=0) { return __builtin_bswap16(readCommand(cmd, index, 2)); }
    std::uint32_t readCommand32(std::uint_fast8_t cmd, std::uint_fast8_t index=0) { return __builtin_bswap32(readCommand(cmd, index, 4)); }
    std::uint32_t readcommand32(std::uint_fast8_t cmd, std::uint_fast8_t index=0) { return __builtin_bswap32(readCommand(cmd, index, 4)); }
    std::uint32_t readPanelID(void) { return readCommand(_panel->getCmdRddid()); }

    __attribute__ ((always_inline)) inline bool getInvert(void) const { return _panel->invert; }

    __attribute__ ((always_inline)) inline PanelCommon* getPanel(void) const { return _panel; }
    __attribute__ ((always_inline)) inline PanelCommon* panel(void) const { return _panel; }
    __attribute__ ((always_inline)) inline void setPanel(PanelCommon* panel) { _panel = panel; postSetPanel(); }
    __attribute__ ((always_inline)) inline void panel(PanelCommon* panel) { _panel = panel; postSetPanel(); }

    __attribute__ ((always_inline)) inline TouchCommon* touch(void) const { return _touch; }
    __attribute__ ((always_inline)) inline void setTouch(TouchCommon* touch_) { _touch = touch_; postSetTouch(); }
    __attribute__ ((always_inline)) inline void touch(TouchCommon* touch_) { _touch = touch_; postSetTouch(); }

    void sleep(void)
    {
      std::uint8_t buf[32];
      if (auto b = _panel->getSleepInCommands(buf)) commandList(b);
      _panel->sleep();
    }

    void wakeup(void)
    {
      std::uint8_t buf[32];
      if (auto b = _panel->getSleepOutCommands(buf)) commandList(b);
      _panel->wakeup();
    }

    void partialOn(void)
    {
      std::uint8_t buf[32];
      if (auto b = _panel->getPartialOnCommands(buf)) commandList(b);
    }

    void partialOff(void)
    {
      std::uint8_t buf[32];
      if (auto b = _panel->getPartialOffCommands(buf)) commandList(b);
    }

    void flush(void) 
    {
      if (nullptr == _panel->fp_flush) return;
      startWrite();
      _panel->fp_flush(_panel, this);
      endWrite();
    }

    void setColorDepth(std::uint8_t bpp) { setColorDepth((color_depth_t)bpp); }
    void setColorDepth(color_depth_t depth)
    {
      std::uint8_t buf[32];
      commandList(_panel->getColorDepthCommands(buf, depth));
      postSetColorDepth();
    }

    void setRotation(std::int_fast8_t r)
    {
      std::uint8_t buf[32];
      commandList(_panel->getRotationCommands(buf, r));
      postSetRotation();
    }

    void invertDisplay(bool i)
    {
      std::uint8_t buf[32];
      commandList(_panel->getInvertDisplayCommands(buf, i));
    }

    std::uint8_t getBrightness(void) const { return _panel->brightness; }

    void setBrightness(std::uint8_t brightness) {
      _panel->setBrightness(brightness);
    }

    virtual void initBus(void) = 0;

    virtual void writeBytes_impl(const std::uint8_t* data, std::int32_t length, bool use_dma) = 0;
    
    void initPanel(bool use_reset = true)
    {
      preInit();
      if (!_panel) return;

      _panel->init(use_reset);

      _sx = _sy = 0;
      _sw = _width;
      _sh = _height;

      startWrite();

      const std::uint8_t *cmds;
      for (std::uint8_t i = 0; (cmds = _panel->getInitCommands(i)); i++) {
        delay(120);
        cs_l();
        commandList(cmds);
        cs_h();
      }
      cs_l();

      invertDisplay(getInvert());
      setColorDepth(getColorDepth());
      setRotation(getRotation());
      setBrightness(getBrightness());

      endWrite();

      _panel->post_init(this);
    }

    void initTouch(void)
    {
      if (!_touch) return;

      _touch->init();
      _touch->wakeup();
    }

    std::uint_fast8_t getTouchRaw(std::int32_t *x = nullptr, std::int32_t *y = nullptr, std::uint_fast8_t number = 0)
    {
      if (!_touch) return 0;

      bool need_transaction = (_touch->bus_shared && _in_transaction);
      if (need_transaction) { endTransaction(); }
      auto res = _touch->getTouch(x, y, number);
      if (need_transaction) { beginTransaction(); }
      return res;
    }

    template <typename T>
    std::uint_fast8_t getTouch(T *x, T *y, std::uint_fast8_t number = 0)
    {
      std::int32_t tx = -1, ty = -1;
      auto res = getTouch(&tx, &ty, number);
      if (x) *x = tx;
      if (y) *y = ty;
      return res;
    }

    std::uint_fast8_t getTouch(std::int32_t *x, std::int32_t *y, std::uint_fast8_t number = 0)
    {
      std::int32_t tx = -1, ty = -1;
      auto res = getTouchRaw(&tx, &ty, number);
      if (0 == res) return 0;
      convertRawXY(&tx, &ty);
      if (x) *x = tx;
      if (y) *y = ty;
      return res;
    }

    void convertRawXY(std::uint16_t *x, std::uint16_t *y)
    {
      std::int32_t tx = *x, ty = *y;
      convertRawXY(&tx, &ty);
      *x = tx;
      *y = ty;
    }

    void convertRawXY(std::int32_t *x, std::int32_t *y)
    {
      std::int32_t tx = (_touch_affine[0] * (float)*x + _touch_affine[1] * (float)*y) + _touch_affine[2];
      std::int32_t ty = (_touch_affine[3] * (float)*x + _touch_affine[4] * (float)*y) + _touch_affine[5];

      std::uint_fast8_t r = _panel->rotation & 7;
      if (r & 1) {
        std::swap(tx, ty);
      }
      if (x) {
        if (r & 2) tx = (_width-1) - tx;
        *x = tx;
      }
      if (y) {
        if ((0 == ((r + 1) & 2)) != (0 == (r & 4))) ty = (_height-1) - ty;
        *y = ty;
      }
    }

    // This requires a uint16_t array with 8 elements. ( or nullptr )
    template <typename T>
    void calibrateTouch(uint16_t *parameters, const T& color_fg, const T& color_bg, uint8_t size = 10)
    {
      calibrate_touch(parameters, _write_conv.convert(color_fg), _write_conv.convert(color_bg), size);
    }

    // This requires a uint16_t array with 8 elements.
    void setTouchCalibrate(std::uint16_t *parameters) { set_touch_calibrate(parameters); }

    bool commandList(const std::uint8_t *addr)
    {
      if (addr == nullptr) return false;
      if (*reinterpret_cast<const std::uint16_t*>(addr) == 0xFFFF) return false;

      startWrite();
      preCommandList();
      command_list(addr);
      postCommandList();
      endWrite();
      return true;
    }

    void command_list(const std::uint8_t *addr)
    {
      for (;;)
      {                // For each command...
        if (*reinterpret_cast<const std::uint16_t*>(addr) == 0xFFFF) break;
        writeCommand(*addr++);  // Read, issue command
        std::uint_fast8_t numArgs = *addr++;  // Number of args to follow
        std::uint_fast8_t ms = numArgs & CMD_INIT_DELAY;       // If hibit set, delay follows args
        numArgs &= ~CMD_INIT_DELAY;          // Mask out delay bit
        if (numArgs)
        {
          do {                   // For each argument...
            writeData(*addr++);  // Read, issue argument
          } while (--numArgs);
        }
        if (ms) {
          ms = *addr++;        // Read post-command delay time (ms)
          delay( (ms==255 ? 500 : ms) );
        }
      }
    }

    board_t getBoard(void) const { return board; }

  protected:
    PanelCommon* _panel = nullptr;
    TouchCommon* _touch = nullptr;

    board_t board = lgfx::board_t::board_unknown;

    float _touch_affine[6] = {1,0,0,0,1,0};

    bool _in_transaction = false;

    virtual void preInit(void) {}
    virtual void preCommandList(void) {}
    virtual void postCommandList(void) {}
    virtual void postSetPanel(void) {}
    virtual void postSetRotation(void) {}
    virtual void postSetTouch(void)
    {
      std::uint16_t xmin = _touch->x_min;
      std::uint16_t xmax = _touch->x_max;
      std::uint16_t ymin = _touch->y_min;
      std::uint16_t ymax = _touch->y_max;
      std::uint16_t parameters[8] = 
        { xmin, ymin
        , xmin, ymax
        , xmax, ymin
        , xmax, ymax };
      set_touch_calibrate(parameters);
    }

    void postSetColorDepth(void)
    {
      _write_conv.setColorDepth(_panel->write_depth);
      _read_conv.setColorDepth(_panel->read_depth);
    }

    virtual void init_impl(bool use_reset = true)
    {
      initBus(); 
      initPanel(use_reset); 
      initTouch(); 
      if (use_reset) { clear(); }
    }

    bool isReadable_impl(void) const override { return _panel->spi_read; }
    std::int_fast8_t getRotation_impl(void) const override { return _panel->rotation; }

    void copyRect_impl(std::int32_t dst_x, std::int32_t dst_y, std::int32_t w, std::int32_t h, std::int32_t src_x, std::int32_t src_y) override
    {
      pixelcopy_t p((void*)nullptr, _write_conv.depth, _read_conv.depth);
      if (w < h) {
        const std::uint32_t buflen = h * _write_conv.bytes;
        std::uint8_t buf[buflen];
        std::int32_t add = (src_x < dst_x) ?   - 1 : 1;
        std::int32_t pos = (src_x < dst_x) ? w - 1 : 0;
        do {
          readRect_impl(src_x + pos, src_y, 1, h, buf, &p);
          setWindow_impl(dst_x + pos, dst_y, dst_x + pos, dst_y + h - 1);
          writePixelsDMA_impl(buf, h);
          pos += add;
        } while (--w);
        waitDMA_impl();
      } else {
        const std::uint32_t buflen = w * _write_conv.bytes;
        std::uint8_t buf[buflen];
        std::int32_t add = (src_y < dst_y) ?   - 1 : 1;
        std::int32_t pos = (src_y < dst_y) ? h - 1 : 0;
        do {
          readRect_impl(src_x, src_y + pos, w, 1, buf, &p);
          setWindow_impl(dst_x, dst_y + pos, dst_x + w - 1, dst_y + pos);
          writePixelsDMA_impl(buf, w);
          pos += add;
        } while (--h);
        waitDMA_impl();
      }
    }

    void pushImageARGB_impl(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, pixelcopy_t* param) override
    {
      auto src_x = param->src_x;
      auto buffer = reinterpret_cast<argb8888_t*>(const_cast<void*>(param->src_data));
      auto bytes = _write_conv.bytes;
/*
      this->setWindow(x, y, x + w, y);
      std::uint8_t* dmabuf = get_dmabuffer(w * bytes);
      memset(dmabuf, 0, w * bytes);
      param->fp_copy(dmabuf, 0, w, param);
      this->writePixelsDMA_impl(dmabuf, w);
      return;
//*/
      pixelcopy_t pc_read(nullptr, static_cast<color_depth_t>(_write_conv.depth), _read_conv.depth);
      for (;;)
      {
        std::uint8_t* dmabuf = get_dmabuffer((w+1) * bytes);
        std::int32_t xstart = 0, drawed_x = 0;
        do
        {
          std::uint_fast8_t a = buffer[xstart].a;
          if (!a)
          {
            if (drawed_x < xstart)
            {
              param->src_x = drawed_x;
              param->fp_copy(dmabuf, drawed_x, xstart, param);
              this->setWindow(x + drawed_x, y, x + xstart, y);
              this->writePixelsDMA_impl(dmabuf + drawed_x * bytes, (xstart - drawed_x));
            }
            drawed_x = xstart + 1;
          }
          else
          {
            while (255 == buffer[xstart].a && ++xstart != w);
            if (xstart == w) break;
            std::int32_t j = xstart;
            while (++j != w && buffer[j].a && buffer[j].a != 255);
            read_rect(x + xstart, y, j - xstart + 1, 1, &dmabuf[xstart * bytes], &pc_read);
            if (w == (xstart = j)) break;
          }
        } while (++xstart != w);
        if (drawed_x < xstart)
        {
          param->src_x = drawed_x;
          param->fp_copy(dmabuf, drawed_x, xstart, param);
          this->setWindow(x + drawed_x, y, x + xstart, y);
          this->writePixelsDMA_impl(dmabuf + drawed_x * bytes, (xstart - drawed_x));
        }
        if (!--h) return;
        param->src_x = src_x;
        param->src_y++;
        ++y;
      }
    }

    struct _dmabufs_t {
      std::uint8_t* buffer = nullptr;
      std::uint32_t length = 0;
      void free(void) {
        if (buffer) {
          heap_free(buffer);
          buffer = nullptr;
          length = 0;
        }
      }
    };

    std::uint8_t* get_dmabuffer(std::uint32_t length)
    {
      _dma_flip = !_dma_flip;
      length = (length + 3) & ~3;
      if (_dmabufs[_dma_flip].length != length) {
        _dmabufs[_dma_flip].free();
        _dmabufs[_dma_flip].buffer = (std::uint8_t*)heap_alloc_dma(length);
        _dmabufs[_dma_flip].length = _dmabufs[_dma_flip].buffer ? length : 0;
      }
      return _dmabufs[_dma_flip].buffer;
    }

    void delete_dmabuffer(void)
    {
      _dmabufs[0].free();
      _dmabufs[1].free();
    }

    void cs_h(void) {
      gpio_hi(_panel->spi_cs);
    }
    void cs_l(void) {
      gpio_lo(_panel->spi_cs);
    }

  private:
    bool _dma_flip = false;
    _dmabufs_t _dmabufs[2];


    void draw_calibrate_point(std::int32_t x, std::int32_t y, std::int32_t r, std::uint32_t fg_rawcolor, std::uint32_t bg_rawcolor)
    {
      setRawColor(bg_rawcolor);
      fillRect(x - r, y - r, r * 2 + 1, r * 2 + 1);
      if (fg_rawcolor == bg_rawcolor) return;
      startWrite();
      setRawColor(fg_rawcolor);
      fillRect(x - 1, y - r, 3, r * 2 + 1);
      fillRect(x - r, y - 1, r * 2 + 1, 3);
      for (std::int32_t i = - r + 1; i < r; ++i) {
        drawFastHLine(x + i - 1, y + i, 3);
        drawFastHLine(x - i - 1, y + i, 3);
      }
      drawFastHLine(x + r - 1, y + r, 2);
      drawFastHLine(x - r    , y + r, 2);
      drawFastHLine(x - r    , y - r, 2);
      drawFastHLine(x + r - 1, y - r, 2);
      endWrite();
    }

    void calibrate_touch(std::uint16_t *parameters, std::uint32_t fg_rawcolor, std::uint32_t bg_rawcolor, std::uint8_t size)
    {
      if (nullptr == _touch) return;
      auto rot = getRotation();
      setRotation(0);

      std::uint16_t orig[8];
      for (int i = 0; i < 4; ++i) {
        std::int32_t px = (_width -  1) * ((i>>1) & 1);
        std::int32_t py = (_height - 1) * ( i     & 1);
        draw_calibrate_point( px, py, size, fg_rawcolor, bg_rawcolor);
        delay(500);
        std::int32_t x_touch = 0, y_touch = 0;
        static constexpr int _RAWERR = 20;
        std::int32_t x_tmp, y_tmp, x_tmp2, y_tmp2;
        for (int j = 0; j < 8; ++j) {
          do {
            do { delay(1); } while (!getTouchRaw(&x_tmp,&y_tmp));
            delay(2); // Small delay to the next sample
          } while (!getTouchRaw(&x_tmp2,&y_tmp2)
                 || (abs(x_tmp - x_tmp2) > _RAWERR)
                 || (abs(y_tmp - y_tmp2) > _RAWERR));

          x_touch += x_tmp;
          x_touch += x_tmp2;
          y_touch += y_tmp;
          y_touch += y_tmp2;
        }
        orig[i*2  ] = x_touch >> 4;
        orig[i*2+1] = y_touch >> 4;
        draw_calibrate_point( px, py, size, bg_rawcolor, bg_rawcolor);
        while (getTouchRaw());
      }
      if (nullptr != parameters) {
        memcpy(parameters, orig, sizeof(std::uint16_t) * 8);
      }
      set_touch_calibrate(orig);
      setRotation(rot);
    }

    void set_touch_calibrate(std::uint16_t *parameters)
    {
      std::uint32_t vect[6] = {0,0,0,0,0,0};
      float mat[3][3] = {{0,0,0},{0,0,0},{0,0,0}};

      bool r = getRotation() & 1;
      std::int32_t w = r ? _height : _width;
      std::int32_t h = r ? _width : _height;
      --w;
      --h;
      float a;
      for ( int i = 0; i < 4; ++i ) {
        std::int32_t tx = w * ((i>>1) & 1);
        std::int32_t ty = h * ( i     & 1);
        std::int32_t px = parameters[i*2  ];
        std::int32_t py = parameters[i*2+1];
        a = px * px;
        mat[0][0] += a;
        a = px * py;
        mat[0][1] += a;
        mat[1][0] += a;
        a = px;
        mat[0][2] += a;
        mat[2][0] += a;
        a = py * py;
        mat[1][1] += a;
        a = py;
        mat[1][2] += a;
        mat[2][1] += a;
        mat[2][2] += 1;
 
        vect[0] += px * tx;
        vect[1] += py * tx;
        vect[2] +=      tx;
        vect[3] += px * ty;
        vect[4] += py * ty;
        vect[5] +=      ty;
      }

      {
        float det = 1;
        for ( int k = 0; k < 3; ++k )
        {
          float t = mat[k][k];
          det *= t;
          for ( int i = 0; i < 3; ++i ) mat[k][i] /= t;

          mat[k][k] = 1 / t;
          for ( int j = 0; j < 3; ++j )
          {
            if ( j == k ) continue;

            float u = mat[j][k];

            for ( int i = 0; i < 3; ++i )
            {
              if ( i != k ) mat[j][i] -= mat[k][i] * u;
              else mat[j][i] = -u / t;
            }
          }
        }
      }

      float v0 = vect[0];
      float v1 = vect[1];
      float v2 = vect[2];
      _touch_affine[0] = mat[0][0] * v0 + mat[0][1] * v1 + mat[0][2] * v2;
      _touch_affine[1] = mat[1][0] * v0 + mat[1][1] * v1 + mat[1][2] * v2;
      _touch_affine[2] = mat[2][0] * v0 + mat[2][1] * v1 + mat[2][2] * v2;
      float v3 = vect[3];
      float v4 = vect[4];
      float v5 = vect[5];
      _touch_affine[3] = mat[0][0] * v3 + mat[0][1] * v4 + mat[0][2] * v5;
      _touch_affine[4] = mat[1][0] * v3 + mat[1][1] * v4 + mat[1][2] * v5;
      _touch_affine[5] = mat[2][0] * v3 + mat[2][1] * v4 + mat[2][2] * v5;
    }
  };

//----------------------------------------------------------------------------

}

using lgfx::LGFX_Device;

#endif
