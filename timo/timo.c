#include "timo.h"

unsigned char TiMo_readRegP(unsigned char cmd, unsigned char *data, unsigned char len, timo_t *fn)
{
    if (len > 128)
        return 2;
    unsigned char irqstatus = 0x00;
    unsigned long count = 0;
    unsigned char times = 0;

    // clear falling counter
    fn->irq_pinCl();
    // set ce line
    fn->cs_l();
    // delay as specified in datasheet for 4 us
    fn->waitUs(5); // spec says 4?
    // write command
    irqstatus = fn->spi_send_rec(cmd);

    // ce line high
    fn->cs_h();
    // check if irq is busy

    while ((irqstatus & TIMO_IRQF_SPI_BUSY))
    // while ((irqstatus & 0xff))
    {
        fn->cs_l();
        fn->waitUs(4);
        irqstatus = fn->spi_send_rec(cmd);
        times++;
        fn->cs_h();
        if (times > 100)
            break;
    }
    if (times > 100)
    {
        return 1;
    }

    while ((fn->irq_pinCl() == 0) && (count < 80000))
    {
        count++;
        fn->waitUs(10);
    }

    // set ce low
    fn->cs_l();
    // delay as specified in datasheet for 4 us
    fn->waitUs(5);

    irqstatus = fn->spi_send_rec(TIMO_COMMAND_NOP); // write (nothing)

    // loop data
    for (unsigned char i = 0; i < len; i++)
    {
        // read data
        data[i] = fn->spi_send_rec(TIMO_COMMAND_NOP);
    }

    // end with ce high
    fn->cs_h();
    count = 0;

    return 0;
}

// disererd register, pointer to data, lenght of data
unsigned char TiMo_writeRegP(unsigned char cmd, unsigned char *data, unsigned char len, timo_t *fn)
{
    if (len > 128)
        return 2;
    unsigned char irqstatus = 0;
    unsigned long count = 0;
    unsigned char times = 0;

    // clear falling counter
    fn->irq_pinCl();
    // set ce line
    fn->cs_l();

    // delay as specified in datasheet for 4 us
    fn->waitUs(5);

    // write command get irq status as return
    irqstatus = fn->spi_send_rec(cmd);

    // ce line high
    fn->cs_h();

    // check if irq is busy
    while ((irqstatus & TIMO_IRQF_SPI_BUSY))
    {
        fn->cs_l();
        fn->waitUs(5);

        irqstatus = fn->spi_send_rec(cmd);
        times++;
        fn->cs_h();

        if (times > 10)
            break;
    }
    if (times > 10)
    {
        return 1;
    }

    while ((fn->irq_pinCl() == 0) && (count < 80000))
    // while ((GPIO_ReadInputPin(timo_irqPin)))
    {
        count++;
        fn->waitUs(5);
    }
    // set ce low
    fn->cs_l();

    fn->waitUs(5);

    irqstatus = fn->spi_send_rec(TIMO_COMMAND_NOP);

    // check if irq is busy
    if (irqstatus & TIMO_IRQF_SPI_BUSY)
    {
        fn->cs_h();
        return 1;
    }

    for (unsigned char i = 0; i < len; i++)
    {
        fn->spi_send_rec(data[i]); // write data
    }
    // end with ce high
    fn->cs_h();
    count = 0;

    return 0;
}

unsigned char TiMo_readReg8(unsigned char cmd, timo_t *fn)
{
    unsigned char d[] = {TIMO_COMMAND_NOP};

    TiMo_readRegP(cmd, d, 1, fn);
    return d[0];
}

// disererd register, poter to data, lenght of data
void TiMo_writeReg8(unsigned char cmd, unsigned char data, timo_t *fn)
{
    TiMo_writeRegP(cmd, &data, 1, fn);
}

// lets just say an addres should start at 0?
void TiMo_setDMXwindow(unsigned short window, unsigned short address, timo_t *fn)
{
    unsigned char d[4];

    if (!window)
        return;
    if ((address + window) > 512)
        return;

    d[1] = (unsigned char)window;
    d[0] = (unsigned char)window >> 8;
    d[3] = (unsigned char)address;
    d[2] = (unsigned char)address >> 8;
    TiMo_writeRegP((TIMO_COMMAND_WRITE_REG | TIMO_REG_DMX_WINDOW), d, 4, fn);
}

// enables the irq
void TiMo_setIRQ(unsigned char irq, timo_t *fn)
{
    unsigned char curIRQ = TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_IRQ_MASK, fn);
    irq |= curIRQ;
    TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_IRQ_MASK, irq, fn);
}

// disables the irq
void TiMo_resetIRQ(unsigned char irq, timo_t *fn)
{
    unsigned char curIRQ = TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_IRQ_MASK, fn);
    curIRQ &= (~irq);
    TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_IRQ_MASK, curIRQ, fn);
}

unsigned char TiMo_getIRQStatus(timo_t *fn)
{
    return TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_IRQ_FLAGS, fn);
}

void TiMo_setExtIRQ(unsigned char irq, timo_t *fn)
{
    TiMo_setIRQ(TIMO_IRQM_EXTENDED_EN, fn);
    unsigned char curIRQ = TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_EXT_IRQ_MASK, fn);
    irq |= curIRQ;
    TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_IRQ_MASK, irq, fn);
}

void TiMo_resetExtIRQ(unsigned char irq, timo_t *fn)
{
    unsigned char curIRQ = TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_EXT_IRQ_MASK, fn);
    irq |= curIRQ;
    TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_IRQ_MASK, irq, fn);
    if (!curIRQ)
    { // diable in main irq mask
        TiMo_resetIRQ(TIMO_IRQM_EXTENDED_EN, fn);
    }
}

