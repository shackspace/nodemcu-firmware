# **NodeMCU 1.5.1** #

[![Join the chat at https://gitter.im/nodemcu/nodemcu-firmware](https://img.shields.io/gitter/room/badges/shields.svg)](https://gitter.im/nodemcu/nodemcu-firmware?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Build Status](https://travis-ci.org/nodemcu/nodemcu-firmware.svg)](https://travis-ci.org/nodemcu/nodemcu-firmware)
[![Documentation Status](https://readthedocs.org/projects/nodemcu/badge/?version=dev)](http://nodemcu.readthedocs.org/)
[![License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](https://github.com/nodemcu/nodemcu-firmware/blob/master/LICENSE)

### A Lua based firmware for ESP8266 WiFi SOC

NodeMCU is an [eLua](http://www.eluaproject.net/) based firmware for the [ESP8266 WiFi SOC from Espressif](http://espressif.com/en/products/esp8266/). The firmware is based on the [Espressif NON-OS SDK 1.5.1](http://bbs.espressif.com/viewtopic.php?f=46&p=5315) and uses a file system based on [spiffs](https://github.com/pellepl/spiffs). The code repository consists of 98.1% C-code that glues the thin Lua veneer to the SDK.

The NodeMCU *firmware* is a companion project to the popular [NodeMCU dev kits](https://github.com/nodemcu/nodemcu-devkit-v1.0), ready-made open source development boards with ESP8266-12E chips.

# Summary

- Easy to program wireless node and/or access point
- Based on Lua 5.1.4 (without *debug, os* modules)
- Asynchronous event-driven programming model
- 35+ [built-in modules](https://github.com/nodemcu/nodemcu-firmware/wiki/Module-list)
- Firmware available with or without floating point support (integer-only uses less memory)
- Up-to-date documentation at [https://nodemcu.readthedocs.org](https://nodemcu.readthedocs.org)

# Programming Model

The NodeMCU programming model is similar to that of [Node.js](https://en.wikipedia.org/wiki/Node.js), only in Lua. It is asynchronous and event-driven. Many functions, therefore, have parameters for callback functions. To give you an idea what a NodeMCU program looks like study the short snippets below. For more extensive examples have a look at the [`/lua_examples`](lua_examples) folder in the repository on GitHub.

```lua
-- a simple HTTP server
srv = net.createServer(net.TCP)
srv:listen(80, function(conn)
	conn:on("receive", function(conn, payload)
		print(payload)
		conn:send("<h1> Hello, NodeMCU.</h1>")
	end)
	conn:on("sent", function(conn) conn:close() end)
end)
```
```lua
-- connect to WiFi access point
wifi.setmode(wifi.STATION)
wifi.sta.config("SSID", "password")
```

# Documentation

The entire [NodeMCU documentation](https://nodemcu.readthedocs.org) is maintained right in this repository at [/docs](docs). The fact that the API documentation is mainted in the same repository as the code that *provides* the API ensures consistency between the two. With every commit the documentation is rebuilt by Read the Docs and thus transformed from terse Markdown into a nicely browsable HTML site at [https://nodemcu.readthedocs.org](https://nodemcu.readthedocs.org).

- How to [build the firmware](https://nodemcu.readthedocs.org/en/dev/en/build/)
- How to [flash the firmware](https://nodemcu.readthedocs.org/en/dev/en/flash/)
- How to [upload code and NodeMCU IDEs](https://nodemcu.readthedocs.org/en/dev/en/upload/)
- API documentation for every module

# Support

See [https://nodemcu.readthedocs.org/en/dev/en/support/](https://nodemcu.readthedocs.org/en/dev/en/support/).

# License

[MIT](https://github.com/nodemcu/nodemcu-firmware/blob/master/LICENSE) © [zeroday](https://github.com/NodeMCU)/[nodemcu.com](http://nodemcu.com/index_en.html)

# Build Options

The following sections explain some of the options you have if you want to [build your own NodeMCU firmware](http://nodemcu.readthedocs.org/en/dev/en/build/).

### Select Modules

Disable modules you won't be using to reduce firmware size and free up some RAM. The ESP8266 is quite limited in available RAM and running out of memory can cause a system panic.

Edit `app/include/user_modules.h` and comment-out the `#define` statement for modules you don't need. Example:

```c
...
#define LUA_USE_MODULES_MQTT
// #define LUA_USE_MODULES_COAP
// #define LUA_USE_MODULES_U8G
...
```

### Tag Your Build

Identify your firmware builds by editing `app/include/user_version.h`

```c
#define NODE_VERSION    "NodeMCU 1.5.1+myname"
#ifndef BUILD_DATE
#define BUILD_DATE      "YYYYMMDD"
#endif
```

### Set UART Bit Rate

The initial baud rate at boot time is 9600bps. You can change this by
editing `BIT_RATE_DEFAULT`  in `app/include/user_config.h`:

```c
#define BIT_RATE_DEFAULT BIT_RATE_115200
```

### Debugging

To enable runtime debug messages to serial console edit `app/include/user_config.h`

```c
#define DEVELOP_VERSION
```

`DEVELOP_VERSION` changes the startup baud rate to 74880.

# Flash the firmware

## Flash tools for Windows

You can use the [nodemcu-flasher](https://github.com/nodemcu/nodemcu-flasher) to burn the firmware.

## Flash tools for Linux

Esptool is a python utility which can read and write the flash in an ESP8266 device. See https://github.com/themadinventor/esptool

## Preparing the hardware for firmware upgrade

To enable ESP8266 firmware flashing, the GPIO0 pin must be pulled low before
the device is reset. Conversely, for a normal boot, GPIO0 must be pulled high
or floating.

If you have a [NodeMCU Development Kit](http://www.nodemcu.com/index_en.html) then
you don't need to do anything, as the USB connection can pull GPIO0
low by asserting DTR, and reset your board by asserting RTS.

If you have an ESP-01 or other device without inbuilt USB, you will need to
enable flashing yourself by pulling GPIO0 low or pressing a "flash" switch.

## Files to burn to the flash

If you got your firmware from [NodeMCU custom builds](http://frightanic.com/nodemcu-custom-build) then you can flash that file directly to address 0x00000.

Otherwise, if you built your own firmware from source code:
  - bin/0x00000.bin to 0x00000
  - bin/0x10000.bin to 0x10000

Also, in some special circumstances, you may need to flash `blank.bin` or `esp_init_data_default.bin` to various addresses on the flash (depending on flash size and type).

If upgrading from `spiffs` version 0.3.2 to 0.3.3 or later, or after flashing any new firmware (particularly one with a much different size), you may need to run `file.format()` to re-format your flash filesystem.
You will know if you need to do this because your flash files disappeared, or they exist but seem empty, or data cannot be written to new files.

# Connecting to your NodeMCU device

NodeMCU serial interface uses 9600 baud at boot time. To increase the speed after booting, issue `uart.setup(0,115200,8,0,1,1)` (ESPlorer will do this automatically when changing the speed in the dropdown list).

If the device panics and resets at any time, errors will be written to the serial interface at 115200 bps.

# User Interface tools

## Esplorer

Victor Brutskiy's [ESPlorer](https://github.com/4refr0nt/ESPlorer) is written in Java, is open source and runs on most platforms such as Linux, Windows, Mac OS, etc.

#### Features

  - Edit Lua scripts and run on the ESP8266 and save to its flash
  - Serial console log
  - Also supports original AT firmware (reading and setting WiFi modes, etc)

## NodeMCU Studio

[NodeMCU Studio](https://github.com/nodemcu/nodemcu-studio-csharp) is written in C# and supports Windows. This software is open source and can write lua files to filesystem.

# OPTIONAL MODULES

####Use DS18B20 module extends your esp8266
```lua
    -- read temperature with DS18B20
    node.compile("ds18b20.lua")   --  run this only once to compile and save to "ds18b20.lc"
    t=require("ds18b20")
    t.setup(9)
    addrs=t.addrs()
    -- Total DS18B20 numbers, assume it is 2
    print(table.getn(addrs))
    -- The first DS18B20
    print(t.read(addrs[1],t.C))
    print(t.read(addrs[1],t.F))
    print(t.read(addrs[1],t.K))
    -- The second DS18B20
    print(t.read(addrs[2],t.C))
    print(t.read(addrs[2],t.F))
    print(t.read(addrs[2],t.K))
    -- Just read
    print(t.read())
    -- Just read as centigrade
    print(t.read(nil,t.C))
    -- Don't forget to release it after use
    t = nil
	ds18b20 = nil
    package.loaded["ds18b20"]=nil
```

####Operate a display with u8glib
u8glib is a graphics library with support for many different displays. The nodemcu firmware supports a subset of these.
Both I2C and SPI:
* sh1106_128x64
* ssd1306 - 128x64 and 64x48 variants
* ssd1309_128x64
* ssd1327_96x96_gr
* uc1611 - dogm240 and dogxl240 variants

SPI only:
* ld7032_60x32
* pcd8544_84x48
* pcf8812_96x65
* ssd1322_nhd31oled - bw and gr variants
* ssd1325_nhd27oled - bw and gr variants
* ssd1351_128x128 - gh and hicolor variants
* st7565_64128n - variants 64128n, dogm128/132, lm6059/lm6063, c12832/c12864
* uc1601_c128032
* uc1608 - 240x128 and 240x64 variants
* uc1610_dogxl160 - bw and gr variants
* uc1611 - dogm240 and dogxl240 variants
* uc1701 - dogs102 and mini12864 variants

U8glib v1.18.1

#####I2C connection
Hook up SDA and SCL to any free GPIOs. Eg. [u8g_graphics_test.lua](lua_examples/u8glib/u8g_graphics_test.lua) expects SDA=5 (GPIO14) and SCL=6 (GPIO12). They are used to set up nodemcu's I2C driver before accessing the display:
```lua
sda = 5
scl = 6
i2c.setup(0, sda, scl, i2c.SLOW)
```

#####SPI connection
The HSPI module is used, so certain pins are fixed:
* HSPI CLK  = GPIO14
* HSPI MOSI = GPIO13
* HSPI MISO = GPIO12 (not used)

All other pins can be assigned to any available GPIO:
* CS
* D/C
* RES (optional for some displays)

Also refer to the initialization sequence eg in [u8g_graphics_test.lua](lua_examples/u8glib/u8g_graphics_test.lua):
```lua
spi.setup(1, spi.MASTER, spi.CPOL_LOW, spi.CPHA_LOW, 8, 8)
```


#####Library usage
The Lua bindings for this library closely follow u8glib's object oriented C++ API. Based on the u8g class, you create an object for your display type.

SSD1306 via I2C:
```lua
sla = 0x3c
disp = u8g.ssd1306_128x64_i2c(sla)
```
SSD1306 via SPI:
```lua
cs  = 8 -- GPIO15, pull-down 10k to GND
dc  = 4 -- GPIO2
res = 0 -- GPIO16, RES is optional YMMV
disp = u8g.ssd1306_128x64_hw_spi(cs, dc, res)
```

This object provides all of u8glib's methods to control the display.
Again, refer to [u8g_graphics_test.lua](lua_examples/u8glib/u8g_graphics_test.lua) to get an impression how this is achieved with Lua code. Visit the [u8glib homepage](https://github.com/olikraus/u8glib) for technical details.

#####Displays
I2C and HW SPI based displays with support in u8glib can be enabled. To get access to the respective constructors, add the desired entries to the I2C or SPI display tables in [app/include/u8g_config.h](app/include/u8g_config.h):
```c
#define U8G_DISPLAY_TABLE_I2C                   \
    U8G_DISPLAY_TABLE_ENTRY(ssd1306_128x64_i2c) \

#define U8G_DISPLAY_TABLE_SPI                      \
    U8G_DISPLAY_TABLE_ENTRY(ssd1306_128x64_hw_spi) \
    U8G_DISPLAY_TABLE_ENTRY(pcd8544_84x48_hw_spi)  \
    U8G_DISPLAY_TABLE_ENTRY(pcf8812_96x65_hw_spi)  \
```
An exhaustive list of available displays can be found in the [u8g module wiki entry](https://github.com/nodemcu/nodemcu-firmware/wiki/nodemcu_api_en#u8g-module).


#####Fonts
u8glib comes with a wide range of fonts for small displays. Since they need to be compiled into the firmware image, you'd need to include them in [app/include/u8g_config.h](app/include/u8g_config.h) and recompile. Simply add the desired fonts to the font table:
```c
#define U8G_FONT_TABLE \
    U8G_FONT_TABLE_ENTRY(font_6x10)  \
    U8G_FONT_TABLE_ENTRY(font_chikita)
```
They'll be available as `u8g.<font_name>` in Lua.

#####Bitmaps
Bitmaps and XBMs are supplied as strings to `drawBitmap()` and `drawXBM()`. This off-loads all data handling from the u8g module to generic methods for binary files. See [u8g_bitmaps.lua](lua_examples/u8glib/u8g_bitmaps.lua).
In contrast to the source code based inclusion of XBMs into u8glib, it's required to provide precompiled binary files. This can be performed online with [Online-Utility's Image Converter](http://www.online-utility.org/image_converter.jsp): Convert from XBM to MONO format and upload the binary result with [nodemcu-uploader.py](https://github.com/kmpm/nodemcu-uploader).

#####Unimplemented functions
- [ ] Cursor handling
  - [ ] disableCursor()
  - [ ] enableCursor()
  - [ ] setCursorColor()
  - [ ] setCursorFont()
  - [ ] setCursorPos()
  - [ ] setCursorStyle()
- [ ] General functions
  - [ ] setContrast()
  - [ ] setPrintPos()
  - [ ] setHardwareBackup()
  - [ ] setRGB()
  - [ ] setDefaultMidColor()

####Operate a display with ucglib
Ucglib is a graphics library with support for color TFT displays.

Ucglib v1.3.3

#####SPI connection
The HSPI module is used, so certain pins are fixed:
* HSPI CLK  = GPIO14
* HSPI MOSI = GPIO13
* HSPI MISO = GPIO12 (not used)

All other pins can be assigned to any available GPIO:
* CS
* D/C
* RES (optional for some displays)

Also refer to the initialization sequence eg in [GraphicsTest.lua](lua_examples/ucglib/GraphicsRest.lua):
```lua
spi.setup(1, spi.MASTER, spi.CPOL_LOW, spi.CPHA_LOW, 8, 8)
```

#####Library usage
The Lua bindings for this library closely follow ucglib's object oriented C++ API. Based on the ucg class, you create an object for your display type.

ILI9341 via SPI:
```lua
cs  = 8 -- GPIO15, pull-down 10k to GND
dc  = 4 -- GPIO2
res = 0 -- GPIO16, RES is optional YMMV
disp = ucg.ili9341_18x240x320_hw_spi(cs, dc, res)
```

This object provides all of ucglib's methods to control the display.
Again, refer to [GraphicsTest.lua](lua_examples/ucglib/GraphicsTest.lua) to get an impression how this is achieved with Lua code. Visit the [ucglib homepage](https://github.com/olikraus/ucglib) for technical details.

#####Displays
To get access to the display constructors, add the desired entries to the display table in [app/include/ucg_config.h](app/include/ucg_config.h):
```c
#define UCG_DISPLAY_TABLE                          \
    UCG_DISPLAY_TABLE_ENTRY(ili9341_18x240x320_hw_spi, ucg_dev_ili9341_18x240x320, ucg_ext_ili9341_18) \
    UCG_DISPLAY_TABLE_ENTRY(st7735_18x128x160_hw_spi, ucg_dev_st7735_18x128x160, ucg_ext_st7735_18) \
```

#####Fonts
ucglib comes with a wide range of fonts for small displays. Since they need to be compiled into the firmware image, you'd need to include them in [app/include/ucg_config.h](app/include/ucg_config.h) and recompile. Simply add the desired fonts to the font table:
```c
#define UCG_FONT_TABLE                              \
    UCG_FONT_TABLE_ENTRY(font_7x13B_tr)             \
    UCG_FONT_TABLE_ENTRY(font_helvB12_hr)           \
    UCG_FONT_TABLE_ENTRY(font_helvB18_hr)           \
    UCG_FONT_TABLE_ENTRY(font_ncenR12_tr)           \
    UCG_FONT_TABLE_ENTRY(font_ncenR14_hr)
```
They'll be available as `ucg.<font_name>` in Lua.


####Control a WS2812 based light strip
```lua
	-- set the color of one LED on GPIO2 to red
	ws2812.writergb(4, string.char(255, 0, 0))
	-- set the color of 10 LEDs on GPIO0 to blue
	ws2812.writergb(3, string.char(0, 0, 255):rep(10))
	-- first LED green, second LED white
	ws2812.writergb(4, string.char(0, 255, 0, 255, 255, 255))
```

####coap client and server
```lua
-- use copper addon for firefox
cs=coap.Server()
cs:listen(5683)

myvar=1
cs:var("myvar") -- get coap://192.168.18.103:5683/v1/v/myvar will return the value of myvar: 1

all='[1,2,3]'
cs:var("all", coap.JSON) -- sets content type to json

-- function should tack one string, return one string.
function myfun(payload)
  print("myfun called")
  respond = "hello"
  return respond
end
cs:func("myfun") -- post coap://192.168.18.103:5683/v1/f/myfun will call myfun

cc = coap.Client()
cc:get(coap.CON, "coap://192.168.18.100:5683/.well-known/core")
cc:post(coap.NON, "coap://192.168.18.100:5683/", "Hello")
```

####cjson
```lua
-- Note that when cjson deal with large content, it may fails a memory allocation, and leaks a bit of memory.
-- so it's better to detect that and schedule a restart.
--
-- Translate Lua value to/from JSON
-- text = cjson.encode(value)
-- value = cjson.decode(text)
json_text = '[ true, { "foo": "bar" } ]'
value = cjson.decode(json_text)
-- Returns: { true, { foo = "bar" } }
value = { true, { foo = "bar" } }
json_text = cjson.encode(value)
-- Returns: '[true,{"foo":"bar"}]'
```

####Read an HX711 load cell ADC.
Note: currently only chanel A with gain 128 is supported.
The HX711 is an inexpensive 24bit ADC with programmable 128x, 64x, and 32x gain.
```lua
	-- Initialize the hx711 with clk on pin 5 and data on pin 6
	hx711.init(5,6)
	-- Read ch A with 128 gain.
	raw_data = hx711.read(0)
```

####Universal DHT Sensor support
Support DHT11, DHT21, DHT22, DHT33, DHT44, etc.
Use all-in-one function to read DHT sensor.
```lua

pin = 5
status,temp,humi,temp_decimial,humi_decimial = dht.readxx(pin)
if( status == dht.OK ) then
  -- Integer firmware using this example
  print(
    string.format(
      "DHT Temperature:%d.%03d;Humidity:%d.%03d\r\n",
      math.floor(temp),
      temp_decimial,
      math.floor(humi),
      humi_decimial
    )
  )
  -- Float firmware using this example
  print("DHT Temperature:"..temp..";".."Humidity:"..humi)
elseif( status == dht.ERROR_CHECKSUM ) then
  print( "DHT Checksum error." );
elseif( status == dht.ERROR_TIMEOUT ) then
  print( "DHT Time out." );
end

```
### Well hello there!

This repository is meant to provide an example for *forking* a repository on GitHub.

Creating a *fork* is producing a personal copy of someone else's project. Forks act as a sort of bridge between the original repository and your personal copy. You can submit *Pull Requests* to help make other people's projects better by offering your changes up to the original project. Forking is at the core of social coding at GitHub.

After forking this repository, you can make some changes to the project, and submit [a Pull Request](https://github.com/octocat/Spoon-Knife/pulls) as practice.

For some more information on how to fork a repository, [check out our guide, "Forking Projects""](http://guides.github.com/overviews/forking/). Thanks! :sparkling_heart:
