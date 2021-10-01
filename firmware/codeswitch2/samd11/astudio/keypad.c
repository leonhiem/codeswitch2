/*
||
|| @file Keypad.cpp
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
#include "samd11.h"
#include "hal_gpio.h"
#include "keypad.h"

unsigned long startTime;
char *keymap;
byte *rowPins;
byte *columnPins;
KeypadSize sizeKpd;
uint debounceTime;
uint holdTime;
int single_key;
extern volatile unsigned long milliseconds;

#define MYROWS  4 //four rows
#define MYCOLS  3 //three columns
char my_keypad_keys[MYROWS][MYCOLS] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
};
#if (KEYPAD_INTEGRATED==0)
#warning "KEYPAD: KEYPAD_INTEGRATED=0"
byte my_rowPins[MYROWS] = {27,23,22,17}; //connect to the row pinouts of the keypad
byte my_colPins[MYCOLS] = {16,15,14};    //connect to the column pinouts of the keypad
#elif (KEYPAD_INTEGRATED==1)
#warning "KEYPAD: KEYPAD_INTEGRATED=1"
byte my_rowPins[MYROWS] = {25,7,5,6}; //connect to the row pinouts of the keypad
byte my_colPins[MYCOLS] = {2,3,4};    //connect to the column pinouts of the keypad
#endif
    
    
void key_init(int k);
void key_key_init(int k,char userKeyChar);
void key_key_update(int k,char userKeyChar, KeyState userState, int userStatus);

unsigned long millis(void)
{
    unsigned long ms;
    __disable_irq();
    ms=milliseconds;
    __enable_irq();
    return ms;
        
}

// <<constructor>> Allows custom keymap, pin configuration, and keypad sizes.
void keypad_initialize(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols) {
    rowPins = row;
    columnPins = col;
    sizeKpd.rows = numRows;
    sizeKpd.columns = numCols;
    keymap = userKeymap;

    keypad_setDebounceTime(10);
    keypad_setHoldTime(500);
    //keypadEventListener = 0;

    startTime = 0;
    single_key = false;
}
void keypad_init(int8_t onoff)
{
    if(onoff==0) return;
    keypad_initialize( makeKeymap(my_keypad_keys), my_rowPins, my_colPins, MYROWS, MYCOLS );
}




// Returns a single key only. Retained for backwards compatibility.
char keypad_getKey() {
    single_key = true;

    if (keypad_getKeys() && key[0].stateChanged && (key[0].kstate==PRESSED))
        return key[0].kchar;
    
    single_key = false;

    return NO_KEY;
}

// Populate the key list.
int keypad_getKeys() {
    int keyActivity = false;

    // Limit how often the keypad is scanned. This makes the loop() run 10 times as fast.
    if ( (millis()-startTime)>debounceTime ) {
        keypad_scanKeys();
        keyActivity = keypad_updateList();
        startTime = millis();
    }

    return keyActivity;
}

/*
HAL_GPIO_PIN(KEY_COL0, A, 14)
HAL_GPIO_PIN(KEY_COL1, A, 15)
HAL_GPIO_PIN(KEY_COL2, A, 16)
HAL_GPIO_PIN(KEY_ROW3, A, 17)
HAL_GPIO_PIN(KEY_ROW2, A, 22)
HAL_GPIO_PIN(KEY_ROW1, A, 23)
HAL_GPIO_PIN(KEY_ROW0, A, 27)
*/
// Private : Hardware scan
void keypad_scanKeys() {
    // Re-intialize the row pins. Allows sharing these pins with other hardware.
    for (byte r=0; r<sizeKpd.rows; r++) {
        //same as: HAL_GPIO_KEY_ROW3_in();
        PORT->Group[HAL_GPIO_PORTA].DIRCLR.reg = (1 << rowPins[r]);
        PORT->Group[HAL_GPIO_PORTA].PINCFG[rowPins[r]].reg |= PORT_PINCFG_INEN;
        //PORT->Group[HAL_GPIO_PORTA].PINCFG[rowPins[r]].reg &= ~PORT_PINCFG_PULLEN;
        
        // same as: HAL_GPIO_KEY_ROW3_pullup();
        PORT->Group[HAL_GPIO_PORTA].OUTSET.reg = (1 << rowPins[r]);
        PORT->Group[HAL_GPIO_PORTA].PINCFG[rowPins[r]].reg |= PORT_PINCFG_PULLEN;
    }
    

    // bitMap stores ALL the keys that are being pressed.
    for (byte c=0; c<sizeKpd.columns; c++) {
        // same as: HAL_GPIO_KEY_COL0_out();
        PORT->Group[HAL_GPIO_PORTA].DIRSET.reg = (1 << columnPins[c]); 
        PORT->Group[HAL_GPIO_PORTA].PINCFG[columnPins[c]].reg |= PORT_PINCFG_INEN;
        
        PORT->Group[HAL_GPIO_PORTA].OUTCLR.reg = (1 << columnPins[c]);// same as: HAL_GPIO_COL0_clr(); // Begin column pulse output.
        
        for (byte r=0; r<sizeKpd.rows; r++) {
            //bitWrite(bitMap[r], c, !pin_read(rowPins[r]));  // keypress is active low so invert to high.
            bitWrite(bitMap[r], c, !((PORT->Group[HAL_GPIO_PORTA].IN.reg & (1 << rowPins[r])) != 0));  // keypress is active low so invert to high.
        }
        //same as: uint8_t tripped = HAL_GPIO_TRIPPED_PIN_read() :=
        //return (PORT->Group[HAL_GPIO_PORTA].IN.reg & (1 << rowPins[r])) != 0;
        
        
        // Set pin to high impedance input. Effectively ends column pulse.
        PORT->Group[HAL_GPIO_PORTA].OUTSET.reg = (1 << columnPins[c]);// same as: HAL_GPIO_COL0_set();
        //same as: HAL_GPIO_KEY_COL0_in();
        PORT->Group[HAL_GPIO_PORTA].DIRCLR.reg = (1 << columnPins[c]);
        PORT->Group[HAL_GPIO_PORTA].PINCFG[columnPins[c]].reg |= PORT_PINCFG_INEN;
        PORT->Group[HAL_GPIO_PORTA].PINCFG[columnPins[c]].reg &= ~PORT_PINCFG_PULLEN;
    }
}

// Manage the list without rearranging the keys. Returns true if any keys on the list changed state.
int keypad_updateList() {

    int anyActivity = false;

    // Delete any IDLE keys
    for (byte i=0; i<LIST_MAX; i++) {
        if (key[i].kstate==IDLE) {
            key[i].kchar = NO_KEY;
            key[i].kcode = -1;
            key[i].stateChanged = false;
        }
    }

    // Add new keys to empty slots in the key list.
    for (byte r=0; r<sizeKpd.rows; r++) {
        for (byte c=0; c<sizeKpd.columns; c++) {
            int button = bitRead(bitMap[r],c);
            char keyChar = keymap[r * sizeKpd.columns + c];
            int keyCode = r * sizeKpd.columns + c;
            int idx = keypad_findInList_int (keyCode);
            // Key is already on the list so set its next state.
            if (idx > -1)    {
                keypad_nextKeyState(idx, button);
            }
            // Key is NOT on the list so add it.
            if ((idx == -1) && button) {
                for (byte i=0; i<LIST_MAX; i++) {
                    if (key[i].kchar==NO_KEY) {        // Find an empty slot or don't add key to list.
                        key[i].kchar = keyChar;
                        key[i].kcode = keyCode;
                        key[i].kstate = IDLE;        // Keys NOT on the list have an initial state of IDLE.
                        keypad_nextKeyState (i, button);
                        break;    // Don't fill all the empty slots with the same key.
                    }
                }
            }
        }
    }

    // Report if the user changed the state of any key.
    for (byte i=0; i<LIST_MAX; i++) {
        if (key[i].stateChanged) anyActivity = true;
    }

    return anyActivity;
}

