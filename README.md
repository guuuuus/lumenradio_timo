# C lib for LumenRadio's TiMo(two)
Library for controlling the lumenradio timotwo module thru spi. 
This library doens't use interrupts or dma, communication over the spi in a blocking manner. This maens the mcu is, during the spi transaction is mostly waiting, since the max speed of the timo is only 2mhz, and will probably be slower due to prescaler factors. It is reconmended to set the timo to the desired mode, set a dmx window, and set an irq and only read the irq register. (this lib currently doens't support external interrupt, since the irq pin is also used in the spi transaction).


### device info 
lumenradio doesn't appear to have a datasheet, so see the docs for info on the spi interface.  
[lumenradio docs](https://docs.lumenrad.io/timotwo/)  
[lumenradio product page](https://lumenradio.com/products/timotwo/)