#include "mbed.h"
#include "NOKIA_5110.h"
#include "rtos.h"
LcdPins myLcdPins = { PTC6, NC, PTC5, PTD2, PTD0, PTD3 };
NokiaLcd myLcd( myLcdPins );    // SPI is started here (8-bits, mode 1)

LcdPins myLcdPins2 = { PTD6, NC, PTD5, PTE3, PTE5, PTE4  };
NokiaLcd score_lcd( myLcdPins2 );    // SPI is started here (8-bits, mode 1)
int b1 = 6; //From 0 to 10, total of 11 lcoations
int b2 = 6; //6 blocks of lcd + 5 mid-locations
const int r1 = 6; //x location of raket 1
const int r2 = 78; //x location of raket 2
bool xdir = false; //x direction of ball (true -> right , false -> left)
bool ydir = false; //y direction of ball (true -> up , false -> down)
int bx = 42;
int by = 24;     //initial location of the ball
//MAKES MENU FOR THEM ALL
bool gamemode = false; //false = score mode, time = time mode
int timelimit = 3; //score mode->#scores to win[1-5],time mode->#seconds[15-60]
int score_limit = 2; //this is set by user from the menus
int AImode = false; //false-> 2player mode, true-> AI mode
int difficulty = 1; //2p -> speed of ball, AI -> speed of AI    [1-3] for both
int waittime = 75; //difficulty depended, 1-75, 2-50, 3-25
//always make ball move 1 pixel,change the waiting time, otherwise might not work
//MAKES MENU FOR THEM ALL
bool gameover = false;
int p1score = 0;
int p2score = 0;
bool gamepaused = false;            //BUNU YAPALIM MI????????
int remaining = 0;

AnalogIn p1x(A0); //P1's joystick (one direction is enough)
AnalogIn p2x(A1); //P2's joystick (one direction is enough)
DigitalIn button(PTB8); //Only P1's button is enough for choosing (master button)
int x1;
int x2;
int var;

PwmOut mypwm(PTB3); // this pin is used for music output

void play()
{
    mypwm.period_us(1136.4f);
    mypwm.pulsewidth_us(568.2f);
    Thread::wait(20);
    mypwm.write(0.0);
}

void play2()
{
    mypwm.period_us(1431.6f);
    mypwm.pulsewidth_us(715.8f);
    Thread::wait(500);
    mypwm.write(0.0);
}

void play3()
{
    mypwm.period_us(1910.9f);
    mypwm.pulsewidth_us(955.5f);
    Thread::wait(500);
    mypwm.write(0.0);
    Thread::wait(50);
    mypwm.period_us(1431.6f);
    mypwm.pulsewidth_us(715.8f);
    Thread::wait(500);
    mypwm.write(0.0);
    Thread::wait(50);
    mypwm.period_us(2024.7f);
    mypwm.pulsewidth_us(1012.4f);
    Thread::wait(500);
    mypwm.write(0.0);
}


void aimove()
{
    var = 3*bx + 7*by;
    if (xdir) {
        if(difficulty == 1) {
            if ((var%5) == 1) {
                if (!ydir)
                    b2--;
                else
                    b2++;
            } else {
                if(!ydir)
                    b2++;
                else
                    b2--;
                if (b2 < 0)
                    b2 = 0;
                if (b2 > 10)
                    b2 = 10;
            }
        } else if(difficulty==2) {
            if ((var%5) == 1 || (var%5) == 2) {
                if (!ydir)
                    b2--;
                else
                    b2++;
            } else {
                if(!ydir)
                    b2++;
                else
                    b2--;
                if (b2 < 0)
                    b2 = 0;
                if (b2 > 10)
                    b2 = 10;
            }
        } else if(difficulty==3) {

            if ((bx%7) == 1) {
                if ((bx < 50)&&(ydir)) {
                    b2 = (78-by-bx)/4 -1;
                }
                if ((bx < 50)&&(!(ydir))) {
                    b2 = (18+bx-by)/4 - 1;
                }
            }
            if (b2 < 0)
                b2 = 0;
            if (b2 > 10)
                b2 = 10;
        }
    } else
        b2 = 6;
}

void timedone()
{
    if (p1score > p2score) { //if score game mode
        score_lcd.print_win(1, AImode);
    }
    if (p1score < p2score) { //if score game mode
        score_lcd.print_win(2, AImode);
    }
    if(p1score == p2score)
        score_lcd.print_win(2, AImode);
    Thread::wait(2000);
    NVIC_SystemReset();

}

