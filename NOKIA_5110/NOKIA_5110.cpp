// Project: Nokia5110 - Controlling a NK5110 display from an NXP LPC1768
// File: NOKIA_5110.cpp
// Author: Chris Yan
// Created: January, 2012
// Revised: January, 2014
//  Desc: Supporting code for the NokiaLcd class

#include "NOKIA_5110.h"
#include "mbed.h"

NokiaLcd::NokiaLcd(LcdPins pinout)
{
    // SPI
    LcdSpi = new SPI(pinout.mosi, pinout.miso, pinout.sclk);
    LcdSpi->format(LCD_SPI_BITS, LCD_SPI_MODE);
    LcdSpi->frequency(LCD_FREQ);

    // Control Pins
    Pins = new DigitalOut*[3];
    Pins[PIN_RST]   = new DigitalOut(pinout.rst);
    Pins[PIN_SCE]   = new DigitalOut(pinout.sce);
    Pins[PIN_DC]    = new DigitalOut(pinout.dc);

    // Initial Command Instructions, note the initial command mode
    FunctionSet.V   = CMD_FS_HORIZONTAL_MODE;
    FunctionSet.H   = CMD_FS_EXTENDED_MODE;
    FunctionSet.PD  = CMD_FS_ACTIVE_MODE;
    FunctionChar    = CreateFunctionChar();

    TempControlChar = CMD_TC_TEMP_2;
    DispControlChar = CMD_DC_NORMAL_MODE;
    BiasChar        = CMD_BI_MUX_48;
    VopChar         = CMD_VOP_7V38;
}

void NokiaLcd::ShutdownLcd()
{
    FunctionSet.PD  = CMD_FS_POWER_DOWN_MODE;

    ClearLcdMem();
    SendFunction( CMD_DC_CLEAR_DISPLAY );
    SendFunction( CreateFunctionChar() );
}

void NokiaLcd::ClearLcdMem()
{
    for(int tick = 0; tick <= 503; tick++)
        LcdSpi->write(0x00);
}

// added for eee212 project
void NokiaLcd::TestLcd2(char test_pattern)
{
    for(int tick = 0; tick <= 1; tick++)
        LcdSpi->write(test_pattern);         // Command gets sent

}

void NokiaLcd::TestLcd(char test_pattern)
{
    for(int tick = 0; tick <= 504; tick++)
        LcdSpi->write(test_pattern);         // Command gets sent
}

void NokiaLcd::InitLcd()
{
    ResetLcd();
    Pins[PIN_SCE]->write(0);     // Chip Select goes low

    // Redefine the FunctionChar in case it has changed
    FunctionSet.V   = CMD_FS_HORIZONTAL_MODE;
    FunctionSet.H   = CMD_FS_EXTENDED_MODE;
    FunctionSet.PD  = CMD_FS_ACTIVE_MODE;
    SendFunction( CreateFunctionChar() );   // Extended CMD set
    SendFunction( VopChar );                // | Vop
    SendFunction( TempControlChar );        // | Temp
    SendFunction( BiasChar );               // | Bias

    FunctionSet.H   = CMD_FS_BASIC_MODE;
    SendFunction( CreateFunctionChar() );   // Basic CMD set
    SendFunction( DispControlChar );        // | Display Mode

    ClearLcdMem();
    Pins[PIN_DC]->write(1);     // Data/CMD goes back to Data mode
}

void NokiaLcd::ResetLcd()
{
    Pins[PIN_RST]->write(0);    // Reset goes low
    Pins[PIN_RST]->write(1);    // Reset goes high
}

char NokiaLcd::CreateFunctionChar()
{
    return ( 0x20 | FunctionSet.PD | FunctionSet.V | FunctionSet.H );
}

void NokiaLcd::SendDrawData(char data)
{
    LcdSpi->write(data);         // Command gets sent
}

void NokiaLcd::DrawChar(char character)
{
    for( int i = 0; i < 6; i++)
        SendDrawData( FONT_6x6[ ((character - 32)*6) + i] );
}

void NokiaLcd::DrawString(char* s)
{
    char len = strlen(s);
    for( int idx = 0; idx < len; idx++ ) {
        for( int i = 0; i < 3; i++)
            SendDrawData( FONT_6x6[ ((s[idx] - 32)*3) + i] );
    }
}

void NokiaLcd::DrawString2(char* s)
{
    char len = strlen(s);
    for( int idx = 0; idx < len; idx++ )
    {
        for( int i = 0; i < 6; i++)
            SendDrawData( FONTS[ ((s[idx] - 32)*6) + i] );
    }
}

void NokiaLcd::DrawFrameChar(char character)
{
    for( int i = 0; i < 6; i++)
        SendDrawData((( FONT_6x6[ ((character - 32)*6) + i]  ) << 1 ) | 0x81);
}

void NokiaLcd::DrawNegFrameChar(char character)
{
    for( int i = 0; i < 6; i++)
        SendDrawData(~(( FONT_6x6[ ((character - 32)*6) + i]  ) << 1 ) | 0x81);
}

