
#ifndef timo_h
#define timo_h
#endif

#define TIMO_COMMAND_READ_REG 0x00
#define TIMO_COMMAND_WRITE_REG 0x40
#define TIMO_COMMAND_READ_DMX 0x81
#define TIMO_COMMAND_READ_ASC 0x82
#define TIMO_COMMAND_READ_RDM 0x83
#define TIMO_COMMAND_READ_RXTX 0x84

#define TIMO_COMMAND_WRITE_DMX 0x91
#define TIMO_COMMAND_WRITE_RDM 0x92
#define TIMO_COMMAND_WRITE_RXTX 0x94


#define TIMO_COMMAND_NOP 0xFF

// registers
#define TIMO_REG_CONFIG 0x00
#define TIMO_REG_STATUS 0x01
#define TIMO_REG_IRQ_MASK 0x02
#define TIMO_REG_IRQ_FLAGS 0x03
#define TIMO_REG_DMX_WINDOW 0x04
#define TIMO_REG_ASC_FRAME 0x05
#define TIMO_REG_LINK_QUALITY 0x06
#define TIMO_REG_ANTENNA 0x07
#define TIMO_REG_DMX_SPEC 0x08
#define TIMO_REG_DMX_CONTROL 0x09
#define TIMO_REG_EXT_IRQ_MASK 0x0A
#define TIMO_REG_EXT_IRQ_FLAGS 0x0B
#define TIMO_REG_VERSION 0x10
#define TIMO_REG_RF_POWER 0x11
#define TIMO_REG_BLOCKED_CHANNELS 0x12
#define TIMO_REG_BINDING_UID 0x20
#define TIMO_REG_BLE_STATUS 0x30
#define TIMO_REG_BLE_PIN 0x31
#define TIMO_REG_BATTERY 0x32
#define TIMO_REG_UNIVERSE_COLOR 0x33
#define TIMO_REG_OEM_INFO 0x34

// confg register
#define TIMO_CONFIG_UART_EN 0x01          // Enable UART output of DMX frames (required for RDM). 0 = Disabled, 1 = Enabled
#define TIMO_CONFIG_RADIO_TX_RX_MODE 0x02 // 0 = Receiver, 1 = Transmitter
#define TIMO_CONFIG_SPI_RDM_EN 0x04       // 0 = UART RDM is used, 1 = SPI RDM is used
#define TIMO_CONFIG_RADIO_EN 0x80         // Enable wireless operation. 0 = Enabled, 1 = Disabled

// status register
#define TIMO_STATUS_LINKED 0x01   // 0 = Not linked, 1 = Linked to TX (or pairing) Write 1 to unlink
#define TIMO_STATUS_RFLINK 0x02   // 0 = No radio link, 1 = Active radio link On transmitter, write 1 to start linking
#define TIMO_STATUS_IDENTIFY 0x04 // 0 = No identify, 1 = RDM identify active
#define TIMO_STATUS_DMX 0x08      // 0 = No DMX available, 1 = DMX available
#define TIMO_STATUS_UPDATE 0x80   // 0 = chip operational, 1 = In driver update mode

// irq mask reg
#define TIMO_IRQM_DMX_EN 0x01         // Enable DMX frame reception interrup
#define TIMO_IRQM_LOST_DMX_EN 0x02    // Enable loss of DMX interrupt
#define TIMO_IRQM_DMX_CHANGED_EN 0x04 // Enable DMX changed interrupt
#define TIMO_IRQM_RF_LINK_EN 0x08     // Enable radio link status change interrupt
#define TIMO_IRQM_ASC_EN 0x10         // Enable alternative start code interrupt
#define TIMO_IRQM_IDENTIFY_EN 0x20    // Enable identify device interrupt
#define TIMO_IRQM_EXTENDED_EN 0x40    // Enable extended interrupts

