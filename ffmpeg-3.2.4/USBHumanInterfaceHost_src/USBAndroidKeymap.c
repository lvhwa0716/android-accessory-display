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

/*
0 00 Reserved (no event indicated)9 N/A √ √ √ 4/101/104  
1 01 Keyboard ErrorRollOver9 N/A √ √ √ 4/101/104  
2 02 Keyboard POSTFail9 N/A √ √ √ 4/101/104  
3 03 Keyboard ErrorUndefined9 N/A √ √ √ 4/101/104  
4 04 Keyboard a and A4 31 √ √ √ 4/101/104  
5 05 Keyboard b and B 50 √ √ √ 4/101/104  
6 06 Keyboard c and C4 48 √ √ √ 4/101/104  
7 07 Keyboard d and D 33 √ √ √ 4/101/104  
8 08 Keyboard e and E 19 √ √ √ 4/101/104  
9 09 Keyboard f and F 34 √ √ √ 4/101/104  
10 0A Keyboard g and G 35 √ √ √ 4/101/104  
11 0B Keyboard h and H 36 √ √ √ 4/101/104  
12 0C Keyboard i and I 24 √ √ √ 4/101/104  
13 0D Keyboard j and J 37 √ √ √ 4/101/104  
14 0E Keyboard k and K 38 √ √ √ 4/101/104  
15 0F Keyboard l and L 39 √ √ √ 4/101/104  
16 10 Keyboard m and M4 52 √ √ √ 4/101/104  
17 11 Keyboard n and N 51 √ √ √ 4/101/104  
18 12 Keyboard o and O4 25 √ √ √ 4/101/104  
19 13 Keyboard p and P4 26 √ √ √ 4/101/104  
20 14 Keyboard q and Q4 17 √ √ √ 4/101/104  
21 15 Keyboard r and R 20 √ √ √ 4/101/104  
22 16 Keyboard s and S4 32 √ √ √ 4/101/104  
23 17 Keyboard t and T 21 √ √ √ 4/101/104  
24 18 Keyboard u and U 23 √ √ √ 4/101/104  
25 19 Keyboard v and V 49 √ √ √ 4/101/104  
26 1A Keyboard w and W4 18 √ √ √ 4/101/104  
27 1B Keyboard x and X4 47 √ √ √ 4/101/104  
28 1C Keyboard y and Y4 22 √ √ √ 4/101/104  
29 1D Keyboard z and Z4 46 √ √ √ 4/101/104  
30 1E Keyboard 1 and !4 2 √ √ √ 4/101/104  
31 1F Keyboard 2 and @4 3 √ √ √ 4/101/104  
32 20 Keyboard 3 and #4 4 √ √ √ 4/101/104  
33 21 Keyboard 4 and $4 5 √ √ √ 4/101/104  
34 22 Keyboard 5 and %4 6 √ √ √ 4/101/104  
35 23 Keyboard 6 and ^4 7 √ √ √ 4/101/104  
36 24 Keyboard 7 and &4 8 √ √ √ 4/101/104  
37 25 Keyboard 8 and *4 9 √ √ √ 4/101/104  
38 26 Keyboard 9 and (4 10 √ √ √ 4/101/104  
39 27 Keyboard 0 and )4 11 √ √ √ 4/101/104  
40 28 Keyboard Return (ENTER)5 43 √ √ √ 4/101/104  
41 29 Keyboard ESCAPE 110 √ √ √ 4/101/104  
42 2A Keyboard DELETE (Backspace)13 15 √ √ √ 4/101/104  
43 2B Keyboard Tab 16 √ √ √ 4/101/104  
44 2C Keyboard Spacebar 61 √ √ √ 4/101/104  
45 2D Keyboard - and (underscore)4 12 √ √ √ 4/101/104  
46 2E Keyboard = and +4 13 √ √ √ 4/101/104  
47 2F Keyboard [ and {4 27 √ √ √ 4/101/104  
48 30 Keyboard ] and }4 28 √ √ √ 4/101/104  
49 31 Keyboard \ and | 29 √ √ √ 4/101/104  
50 32 Keyboard Non-US # and ~2 42 √ √ √ 4/101/104  
51 33 Keyboard ; and :4 40 √ √ √ 4/101/104  
52 34 Keyboard ‘ and “4 41 √ √ √ 4/101/104  
53 35 Keyboard Grave Accent and Tilde4 1 √ √ √ 4/101/104  
54 36 Keyboard, and <4 53 √ √ √ 4/101/104  
55 37 Keyboard . and >4 54 √ √ √ 4/101/104  
56 38 Keyboard / and ?4 55 √ √ √ 4/101/104  
57 39 Keyboard Caps Lock11 30 √ √ √ 4/101/104  
58 3A Keyboard F1 112 √ √ √ 4/101/104  
59 3B Keyboard F2 113 √ √ √ 4/101/104  
60 3C Keyboard F3 114 √ √ √ 4/101/104  
61 3D Keyboard F4 115 √ √ √ 4/101/104  
62 3E Keyboard F5 116 √ √ √ 4/101/104  
63 3F Keyboard F6 117 √ √ √ 4/101/104  
64 40 Keyboard F7 118 √ √ √ 4/101/104  
65 41 Keyboard F8 119 √ √ √ 4/101/104  
66 42 Keyboard F9 120 √ √ √ 4/101/104  
67 43 Keyboard F10 121 √ √ √ 4/101/104  
68 44 Keyboard F11 122 √ √ √ 101/104  
69 45 Keyboard F12 123 √ √ √ 101/104  
70 46 Keyboard PrintScreen1 124 √ √ √ 101/104  
71 47 Keyboard Scroll Lock11 125 √ √ √ 4/101/104  
72 48 Keyboard Pause1 126 √ √ √ 101/104  
73 49 Keyboard Insert1 75 √ √ √ 101/104  
74 4A Keyboard Home1 80 √ √ √ 101/104  
75 4B Keyboard PageUp1 85 √ √ √ 101/104  
76 4C Keyboard Delete Forward1;14 76 √ √ √ 101/104  
77 4D Keyboard End1 81 √ √ √ 101/104  
78 4E Keyboard PageDown1 86 √ √ √ 101/104  
79 4F Keyboard RightArrow1 89 √ √ √ 101/104  
80 50 Keyboard LeftArrow1 79 √ √ √ 101/104  
81 51 Keyboard DownArrow1 84 √ √ √ 101/104  
82 52 Keyboard UpArrow1 83 √ √ √ 101/104  
83 53 Keypad Num Lock and Clear11 90 √ √ √ 101/104  
84 54 Keypad /1 95 √ √ √ 101/104  
85 55 Keypad * 100 √ √ √ 4/101/104  
86 56 Keypad - 105 √ √ √ 4/101/104  
87 57 Keypad + 106 √ √ √ 4/101/104  
88 58 Keypad ENTER5 108 √ √ √ 101/104  
89 59 Keypad 1 and End 93 √ √ √ 4/101/104  
90 5A Keypad 2 and Down Arrow 98 √ √ √ 4/101/104  
91 5B Keypad 3 and PageDn 103 √ √ √ 4/101/104  
92 5C Keypad 4 and Left Arrow 92 √ √ √ 4/101/104  
93 5D Keypad 5 97 √ √ √ 4/101/104  
94 5E Keypad 6 and Right Arrow 102 √ √ √ 4/101/104  
95 5F Keypad 7 and Home 91 √ √ √ 4/101/104  
96 60 Keypad 8 and Up Arrow 96 √ √ √ 4/101/104  
97 61 Keypad 9 and PageUp 101 √ √ √ 4/101/104  
98 62 Keypad 0 and Insert 99 √ √ √ 4/101/104  
99 63 Keypad . and Delete 104 √ √ √ 4/101/104  
100 64 Keyboard Non-US \ and |3;6 45 √ √ √ 4/101/104  
101 65 Keyboard Application10 129 √ √ 104  
102 66 Keyboard Power9 √ √  
103 67 Keypad = √  
104 68 Keyboard F13 √  
105 69 Keyboard F14 √  
106 6A Keyboard F15 √  
107 6B Keyboard F16  
108 6C Keyboard F17  
109 6D Keyboard F18  
110 6E Keyboard F19  
111 6F Keyboard F20  
112 70 Keyboard F21  
113 71 Keyboard F22  
114 72 Keyboard F23  
115 73 Keyboard F24  
116 74 Keyboard Execute √  
117 75 Keyboard Help √  
118 76 Keyboard Menu √  
119 77 Keyboard Select √  
120 78 Keyboard Stop √  
121 79 Keyboard Again √  
122 7A Keyboard Undo √  
123 7B Keyboard Cut √  
124 7C Keyboard Copy √  
125 7D Keyboard Paste √  
126 7E Keyboard Find √  
127 7F Keyboard Mute √  
128 80 Keyboard Volume Up √  
129 81 Keyboard Volume Down √  
130 82 Keyboard Locking Caps Lock12 √  
131 83 Keyboard Locking Num Lock12 √  
132 84 Keyboard Locking Scroll Lock12 √  
133 85 Keypad Comma27 107  
134 86 Keypad Equal Sign29  
135 87 Keyboard International115,28 56  
136 88 Keyboard International216  
137 89 Keyboard International317  
138 8A Keyboard International418  
139 8B Keyboard International519  
140 8C Keyboard International620  
141 8D Keyboard International721  
142 8E Keyboard International822  
143 8F Keyboard International922  
144 90 Keyboard LANG125  
145 91 Keyboard LANG226  
146 92 Keyboard LANG330  
147 93 Keyboard LANG431  
148 94 Keyboard LANG532  
149 95 Keyboard LANG68  
150 96 Keyboard LANG78  
151 97 Keyboard LANG88  
152 98 Keyboard LANG98  
153 99 Keyboard Alternate Erase7  
154 9A Keyboard SysReq/Attention1  
155 9B Keyboard Cancel  
156 9C Keyboard Clear  
157 9D Keyboard Prior  
158 9E Keyboard Return  
159 9F Keyboard Separator  
160 A0 Keyboard Out  
161 A1 Keyboard Oper  
162 A2 Keyboard Clear/Again  
163 A3 Keyboard CrSel/Props  
164 A4 Keyboard ExSel  
165-175 A5-CF Reserved  
176 B0 Keypad 00  
177 B1 Keypad 000  
178 B2 Thousands Separator 33  
179 B3 Decimal Separator 33  
180 B4 Currency Unit 34  
181 B5 Currency Sub-unit 34  
182 B6 Keypad (  
183 B7 Keypad )  
184 B8 Keypad {  
185 B9 Keypad }  
186 BA Keypad Tab  
187 BB Keypad Backspace  
188 BC Keypad A  
189 BD Keypad B  
190 BE Keypad C  
191 BF Keypad D  
192 C0 Keypad E  
193 C1 Keypad F  
194 C2 Keypad XOR  
195 C3 Keypad ^  
196 C4 Keypad %  
197 C5 Keypad <  
198 C6 Keypad >  
199 C7 Keypad &  
200 C8 Keypad &&  
201 C9 Keypad |  
202 CA Keypad ||  
203 CB Keypad :  
204 CC Keypad #  
205 CD Keypad Space  
206 CE Keypad @  
207 CF Keypad !  
208 D0 Keypad Memory Store  
209 D1 Keypad Memory Recall  
210 D2 Keypad Memory Clear  
211 D3 Keypad Memory Add  
212 D4 Keypad Memory Subtract  
213 D5 Keypad Memory Multiply  
214 D6 Keypad Memory Divide  
215 D7 Keypad +/-  
216 D8 Keypad Clear  
217 D9 Keypad Clear Entry  
218 DA Keypad Binary  
219 DB Keypad Octal  
220 DC Keypad Decimal  
221 DD Keypad Hexadecimal  
222-223 DE-DF Reserved  
224 E0 Keyboard LeftControl 58 √ √ √ 4/101/104  
225 E1 Keyboard LeftShift 44 √ √ √ 4/101/104  
226 E2 Keyboard LeftAlt 60 √ √ √ 4/101/104  
227 E3 Keyboard Left GUI10;23 127 √ √ √ 104  
228 E4 Keyboard RightControl 64 √ √ √ 101/104  
229 E5 Keyboard RightShift 57 √ √ √ 4/101/104  
230 E6 Keyboard RightAlt 62 √ √ √ 101/104  
231 E7 Keyboard Right GUI10;24 128 √ √ √ 104  
232-65535 E8-FFFF Reserved  
*/
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
			return 0x48;
			break;
		case SDLK_ESCAPE:
			return 0x29;
			break;
		case SDLK_SPACE:
			return 0x2C;
			break;
		case SDLK_EXCLAIM:
			break;
		case SDLK_BACKQUOTE:
		case SDLK_QUOTEDBL:
			return 0x34;
			break;
		case SDLK_HASH:
			break;
		case SDLK_DOLLAR:
			return 0x21;
			break;
		case SDLK_AMPERSAND:
			break;
		case SDLK_QUOTE:
			return 0x34;
			break;
		case SDLK_LEFTPAREN:
			return 0x2F;
			break;
		case SDLK_RIGHTPAREN:
			return 0x30;
			break;
		case SDLK_ASTERISK:
			return 0x25;
			break;





		case SDLK_0:
			return 0x27;
			break;
		case SDLK_1:
			return 0x1E;
			break;
		case SDLK_2:
			return 0x1F;
			break;
		case SDLK_3:
			return 0x20;
			break;
		case SDLK_4:
			return 0x21;
			break;
		case SDLK_5:
			return 0x22;
			break;
		case SDLK_6:
			return 0x23;
			break;
		case SDLK_7:
			return 0x24;
			break;
		case SDLK_8:
			return 0x25;
			break;
		case SDLK_9:
			return 0x26;
			break;
		case SDLK_COLON:
		case SDLK_SEMICOLON:
			return 0x33;
			break;
		case SDLK_LESS:
		case SDLK_COMMA:
			return 0x36;
			break;
		case SDLK_PLUS:
		case SDLK_EQUALS:
			return 0x2E;
			break;
		case SDLK_PERIOD:
		case SDLK_GREATER:
			return 0x37;
			break;
		case SDLK_SLASH:
		case SDLK_QUESTION:
			return 0x38;
			break;
		case SDLK_AT:
			return 0x1F;
			break;
	/* 
	   Skip uppercase letters
	 */
		case SDLK_LEFTBRACKET:
			return 0x2F;
			break;
		case SDLK_BACKSLASH:
			return 0x31;
			break;
		case SDLK_RIGHTBRACKET:
			return 0x30;
			break;
		case SDLK_CARET:
			break;
		case SDLK_MINUS:
		case SDLK_UNDERSCORE:
			return 0x2D;
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
			return 0x2A;
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
			return 0x62;
			break;
		case SDLK_KP1:
			return 0x59;
			break;
		case SDLK_KP2:
			return 0x5A;
			break;
		case SDLK_KP3:
			return 0x5B;
			break;
		case SDLK_KP4:
			return 0x5C;
			break;
		case SDLK_KP5:
			return 0x5D;
			break;
		case SDLK_KP6:
			return 0x5E;
			break;
		case SDLK_KP7:
			return 0x5F;
			break;
		case SDLK_KP8:
			return 0x60;
			break;
		case SDLK_KP9:
			return 0x61;
			break;
		case SDLK_KP_PERIOD:
			return 0x63;
			break;
		case SDLK_KP_DIVIDE:
			return 0x54;
			break;
		case SDLK_KP_MULTIPLY:
			return 0x55;
			break;
		case SDLK_KP_MINUS:
			return 0x56;
			break;
		case SDLK_KP_PLUS:
			return 0x57;
			break;
		case SDLK_KP_ENTER:
			return 0x58;
			break;
		case SDLK_KP_EQUALS:
			break;
        /*@}*/

	/** @name Arrows + Home/End pad */
        /*@{*/
		case SDLK_UP:
			return 0x52;
			break;
		case SDLK_DOWN:
			return 0x51;
			break;
		case SDLK_RIGHT:
			return 0x4F;
			break;
		case SDLK_LEFT:
			return 0x50;
			break;
		case SDLK_INSERT:
			return 0x49;
			break;
		case SDLK_HOME:
			return 0x4A;
			break;
		case SDLK_END:
			return 0x4D;
			break;
		case SDLK_PAGEUP:
			return 0x4B;
			break;
		case SDLK_PAGEDOWN:
			return 0x4E;
			break;
        /*@}*/

	/** @name Function keys */
        /*@{*/
		case SDLK_F1:
			return 0x3A;
			break;
		case SDLK_F2:
			return 0x3B;
			break;
		case SDLK_F3:
			return 0x3C;
			break;
		case SDLK_F4:
			return 0x3D;
			break;
		case SDLK_F5:
			return 0x3E;
			break;
		case SDLK_F6:
			return 0x3F;
			break;
		case SDLK_F7:
			return 0x40;
			break;
		case SDLK_F8:
			return 0x41;
			break;
		case SDLK_F9:
			return 0x42;
			break;
		case SDLK_F10:
			return 0x43;
			break;
		case SDLK_F11:
			return 0x44;
			break;
		case SDLK_F12:
			return 0x45;
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
			return 0x23;
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