void restartgame(bool over)
{
//over = 1 -> game over, 0 -> start from middle
    if (over) {
        play3();
        Thread::wait(2000);
        NVIC_SystemReset();
    } else {
        xdir = !(xdir);
        ydir = !(ydir);
        bx = 42;
        by = 24;
        b1 = 6;
        b2 = 6;
    }
}


void changescore(int p)
{
    if (p == 1)
        p1score++;
    if (p == 2)
        p2score++;

    score_lcd.print_score(p1score, p2score, AImode,  remaining, gamemode);

    if ((!(gamemode))&&(p1score == score_limit)) { //if score game mode
        score_lcd.print_win(1, AImode);
        gameover = 1;
    }
    if ((!(gamemode))&&(p2score == score_limit)) { //if score game mode
        score_lcd.print_win(2, AImode);
        gameover = 1;
    }
    restartgame(gameover);
}

//call this at each 10 * waittime
void joystick_int()
{
    x1 = p1x.read() * 1000;// 0-1000 arası
    x2 = p2x.read() * 1000; // 0-1000 arası
}

void moverakets()  //call every 0.5 seconds ??????????
{
    //joystick click için her seferinde interrupts çağırmaya gerek yok
    joystick_int(); //to get x1 & x2 data

    if (x1 < 100)
        b1--;
    if (x1 > 900)
        b1++;

    if(!(AImode)) {
        if (x2 < 100)
            b2--;
        if (x2 > 900)
            b2++;
    } else
        aimove();

    if (b1 < 0)
        b1 = 0;
    if (b1 > 10)
        b1 = 10;

    if (b2 < 0)
        b2 = 0;
    if (b2 > 10)
        b2 = 10;
}

void moveBall()
{
//this function generates the next location of the ball, then calls drawBall
    if (xdir) {
        bx++;
    } else {
        bx--;
    }

    if (ydir) {
        by--; //ydir = true -> goes up
    } else {
        by++;
    }
}

void changeBallMove()
{
//bx, by ->ball locations,b1,r1,b2,r2->raket locations,dir->ball directions
//this function changes direction of the ball when necessary
//bx,by are top-left of the ball!!! (might be useful)
    //(b1/2 = i)=> starts from 4*i, ends at 4*i + 8 !!!!
//CHECK X DIRECTION OF THE BALL
    if (bx == r1 + 2) { //just right of the raket-1
        if ((b1*4 <= by)&&(by <= b1*4 + 8)) { //raket-1 hits the ball{
            xdir = true; //changes direction to right
            play();
        }
    } else if (bx == r2 - 1) { //just left of the raket-2
        if ((b2*4 <= by)&&(by <= b2*4 + 8)) { //raket-2 hits the ball
            xdir = false; //changes direction to left
            play();
        }
    } else if ((bx == 0)||(bx == 1)) { //to be safe
        //xdir = true; //for infinite motion try, remove this
        changescore(2);
        play2();
    } else if ((bx == 82)||(bx == 83)) { //to be safe
        //xdir = false; //for infinite motion try
        changescore(1);
        play2();
    }

//CHECK Y DIRECTION OF THE BALL
    if(by == 0) {
        ydir = false;
        play();
    } else if (by == 47) {
        ydir = true;
        play();
    }

    moveBall();
}


