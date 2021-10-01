/*
||
|| @file Keypad.h
|| @version 3.1
|| @author Mark Stanley, Alexander Brevig
|| @contact mstanley@technologist.com, alexanderbrevig@gmail.com
||
|| @description
|| | This library provides a simple interface for using matrix
|| | keypads. It supports multiple keypresses while maintaining
|| | backwards compatibility with the old single key library.
|| | It also supports user selectable pins and definable keymaps.
|| #
||
|| @license
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, write to the Free Software
|| | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
|| #
||
*/
/*
 * LeonH: changed from C++ to C
 */

#ifndef KEYPAD_H
#define KEYPAD_H


#define LOW  0
#define HIGH  1
#define OPEN LOW
#define CLOSED HIGH

typedef char KeypadEvent;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char byte;

#define false 0
#define true  1
typedef enum{ IDLE, PRESSED, HOLD, RELEASED } KeyState;
    
#define bitRead(value,bit) (((value)>>(bit))&0x01)
#define bitSet(value,bit) ((value)|=(1UL<<(bit)))
#define bitClear(value,bit) ((value)&=~(1UL<<(bit)))
#define bitWrite(value,bit,bitvalue) (bitvalue ? bitSet(value,bit) : bitClear(value,bit))

// Made changes according to this post http://arduino.cc/forum/index.php?topic=58337.0
// by Nick Gammon. Thanks for the input Nick. It actually saved 78 bytes for me. :)
typedef struct {
    byte rows;
    byte columns;
} KeypadSize;

typedef struct {
    char kchar;
    int kcode;
    KeyState kstate;
    int stateChanged;
} Key;

#define LIST_MAX 12        // Max number of keys on the active list.
#define MAPSIZE 10        // MAPSIZE is the number of rows (times 16 columns)
#define makeKeymap(x) ((char*)x)

#define NO_KEY  '\0'

    //void keypad_init(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols);
    void keypad_init(int8_t onoff);

    //virtual void pin_mode(byte pinNum, byte mode) { pinMode(pinNum, mode); }
    //virtual void pin_write(byte pinNum, bool level) { digitalWrite(pinNum, level); }
    //virtual int  pin_read(byte pinNum) { return digitalRead(pinNum); }

    uint bitMap[MAPSIZE];    // 10 row x 16 column array of bits. Except Due which has 32 columns.
    Key key[LIST_MAX];
    unsigned long holdTimer;

    char keypad_getKey();
    int keypad_getKeys();
    KeyState keypad_getState();
    int keypad_isPressed(char keyChar);
    void keypad_setDebounceTime(uint);
    void keypad_setHoldTime(uint);
    //void keypad_addEventListener(void (*listener)(char));
    int keypad_findInList_char(char keyChar);
    int keypad_findInList_int(int keyCode);
    char keypad_waitForKey();
    int keypad_keyStateChanged();
    byte keypad_numKeys();

    void keypad_scanKeys();
    int keypad_updateList();
    void keypad_nextKeyState(byte n, int button);
    void keypad_transitionTo(byte n, KeyState nextState);
//    void (*keypadEventListener)(char);


#endif