char* NokiaLcd::NumToStr(int num)
{
    if(num <= 0)
        return "0";

    double length = 0;
    int tlen = 0;
    int temp = 1;
    char c;

    // Get number of digits
    while( temp <= num ) {
        temp *= 10;
        length++;
    }
    tlen = length;
    char* numString = new char[tlen+1];

    // Convert each place in number to a stand-alone representative number
    temp = 0;
    for(int idx = pow(10, length); idx>1; idx = (idx/10)) {
        c = (char)( ((num % idx)-(num % (idx/10)))/(idx/10) + 48);
        numString[temp] = c;
        temp++;
    }
    numString[temp] = '\0';
    return numString;
}

void NokiaLcd::SetXY(char x, char y)
{
    if( (x > 83) || (y > 5) )
        return;

    SendFunction( x | 0x80 );
    SendFunction( y | 0x40 );
}

void NokiaLcd::print_score(int s1, int s2, bool mode, int remaining, bool tmode)
{
//this function gets the scores and prints the corresponding data to screen
//mode = 0 -> 2 player mode, mode = 1 -> AI mode
    ClearLcdMem();
    SetXY(9,2);
    DrawString2("P1 ");
    SetXY(27,2);
    char* s11 ;
    s11 = NumToStr(s1); //try this if "0" + s1 doesn't work
    DrawString2(s11);
    SetXY(33,2);
    DrawString2(" - ");
    SetXY(51,2);
    char* s22 = NumToStr(s2); //try this if "0" + s1 doesn't work
    DrawString2(s22);
    SetXY(57,2);
    if(!(mode)) //2 player mode
        DrawString2(" P2");
    if(mode)  // AI mode
        DrawString2(" AI");
        
    if(tmode){
        SetXY(3,4);
        DrawString2("REMAINING: ");
        char* rem = NumToStr(remaining); //try this if "0" + s1 doesn't work
        SetXY(69,4);
        DrawString2(rem);
    }    
}

void NokiaLcd::print_win(int player, bool mode)
{
//this function gets the scores and prints the corresponding data to screen
//mode = 0 -> 2 player mode, mode = 1 -> AI mode
    ClearLcdMem();
    SetXY(24,3);
    if(!(mode)) {
        DrawString2("P");
        SetXY(30,3);
        if(player == 1)
            DrawString2("1");
        if(player == 2)
            DrawString("2");
        SetXY(36,3);
        DrawString2(" WON");
    }
    if(mode && (player == 2)){
        DrawString2("AI WON");
    }
    if(mode && (player == 1)){
        DrawString2("P1 WON");
    }
    if(player == 3){
        DrawString2(" DRAW ");
    }
}

void NokiaLcd::drawRakets(int r1, int r2,int b1,int b2) //b1 = location of raket-1, b2 = raket-2
{
    //Draw raket-1
    if (b1%2 == 0) {
        SetXY(r1, b1/2);
        DrawString("|");
    } else {
        //z draws upper half, y draws lower half
        SetXY(r1, b1/2); //b1/2 -> upper half location
        DrawString("z");
        SetXY(r1, b1/2 + 1); //b1/2 + 1 -> lower half location
        DrawString("y");
    }

    //Draw raket-2
    if (b2%2 == 0) {
        SetXY(r2, b2/2);
        DrawString("|");
    } else {
        //z draws upper half, y draws lower half
        SetXY(r2, b2/2); //b2/2 -> upper half location
        DrawString("z");
        SetXY(r2, b2/2 + 1); //b2/2 + 1 -> lower half location
        DrawString("y");
    }
}

void NokiaLcd::drawBall(int bx, int by) //
{
    //Ball is 2x2, therefore 7 possible drawings in one block,
//additionally 8th drawing that uses 2 blocks, a,b,c,d,e,f,g - 7 drawings
//h,i => h = lower part of upper block, i = upper part of lower block
    //bx is x location (ez)
    //y location (mode 8) = 0-6 => a-g, 7 => h,i
    //ascii of a = 97 -> y location (mode 8) + 97 - send string
    if ((by%8) == 0) {
        SetXY(bx, by/8);
        DrawString("g");
    } else if((by%8) == 1) {
        SetXY(bx, by/8);
        DrawString("f");
    } else if((by%8) == 2) {
        SetXY(bx, by/8);
        DrawString("e");
    } else if((by%8) == 3) {
        SetXY(bx, by/8);
        DrawString("d");
    } else if((by%8) == 4) {
        SetXY(bx, by/8);
        DrawString("c");
    } else if((by%8) == 5) {
        SetXY(bx, by/8);
        DrawString("b");
    } else if((by%8) == 6) {
        SetXY(bx, by/8);
        DrawString("a");
    } else { // by%8 == 7
        SetXY(bx, by/8); //to draw lower part of upper block
        DrawString("i");
        SetXY(bx, by/8 + 1); //to draw upper part of lower block
        DrawString("h");
    }
}


void NokiaLcd::SendFunction(char cmd) //TODO:Detection of what H should be
{
    Pins[PIN_DC]->write(0);     // Data/CMD goes low
    LcdSpi->write(cmd);         // Command gets sent
    Pins[PIN_DC]->write(1);     // Data/CMD goes back to Data mode
}

NokiaLcd::~NokiaLcd()
{
    ShutdownLcd();
}