void printmenu()
{
    score_lcd.SetXY(12,2);
    score_lcd.DrawString2("WELCOME TO");
    score_lcd.SetXY(6,4);
    score_lcd.DrawString2("AI PONG GAME");
    Thread::wait(2000);
//selections are made by master joystick(p1), approved by click of joystick
    //First menu = game mode (score or time)
    int i = 0;
    while(button) {// as long as button = 0 (not pressed)
        joystick_int(); // read data, only x1 will be used
        if (x1 < 100)
            i++;
        if (x1 > 900)
            i++;
        score_lcd.ClearLcdMem();
        score_lcd.SetXY(15,2);
        score_lcd.DrawString2("GAME MODE");
        if ((i%2) == 0) {
            score_lcd.SetXY(0,4);
            score_lcd.DrawString2(">SCORE    TIME");
            gamemode = false;
        }
        if ((i%2) == 1) {
            score_lcd.SetXY(0,4);
            score_lcd.DrawString2(" SCORE   >TIME");
            gamemode = true;
        }
        Thread::wait(500); //wait for 0.5 second
    }
    Thread::wait(1000);
    score_lcd.ClearLcdMem();
//Second menu = score/time
    int t = 0;
    while(button) {// as long as button = 0 (not pressed)
        joystick_int(); // read data, only x1 will be used
        if (x1 < 100)
            t++;
        if (x1 > 900)
            t++;
        score_lcd.ClearLcdMem();
        if(!(gamemode)) {
            score_lcd.SetXY(9,2);
            score_lcd.DrawString2("SCORE LIMIT");
            if ((t%3) == 0) {
                score_lcd.SetXY(9,4);
                score_lcd.DrawString2(" >2   3   5");
                score_limit = 2;
            }
            if ((t%3) == 1) {
                score_lcd.SetXY(9,4);
                score_lcd.DrawString2("  2  >3   5");
                score_limit = 3;
            }
            if ((t%3) == 2) {
                score_lcd.SetXY(9,4);
                score_lcd.DrawString2("  2   3  >5");
                score_limit = 5;
            }
        } else {
            score_lcd.SetXY(3,2);
            score_lcd.DrawString2("GAME DURATION");
            if ((t%3) == 0) {
                score_lcd.SetXY(9,4);
                score_lcd.DrawString2(">10  15  30");
                timelimit = 10;
            }
            if ((t%3) == 1) {
                score_lcd.SetXY(9,4);
                score_lcd.DrawString2(" 10 >15  30");
                timelimit = 15;
            }
            if ((t%3) == 2) {
                score_lcd.SetXY(9,4);
                score_lcd.DrawString2(" 10  15 >30");
                timelimit = 30;
            }
            Thread::wait(500); //wait for 0.5 second
        }
    }
    Thread::wait(1000);
    score_lcd.ClearLcdMem();

//Third Menu = game type (2P or 1P(AI opponent))
    int k = 0;
    while(button) {// as long as button = 0 (not pressed)
        joystick_int(); // read data, only x1 will be used
        if (x1 < 100)
            k++;
        if (x1 > 900)
            k++;
        score_lcd.ClearLcdMem();
        score_lcd.SetXY(15,2);
        score_lcd.DrawString2("GAME TYPE");
        if ((k%2) == 0) {
            score_lcd.SetXY(18,4);
            score_lcd.DrawString2("2-PLAYER");
            AImode = false;
        }
        if ((k%2) == 1) {
            score_lcd.SetXY(18,4);
            score_lcd.DrawString2("1-PLAYER");
            AImode = true;
        }
        Thread::wait(500); //wait for 0.5 second
    }
    Thread::wait(1000);
//Fourth Menu = difficulty

    int uo = 0;
    while(button) {// as long as button = 0 (not pressed)
        joystick_int(); // read data, only x1 will be used
        if (x1 < 100)
            uo++;
        if (x1 > 900)
            uo++;
        score_lcd.ClearLcdMem();
        score_lcd.SetXY(12,2);
        score_lcd.DrawString2("DIFFICULTY");
        if ((uo%3) == 0) {
            score_lcd.SetXY(9,4);
            score_lcd.DrawString2(" >1   2   3");
            difficulty = 1;
            waittime = 75;
        }
        if ((uo%3) == 1) {
            score_lcd.SetXY(9,4);
            score_lcd.DrawString2("  1  >2   3");
            difficulty = 2;
            waittime = 60;
        }
        if ((uo%3) == 2 ) {
            score_lcd.SetXY(9,4);
            score_lcd.DrawString2("  1   2  >3");
            difficulty = 3;
            waittime = 50;
        }
        Thread::wait(500); //wait for 0.5 second
    }
    Thread::wait(1000);
    score_lcd.ClearLcdMem();
    score_lcd.print_score(0, 0, AImode, remaining, gamemode);
}

int main()
{
    myLcd.InitLcd();
    score_lcd.InitLcd();
    printmenu();
    remaining = timelimit;

    if(!(gamemode)) { //score mode, 2 player mode
        int timeconst = waittime;
        while(1) {
            for (int i = 0; i < 5; i++) {
                myLcd.drawRakets(r1,r2,b1,b2); //2 random raket generated
                myLcd.drawBall(bx, by); //initial values = 42,24 (center of screen)
                Thread::wait(timeconst);
                changeBallMove();
                myLcd.ClearLcdMem();
            }
            moverakets();
        }
    } else {
        score_limit = 10;
        int timeconst = waittime;
        for (int tim = 0; tim < timelimit; tim++) {
            int temp = 200/(timeconst);
            for (int abc = 0; abc < temp ; abc++) {
                for (int i = 0; i < 5; i++) {
                    myLcd.drawRakets(r1,r2,b1,b2); //2 random raket generated
                    myLcd.drawBall(bx, by); //initial values = 42,24 (center of screen)
                    Thread::wait(timeconst);
                    changeBallMove();
                    myLcd.ClearLcdMem();
                }
                moverakets();
            }
            remaining--;
            score_lcd.print_score(p1score, p2score, AImode, remaining, gamemode);
        }
        timedone();
    }
}