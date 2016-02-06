# UART Module
The [UART](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver/transmitter) (Universal asynchronous receiver/transmitter) module allows configuration of and communication over the UART serial port.

## uart.alt()
Change UART pin assignment.

#### Syntax
`uart.alt(on)`

#### Parameters
`on`:
- `0` use standard pins
- `1` use alternate pins GPIO13 and GPIO15

#### Returns
`nil`

## uart.on()

Sets the callback function to handle UART events.

Currently only the "data" event is supported.

#### Syntax
`uart.on(method, [number/end_char], [function], [run_input])`

#### Parameters
- `method` "data", data has been received on the UART
- `number/end_char`
	- if pass in a number n<255, the callback will called when n chars are received.
	- if n=0, will receive every char in buffer.
	- if pass in a one char string "c", the callback will called when "c" is encounterd, or max n=255 received.
- `function` callback function, event "data" has a callback like this: `function(data) end`
- `run_input` 0 or 1. If 0, input from UART will not go into Lua interpreter, can accept binary data. If 1, input from UART will go into Lua interpreter, and run.

To unregister the callback, provide only the "data" parameter.

#### Returns
`nil`

#### Example
```lua
-- when 4 chars is received.
uart.on("data", 4,
  function(data)
	print("receive from uart:", data)
	if data=="quit" then
	  uart.on("data") -- unregister callback function
	end
end, 0)
-- when '\r' is received.
uart.on("data", "\r",
  function(data)
	print("receive from uart:", data)
	if data=="quit\r" then
	  uart.on("data") -- unregister callback function
	end
end, 0)
```

## uart.setup()

(Re-)configures the communication parameters of the UART.

#### Syntax
`uart.setup(id, baud, databits, parity, stopbits, echo)`

#### Parameters
- `id` always zero, only one uart supported
- `baud` one of 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200, 230400, 460800, 921600, 1843200, 2686400
- `databits` one of 5, 6, 7, 8
- `parity` `uart.PARITY_NONE`, `uart.PARITY_ODD`, or `uart.PARITY_EVEN`
- `stopbits` `uart.STOPBITS_1`, `uart.STOPBITS_1_5`, or `uart.STOPBITS_2`
- `echo` if 0, disable echo, otherwise enable echo

#### Returns
configured baud rate (number)

#### Example
```lua
-- configure for 9600, 8N1, with echo
uart.setup(0, 9600, 8, uart.PARITY_NONE, uart.STOPBITS_1, 1)
```

## uart.write()

Write string or byte to the UART.

#### Syntax
`uart.write(id, data1 [, data2, ...])`

#### Parameters
- `id` always 0, only one UART supported
- `data1`... string or byte to send via UART

#### Returns
`nil`

#### Example
```lua
uart.write(0, "Hello, world\n")
```