unsigned char TiMo_getExtIRQStatus(timo_t *fn)
{
    return TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_EXT_IRQ_FLAGS, fn);
}

unsigned char TiMo_getStatus(timo_t *fn)
{
    return TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_STATUS, fn);
}

// configure timo // use
void TiMo_setConfig(unsigned char config, timo_t *fn)
{
    TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_CONFIG, config, fn);
}

unsigned char TiMo_getConfig(timo_t *fn)
{
    return TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_CONFIG, fn);
}

// set timo in tx mode (if a) else set in rx mode
void TiMo_setModeTX(unsigned char a, timo_t *fn)
{
    unsigned char d = TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_CONFIG, fn);
    if (a)
    {
        d = (d | TIMO_CONFIG_RADIO_TX_RX_MODE);
        // TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_CONFIG, d);
    }
    else
    {
        d = d & ~(TIMO_CONFIG_RADIO_TX_RX_MODE);
        // TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_CONFIG, d);
    }
    TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_CONFIG, d, fn);
}

// 0xff sets no battery, 0-100DEC sets level in %
void TiMo_setBattery(unsigned char level, timo_t *fn)
{
    if ((level > 100) && (level < 0xff))
        level = 100;
    TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_BATTERY, level, fn);
}

void TiMo_setRFPower(unsigned char level, timo_t *fn)
{
    TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_RF_POWER, level, fn);
}

unsigned char TiMo_getRFPower(timo_t *fn)
{
    unsigned char d = TiMo_readReg8(TIMO_COMMAND_READ_REG | TIMO_REG_RF_POWER, fn);
    return d;
}

// enables or disables external antenna (0 sets internall antenna)
void TiMo_extAntenna(unsigned char a, timo_t *fn)
{
    if (a)
        a = 0x01;
    TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_ANTENNA, a, fn);
}

void TiMo_getRGB(timo_unitcol_t *p, timo_t *fn)
{
    TiMo_readRegP(TIMO_REG_UNIVERSE_COLOR, (unsigned char *)p, 3, fn);
}

void TiMo_setRGB(timo_unitcol_t *p, timo_t *fn)
{
    TiMo_writeRegP(TIMO_REG_UNIVERSE_COLOR | TIMO_COMMAND_WRITE_REG, (unsigned char *)p, 3, fn);
}

void TiMo_readRDM(unsigned char *d, unsigned char len, timo_t *fn)
{
    unsigned char t = 0;
    while (len)
    {
        if (len > 128)
        {
            TiMo_readRegP(TIMO_COMMAND_READ_RDM, d + (t * 128), 128, fn);
            len = len - 128;
            t++;
        }
        else
        {
            TiMo_readRegP(TIMO_COMMAND_READ_RDM, d + (t * 128), len, fn);
            len = 0;
        }
    }
}

void TiMo_writeRDM(unsigned char *d, unsigned char len, timo_t *fn)
{
    unsigned char t = 0;
    while (len)
    {
        if (len > 128)
        {
            TiMo_writeRegP(TIMO_COMMAND_READ_RDM, d + (t * 128), 128, fn);
            len = len - 128;
            t++;
        }
        else
        {
            TiMo_writeRegP(TIMO_COMMAND_READ_RDM, d + (t * 128), len, fn);
            len = 0;
        }
    }
}

void TiMo_setOEMInfo(unsigned short vid, unsigned short pid, timo_t *fn)
{
    unsigned char d[4];
    d[1] = (unsigned char)(vid & 0xff);
    d[0] = (unsigned char)((vid >> 8) & 0xff);
    d[3] = (unsigned char)(pid & 0xff);
    d[2] = (unsigned char)((pid >> 8) & 0xff);
    TiMo_writeRegP(TIMO_COMMAND_WRITE_REG | TIMO_REG_OEM_INFO, d, 4, fn);
}

// read dmx data
void TiMo_readDMX(unsigned char *d, unsigned short len, timo_t *fn)
{
    // unsigned char tmpdata[128];
    unsigned short count = 0;
    while (len)
    {
        if (len > 128)
        {
            TiMo_readRegP(TIMO_COMMAND_READ_DMX, (unsigned char *)(d + count), 128, fn);
            len = len - 128;

            count = count + 128;
        }
        else
        {
            TiMo_readRegP(TIMO_COMMAND_READ_DMX, (unsigned char *)(d + count), len, fn);
            len = 0;
        }
    }
}

void TiMo_writeDMX(unsigned char *d, unsigned short len, timo_t *fn)
{
    unsigned char t = 0;
    while (len)
    {
        if (len > 128)
        {
            TiMo_writeRegP(TIMO_COMMAND_WRITE_DMX, d + (t * 128), 128, fn);
            len = len - 128;
            t++;
        }
        else
        {
            TiMo_writeRegP(TIMO_COMMAND_WRITE_DMX, d + (t * 128), len, fn);
            len = 0;
        }
    }
}

void TiMo_setBLEStatus(unsigned char d, timo_t *fn)
{
    unsigned char cur = TiMo_readReg8(TIMO_REG_BLE_STATUS, fn);
    if (d)
        TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_BLE_STATUS, (cur | 0x01), fn);
    else
        TiMo_writeReg8(TIMO_COMMAND_WRITE_REG | TIMO_REG_BLE_STATUS, (cur & ~(0x01)), fn);
}

void TiMo_setBLEPin(unsigned char *p, unsigned char len, timo_t *fn)
{
    if (len > 6)
        len = 6; // just chop of last digits??
    TiMo_writeRegP(TIMO_COMMAND_WRITE_REG | TIMO_REG_BLE_PIN, p, len, fn);
}