// Private
// This function is a state machine but is also used for debouncing the keys.
void keypad_nextKeyState(byte idx, int button) {
    key[idx].stateChanged = false;

    switch (key[idx].kstate) {
        case IDLE:
            if (button==CLOSED) {
                keypad_transitionTo (idx, PRESSED);
                holdTimer = millis(); }        // Get ready for next HOLD state.
            break;
        case PRESSED:
            if ((millis()-holdTimer)>holdTime)    // Waiting for a key HOLD...
                keypad_transitionTo (idx, HOLD);
            else if (button==OPEN)                // or for a key to be RELEASED.
                keypad_transitionTo (idx, RELEASED);
            break;
        case HOLD:
            if (button==OPEN)
                keypad_transitionTo (idx, RELEASED);
            break;
        case RELEASED:
            keypad_transitionTo (idx, IDLE);
            break;
    }
}

// New in 2.1
int keypad_isPressed(char keyChar) {
    for (byte i=0; i<LIST_MAX; i++) {
        if ( key[i].kchar == keyChar ) {
            if ( (key[i].kstate == PRESSED) && key[i].stateChanged )
                return true;
        }
    }
    return false;    // Not pressed.
}

// Search by character for a key in the list of active keys.
// Returns -1 if not found or the index into the list of active keys.
int keypad_findInList_char (char keyChar) {
    for (byte i=0; i<LIST_MAX; i++) {
        if (key[i].kchar == keyChar) {
            return i;
        }
    }
    return -1;
}

// Search by code for a key in the list of active keys.
// Returns -1 if not found or the index into the list of active keys.
int keypad_findInList_int (int keyCode) {
    for (byte i=0; i<LIST_MAX; i++) {
        if (key[i].kcode == keyCode) {
            return i;
        }
    }
    return -1;
}

// New in 2.0
char keypad_waitForKey() {
    char waitKey = NO_KEY;
    while( (waitKey = keypad_getKey()) == NO_KEY );    // Block everything while waiting for a keypress.
    return waitKey;
}

// Backwards compatibility function.
KeyState keypad_getState() {
    return key[0].kstate;
}

// The end user can test for any changes in state before deciding
// if any variables, etc. needs to be updated in their code.
int keypad_keyStateChanged() {
    return key[0].stateChanged;
}

// The number of keys on the key list, key[LIST_MAX], equals the number
// of bytes in the key list divided by the number of bytes in a Key object.
byte keypad_numKeys() {
    return sizeof(key)/sizeof(Key);
}

// Minimum debounceTime is 1 mS. Any lower *will* slow down the loop().
void keypad_setDebounceTime(uint debounce) {
    if(debounce<1) debounceTime=1; else debounceTime=debounce;
}

void keypad_setHoldTime(uint hold) {
    holdTime = hold;
}

/*
void keypad_addEventListener(void (*listener)(char)){
    keypadEventListener = listener;
}
*/
void keypad_transitionTo(byte idx, KeyState nextState) {
    key[idx].kstate = nextState;
    key[idx].stateChanged = true;
/*
    // Sketch used the getKey() function.
    // Calls keypadEventListener only when the first key in slot 0 changes state.
    if (single_key)  {
          if ( (keypadEventListener!=NULL) && (idx==0) )  {
            keypadEventListener(key[0].kchar);
        }
    }
    // Sketch used the getKeys() function.
    // Calls keypadEventListener on any key that changes state.
    else {
          if (keypadEventListener!=NULL)  {
            keypadEventListener(key[idx].kchar);
        }
    }
    */
}

void key_init(int k) {
    key[k].kchar = NO_KEY;
    key[k].kstate = IDLE;
    key[k].stateChanged = false;
}

// constructor
void key_key_init(int k,char userKeyChar) {
    key[k].kchar = userKeyChar;
    key[k].kcode = -1;
    key[k].kstate = IDLE;
    key[k].stateChanged = false;
}

void key_key_update (int k,char userKeyChar, KeyState userState, int userStatus) {
    key[k].kchar = userKeyChar;
    key[k].kstate = userState;
    key[k].stateChanged = userStatus;
}
