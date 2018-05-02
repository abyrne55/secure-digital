#include "mbed.h"
#include "SWSPI.h"

Serial pc(USBTX, USBRX);
DigitalOut rled(LED_RED);
DigitalOut bled(LED_BLUE);

/*
DigitalOut toSDcs(D3);
D10 = PTD0 
D11 = PTD2 
D12 = PTD3 
D13 = PTD1 
D14 = PTE25 
D15 = PTE24 
*/
DigitalIn fromCamcs(PTD0);
SPISlave fromCam(PTD2, PTD3, PTD1, PTD0); // mosi, miso, sclk, ssel
SPI toSD(PTD6, PTD7, PTD5); // mosi, miso, sclk
DigitalOut toSDcs(PTD4);

int sendCMD0(){
    toSDcs.write(0);
    // 40 00 00 00 00 95
    int repl;
    repl = toSD.write(0x40);
    //pc.printf("CMD0: SD response was 0x%x\r\n", repl);
    repl = toSD.write(0x00);
    //pc.printf("CMD0: SD response was 0x%x\r\n", repl);
    repl = toSD.write(0x00);
    //pc.printf("CMD0: SD response was 0x%x\r\n", repl);
    repl = toSD.write(0x00);
    //pc.printf("CMD0: SD response was 0x%x\r\n", repl);
    repl = toSD.write(0x00);
    //pc.printf("CMD0: SD response was 0x%x\r\n", repl);
    repl = toSD.write(0x95);
    //pc.printf("CMD0: SD response was 0x%x\r\n", repl);
    
    int retry = 0;
    while ((repl = toSD.write(0xFF)) == 0xFF) {
        if (retry++ > 100) 
            break;
    }
    toSDcs.write(1);
    return repl;
}

int main() {
    pc.baud(115200);
    pc.printf("\r\n\nHello! Initial setup...\r\n");
    
    // Configure SPI Slave
    fromCam.frequency(125000);
    fromCam.format(8, 0);
    //fromCam.reply(0xFF);
    
    // DEBUG: STOP HERE AND JUST ECHO ALL SPI BYTES TO SERIAL
//    pc.printf("DEBUG: PRINTING ALL INCOMING SPI PACKETS\r\n");
//    int ct = 0;
//    while (1) {
//        fromCam.reply(ct);
//        while (!fromCam.receive());
//        int in = fromCam.read();
//        pc.printf("Rec'vd 0x%x, sent 0x%x \r\n", in, ct);
//        ct++;
//        if (ct % 10 == 0) pc.printf(".");
//    }
    
    // Configure SPI Master
    toSD.frequency(400000);
    toSD.format(8, 0);
    
    // Putting SD in SPI mode
    pc.printf("Initializing card (74 cycle init) into native mode...\r\n");
    wait(1);
    int sdReply;
    toSDcs.write(1); // Keep CS and MOSI high for 74 cycles
    for (int i = 0; i < 16; i++) {
    //while (1) {
        sdReply = toSD.write(0xFF);
        pc.printf("SD response was 0x%x\r\n", sdReply);
        //delay(50);
    }
    pc.printf("Card in native mode. Last response was 0x%x\r\n", sdReply);
    
    // With CS low, send CMD0 (triggers software reset)
    pc.printf("Commanding card to reset into SPI mode (CMD0)...\r\n");
    int repl = sendCMD0();
    while(repl != 0x01) {
        pc.printf("Initialization failed, trying again...\r\n");
        wait(3);
        repl = sendCMD0();
    }
    pc.printf("Card in SPI mode. Last response was 0x%x. Increasing speed to 20MHz\r\n", repl);
    toSD.frequency(20000000);
    
    // Wait while the camera tries to put *us* into SPI mode
    pc.printf("Waiting for camera to command us to SPI mode...\r\n");
    int camStatus = 0;
    
    while (camStatus != 0x95) { 
        fromCam.reply(0xFF);
        while (!fromCam.receive());
        camStatus = fromCam.read();
        pc.printf("%x ", camStatus);
    }
    pc.printf("\r\nCamera finished command with 0x%x. Next reply will be IDLE\r\n", camStatus);
    while (camStatus == 0xff){
        fromCam.reply(0x01);
        while (!fromCam.receive());
        camStatus = fromCam.read();
        pc.printf("%x ", camStatus);
    }
    pc.printf("IDLE reply sent while rec'v 0x%x.\r\n\r\n", camStatus);
    // Received first bit of CMD8, so forward that and let forwarder handle the rest
    //toSDcs.write(0);
    //toSD.write(0x48);
    //fromCam.reply(0xFF);
    
    
    // Main Forwarding Loop
    bled = 0; // Status Light
    fromCam.reply(0xFF);
    while (!fromCam.receive());
    camStatus = fromCam.read();
    pc.printf("rec'v 0x%x. Starting forwarder...\r\n\r\n", camStatus);
    
    while(1) {
        if (fromCam.receive()) {
            // Copy value of CS pin
            toSDcs = fromCamcs;
            //rled = !rled;
            int msg = fromCam.read(); 
            //if (msg != 0xff) pc.printf("C%x ", msg);
            int reply = toSD.write(msg);
            toSDcs = fromCamcs;
            fromCam.reply(reply);
            //pc.printf("S%x ", reply);
            
            toSDcs = fromCamcs;
            
            //rled = !rled;
        }
    }
}