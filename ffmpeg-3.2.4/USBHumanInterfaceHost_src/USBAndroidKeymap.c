/*
 ============================================================================
 Name        : USBAndroidKeymap.c
 Author      : lvhwa0716@gmail.com
 Version     :
 Copyright   : 
 Description :  Ansi-style
 ============================================================================
 */

#include <SDL/SDL_keysym.h>
#include "USBAndroid.h"
// SDL => USB Key
int USBAndroidKeymap_getKeyMap(int key) {
	switch(key) {
		case SDLK_BACKSPACE:
			return 0x2A;
			break;
		case SDLK_TAB:
			return 0x2B;
			break;
		case SDLK_CLEAR:
			break;
		case SDLK_RETURN:
			return 0x28;
			break;
		case SDLK_PAUSE:
			break;
		case SDLK_ESCAPE:
			return 0x29;
			break;
		case SDLK_SPACE:
			return 0x2C;
			break;
		case SDLK_EXCLAIM:
			break;
		case SDLK_QUOTEDBL:
			break;
		case SDLK_HASH:
			break;
		case SDLK_DOLLAR:
			break;
		case SDLK_AMPERSAND:
			break;
		case SDLK_QUOTE:
			break;
		case SDLK_LEFTPAREN:
			break;
		case SDLK_RIGHTPAREN:
			break;
		case SDLK_ASTERISK:
			break;
		case SDLK_PLUS:
			break;
		case SDLK_COMMA:
			break;
		case SDLK_MINUS:
			break;
		case SDLK_PERIOD:
			break;
		case SDLK_SLASH:
			break;
		case SDLK_0:
			break;
		case SDLK_1:
			break;
		case SDLK_2:
			break;
		case SDLK_3:
			break;
		case SDLK_4:
			break;
		case SDLK_5:
			break;
		case SDLK_6:
			break;
		case SDLK_7:
			break;
		case SDLK_8:
			break;
		case SDLK_9:
			break;
		case SDLK_COLON:
			break;
		case SDLK_SEMICOLON:
			break;
		case SDLK_LESS:
			break;
		case SDLK_EQUALS:
			break;
		case SDLK_GREATER:
			break;
		case SDLK_QUESTION:
			break;
		case SDLK_AT:
			break;
	/* 
	   Skip uppercase letters
	 */
		case SDLK_LEFTBRACKET:
			break;
		case SDLK_BACKSLASH:
			break;
		case SDLK_RIGHTBRACKET:
			break;
		case SDLK_CARET:
			break;
		case SDLK_UNDERSCORE:
			break;
		case SDLK_BACKQUOTE:
			break;
		case SDLK_a:
			return 4;
			break;
		case SDLK_b:
			return 5;
			break;
		case SDLK_c:
			return 6;
			break;
		case SDLK_d:
			return 7;
			break;
		case SDLK_e:
			return 8;
			break;
		case SDLK_f:
			return 9;
			break;
		case SDLK_g:
			return 10;
			break;
		case SDLK_h:
			return 11;
			break;
		case SDLK_i:
			return 12;
			break;
		case SDLK_j:
			return 13;
			break;
		case SDLK_k:
			return 14;
			break;
		case SDLK_l:
			return 15;
			break;
		case SDLK_m:
			return 16;
			break;
		case SDLK_n:
			return 17;
			break;
		case SDLK_o:
			return 18;
			break;
		case SDLK_p:
			return 19;
			break;
		case SDLK_q:
			return 20;
			break;
		case SDLK_r:
			return 21;
			break;
		case SDLK_s:
			return 22;
			break;
		case SDLK_t:
			return 23;
			break;
		case SDLK_u:
			return 24;
			break;
		case SDLK_v:
			return 25;
			break;
		case SDLK_w:
			return 26;
			break;
		case SDLK_x:
			return 27;
			break;
		case SDLK_y:
			return 28;
			break;
		case SDLK_z:
			return 29;
			break;
		case SDLK_DELETE:
			break;
	/* End of ASCII mapped keysyms */
        /*@}*/

#if 0
	/** @name International keyboard syms */
        /*@{*/
	case SDLK_WORLD_0		= 160,		/* 0xA0 */
	SDLK_WORLD_1		= 161,
	SDLK_WORLD_2		= 162,
	SDLK_WORLD_3		= 163,
	SDLK_WORLD_4		= 164,
	SDLK_WORLD_5		= 165,
	SDLK_WORLD_6		= 166,
	SDLK_WORLD_7		= 167,
	SDLK_WORLD_8		= 168,
	SDLK_WORLD_9		= 169,
	SDLK_WORLD_10		= 170,
	SDLK_WORLD_11		= 171,
	SDLK_WORLD_12		= 172,
	SDLK_WORLD_13		= 173,
	SDLK_WORLD_14		= 174,
	SDLK_WORLD_15		= 175,
	SDLK_WORLD_16		= 176,
	SDLK_WORLD_17		= 177,
	SDLK_WORLD_18		= 178,
	SDLK_WORLD_19		= 179,
	SDLK_WORLD_20		= 180,
	SDLK_WORLD_21		= 181,
	SDLK_WORLD_22		= 182,
	SDLK_WORLD_23		= 183,
	SDLK_WORLD_24		= 184,
	SDLK_WORLD_25		= 185,
	SDLK_WORLD_26		= 186,
	SDLK_WORLD_27		= 187,
	SDLK_WORLD_28		= 188,
	SDLK_WORLD_29		= 189,
	SDLK_WORLD_30		= 190,
	SDLK_WORLD_31		= 191,
	SDLK_WORLD_32		= 192,
	SDLK_WORLD_33		= 193,
	SDLK_WORLD_34		= 194,
	SDLK_WORLD_35		= 195,
	SDLK_WORLD_36		= 196,
	SDLK_WORLD_37		= 197,
	SDLK_WORLD_38		= 198,
	SDLK_WORLD_39		= 199,
	SDLK_WORLD_40		= 200,
	SDLK_WORLD_41		= 201,
	SDLK_WORLD_42		= 202,
	SDLK_WORLD_43		= 203,
	SDLK_WORLD_44		= 204,
	SDLK_WORLD_45		= 205,
	SDLK_WORLD_46		= 206,
	SDLK_WORLD_47		= 207,
	SDLK_WORLD_48		= 208,
	SDLK_WORLD_49		= 209,
	SDLK_WORLD_50		= 210,
	SDLK_WORLD_51		= 211,
	SDLK_WORLD_52		= 212,
	SDLK_WORLD_53		= 213,
	SDLK_WORLD_54		= 214,
	SDLK_WORLD_55		= 215,
	SDLK_WORLD_56		= 216,
	SDLK_WORLD_57		= 217,
	SDLK_WORLD_58		= 218,
	SDLK_WORLD_59		= 219,
	SDLK_WORLD_60		= 220,
	SDLK_WORLD_61		= 221,
	SDLK_WORLD_62		= 222,
	SDLK_WORLD_63		= 223,
	SDLK_WORLD_64		= 224,
	SDLK_WORLD_65		= 225,
	SDLK_WORLD_66		= 226,
	SDLK_WORLD_67		= 227,
	SDLK_WORLD_68		= 228,
	SDLK_WORLD_69		= 229,
	SDLK_WORLD_70		= 230,
	SDLK_WORLD_71		= 231,
	SDLK_WORLD_72		= 232,
	SDLK_WORLD_73		= 233,
	SDLK_WORLD_74		= 234,
	SDLK_WORLD_75		= 235,
	SDLK_WORLD_76		= 236,
	SDLK_WORLD_77		= 237,
	SDLK_WORLD_78		= 238,
	SDLK_WORLD_79		= 239,
	SDLK_WORLD_80		= 240,
	SDLK_WORLD_81		= 241,
	SDLK_WORLD_82		= 242,
	SDLK_WORLD_83		= 243,
	SDLK_WORLD_84		= 244,
	SDLK_WORLD_85		= 245,
	SDLK_WORLD_86		= 246,
	SDLK_WORLD_87		= 247,
	SDLK_WORLD_88		= 248,
	SDLK_WORLD_89		= 249,
	SDLK_WORLD_90		= 250,
	SDLK_WORLD_91		= 251,
	SDLK_WORLD_92		= 252,
	SDLK_WORLD_93		= 253,
	SDLK_WORLD_94		= 254,
	SDLK_WORLD_95		= 255,		/* 0xFF */
        /*@}*/
#endif
	/** @name Numeric keypad */
        /*@{*/
		case SDLK_KP0:
			break;
		case SDLK_KP1:
			break;
		case SDLK_KP2:
			break;
		case SDLK_KP3:
			break;
		case SDLK_KP4:
			break;
		case SDLK_KP5:
			break;
		case SDLK_KP6:
			break;
		case SDLK_KP7:
			break;
		case SDLK_KP8:
			break;
		case SDLK_KP9:
			break;
		case SDLK_KP_PERIOD:
			break;
		case SDLK_KP_DIVIDE:
			break;
		case SDLK_KP_MULTIPLY:
			break;
		case SDLK_KP_MINUS:
			break;
		case SDLK_KP_PLUS:
			break;
		case SDLK_KP_ENTER:
			break;
		case SDLK_KP_EQUALS:
			break;
        /*@}*/

	/** @name Arrows + Home/End pad */
        /*@{*/
		case SDLK_UP:
			break;
		case SDLK_DOWN:
			break;
		case SDLK_RIGHT:
			break;
		case SDLK_LEFT:
			break;
		case SDLK_INSERT:
			break;
		case SDLK_HOME:
			break;
		case SDLK_END:
			break;
		case SDLK_PAGEUP:
			break;
		case SDLK_PAGEDOWN:
			break;
        /*@}*/

	/** @name Function keys */
        /*@{*/
		case SDLK_F1:
			break;
		case SDLK_F2:
			break;
		case SDLK_F3:
			break;
		case SDLK_F4:
			break;
		case SDLK_F5:
			break;
		case SDLK_F6:
			break;
		case SDLK_F7:
			break;
		case SDLK_F8:
			break;
		case SDLK_F9:
			break;
		case SDLK_F10:
			break;
		case SDLK_F11:
			break;
		case SDLK_F12:
			break;
		case SDLK_F13:
			break;
		case SDLK_F14:
			break;
		case SDLK_F15:
			break;

        /*@}*/

	/** @name Key state modifier keys */
        /*@{*/
		case SDLK_NUMLOCK:
			break;
		case SDLK_CAPSLOCK:
			break;
		case SDLK_SCROLLOCK:
			break;
		case SDLK_RSHIFT:
			break;
		case SDLK_LSHIFT:
			break;
		case SDLK_RCTRL:
			break;
		case SDLK_LCTRL:
			break;
		case SDLK_RALT:
			break;
		case SDLK_LALT:
			break;
		case SDLK_RMETA:
			break;
		case SDLK_LMETA:
			break;
		case SDLK_LSUPER:
			break;
		case SDLK_RSUPER: /**< Right "Windows" key */
			break;
		case SDLK_MODE:		/**< "Alt Gr" key */
			break;
		case SDLK_COMPOSE:		/**< Multi-key compose key */
			break;
        /*@}*/

	/** @name Miscellaneous function keys */
        /*@{*/
		case SDLK_HELP:
			break;
		case SDLK_PRINT:
			break;
		case SDLK_SYSREQ:
			break;
		case SDLK_BREAK:
			break;
		case SDLK_MENU:
			break;
		case SDLK_POWER:
			break;	/**< Power Macintosh power key */
		case SDLK_EURO:
			break;	/**< Some european keyboards */
		case SDLK_UNDO:
			break;		/**< Atari keyboard has Undo */
        /*@}*/

	/* Add any other keys here */

		case SDLK_LAST:
		default:
			break;
	}
	return -1;
}