// irq flags
#define TIMO_IRQF_DMX_RX 0x01      // Complete DMX frame received interrupt
#define TIMO_IRQF_DMX_LOST 0x02    // Loss of DMX interrupt
#define TIMO_IRQF_DMX_CHANGED 0x04 // DMX changed in DMX window interrupt
#define TIMO_IRQF_RF_LINK 0x08     // Radio link status change interrupt
#define TIMO_IRQF_ASC 0x10         // Alternative start code frame received interrupt
#define TIMO_IRQF_IDENTIFY 0x20    // Identify device state change interrupt
#define TIMO_IRQF_EXTENDED 0x40    // Extended interrupt
#define TIMO_IRQF_SPI_BUSY 0x80    // SPI slave device is busy and cannot comply with command. Command sequence MUST be restarted.

// ext irq mask
#define TIMO_EXTIRQM_RDM_REQUEST_EN 0x01
#define TIMO_EXTIRQM_RXTX_DA_EN 0x02
#define TIMO_EXTIRQM_RXTX_CTS_EN 0x04
#define TIMO_EXTIRQM_UNIV_META_CHANGED_EN 0x20

// ext irq flags
#define TIMO_EXTIRQF_RDM_REQUEST 0x01
#define TIMO_EXTIRQF_RXTX_DA 0x02
#define TIMO_EXTIRQF_RXTX_CTS 0x04
#define TIMO_EXTIRQF_UNIV_META_CHANGED 0x20

// types
typedef struct timo_t
{
    void (*cs_h)();
    void (*cs_l)();
    void (*waitUs)(unsigned long long);
    unsigned long (*irq_pinCl)();
    unsigned char (*spi_send_rec)(unsigned char data);
} timo_t;

typedef struct __attribute__((__packed__, aligned(1))) timo_unitcol_t
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} timo_unitcol_t;

// functions
void TiMo_setDMXwindow(unsigned short window, unsigned short address, timo_t *fn); // set dmx "window" to listen to

void TiMo_setIRQ(unsigned char irq, timo_t *fn);   // enables irqs (irq mask reg)
void TiMo_resetIRQ(unsigned char irq, timo_t *fn); // disables irq (irq mask reg)

unsigned char TiMo_getIRQStatus(timo_t *fn); // return which irq flags are set

void TiMo_setExtIRQ(unsigned char irq, timo_t *fn);   // enables irqs (irq ext mask reg)
void TiMo_resetExtIRQ(unsigned char irq, timo_t *fn); // disables irq (irq ext mask reg)

unsigned char TiMo_getExtIRQStatus(timo_t *fn); // return which ext irq flags are set ()

void TiMo_readDMX(unsigned char *d, unsigned short len, timo_t *fn);  // read dmx package
void TiMo_writeDMX(unsigned char *d, unsigned short len, timo_t *fn); // writes dmx package

void TiMo_readRDM(unsigned char *d, unsigned char len, timo_t *fn);
void TiMo_writeRDM(unsigned char *d, unsigned char len, timo_t *fn);

void TiMo_setConfig(unsigned char config, timo_t *fn);
unsigned char TiMo_getConfig(timo_t *fn);

void TiMo_getRGB(timo_unitcol_t *p, timo_t *fn); // get universe color
void TiMo_setRGB(timo_unitcol_t *p, timo_t *fn); // set universe color

void TiMo_setBLEStatus(unsigned char d, timo_t *fn); // sets status of bluetooth

void TiMo_setBLEPin(unsigned char *p, unsigned char len, timo_t *fn); // set bluetooth connection PIN (unknow format, ascii?)

void TiMo_extAntenna(unsigned char a, timo_t *fn); // not avail in TimoTwo

void TiMo_setModeTX(unsigned char a, timo_t *fn);

void TiMo_setBattery(unsigned char level, timo_t *fn);
void TiMo_setRFPower(unsigned char level, timo_t *fn);

unsigned char TiMo_getStatus(timo_t *fn);

unsigned char TiMo_getRFPower(timo_t *fn);

void TiMo_setOEMInfo(unsigned short vid, unsigned short pid, timo_t *fn);

unsigned char TiMo_readRegP(unsigned char cmd, unsigned char *data, unsigned char len, timo_t *fn);
unsigned char TiMo_writeRegP(unsigned char cmd, unsigned char *data, unsigned char len, timo_t *fn);
unsigned char TiMo_readReg8(unsigned char cmd, timo_t *fn);
void TiMo_writeReg8(unsigned char cmd, unsigned char data, timo_t *fn);
