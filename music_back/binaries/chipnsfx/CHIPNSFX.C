/* ***************************************************************
CHIPNSFX interactive three-channel musical tracker and data editor
written by Cesar Nicolas-Gonzalez / CNGSOFT since 2017-02-09 14:20
*************************************************************** */

#define MY_FILE "CHIPNSFX"
#define MY_DATE "20240422"
#define MY_NAME "CHIPNSFX tracker"
#define MY_SELF "Cesar Nicolas-Gonzalez / CNGSOFT"

#define GPL_3_INFO \
	"This program comes with ABSOLUTELY NO WARRANTY; for more details" "\n" \
	"please see the GNU General Public License. This is free software" "\n" \
	"and you are welcome to redistribute it under certain conditions."

/* This notice applies to the sources of CHIPNSFX and its binaries.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Contact information: <mailto:cngsoft@gmail.com> */

#define INLINE // 'inline': useless in TCC and GCC4, harmful in GCC5!
INLINE int ucase(int i) { return i>='a'&&i<='z'?i-32:i; }
INLINE int lcase(int i) { return i>='A'&&i<='Z'?i+32:i; }
#define length(x) (sizeof(x)/sizeof(*(x)))

#include <stdio.h>
#include <stdlib.h> // malloc...
#include <string.h> // strcpy...

#ifndef DEBUG // enable UI?

#ifdef _WIN32
	#define strcasecmp _stricmp
	#define WIN32_LEAN_AND_MEAN 1
	#include <windows.h> // UI, FindFirstFile()...
#else
	#include <dirent.h> // opendir()...
	#include <sys/stat.h> // stat()...
	#ifndef SDL2
		#define SDL2 // fallback!
	#endif
	#define INT8 Sint8
	#define BYTE Uint8
	#define WORD Uint16
	#define DWORD Uint32
#endif
#ifdef SDL2
	#ifndef SDL_MAIN_HANDLED
	#define SDL_MAIN_HANDLED // required!
	#endif
	#include <SDL2/SDL.h> // UI+audio; requires -lSDL2
#else
	#include <mmsystem.h> // audio; requires -lwinmm
#endif

#endif

#define MAXSTRLEN 1024 // a wave frame (44100*2/100=882) or a pattern ("#01 A-4:01 ...":7*(1+128)=903) must fit inside
FILE *f; // I/O operations never require more than one file at once

#define CHIPNSFX_SIZE 112 // 0x70, note/control code threshold: in the target encoding, 0x00..0x6B are notes (C-0..B-8),
	// 0x6C..0x6F are "SFX", "===", "???" and "^^^", 0x70..0xEF are lengths (1..128) and 0xF0..0xFF are commands.
	// "???"/110 is used only when generating length-optimisation "hollow" notes from the source to the target.
	// Notice that within the tracker 0 means "...", 1..108 mean C-0..B-8, 109 is "SFX", 110 is "===" and 112 is "^^^"!

const char chipnsfxid[]=MY_FILE " format",chipnsfxlf[]="===\n",crlf[]="\n", // file format strings
	vubars[][17]={
		#ifdef SDL2
		" ____\034\034\034\034\035\035\035\036\036\037\037"//custom font
		#else
		" ....ooooOOO88@@"//pure ASCII
		//"\040\040\372\372\055\055\376\376\260\260\261\261\262\262\333\333"//OEM chars
		#endif
		,"0123456789ABCDEF"}, // note amplitude, visual or numeric
	notes1[]="CCDDEFFGGAAB",notes2[]="-#-#--#-#-#-"; // C- C# D- D# E- F- F# G- G# A- A# B-

char pianos[]="zsxdcvgbhnjm,q2w3er5t6y7ui9o0pl1a."; // standard QWERTY keyboard: 12+12+6+SFX+REST+BRAKE+ERASE
const char pianoz[][sizeof(pianos)]={
	"ysxdcvgbhnjm,q2w3er5t6z7ui9o0pl1a.", // QWERTZ keyboard
	#ifdef SDL2
	"wsxdcvgbhnj,;a\351z\042er(t-y\350ui\230o\340pl&q:" // AZERTY keyboard, SDL2:ANSI
	#else
	"wsxdcvgbhnj,;a\202z\042er(t-y\212ui\207o\205pl&q:" // AZERTY keyboard, WIN32:OEM
	#endif
	};

// song header fields
int hertz,divisor,transpose,loopto,orders,instrs,loopto_,orders_,instrs_;
// song body contents
unsigned char tmp[MAXSTRLEN],*title0="song_", // work buffer and output label
	title1[MAXSTRLEN],title2[MAXSTRLEN], // song name and signature
	filepath[MAXSTRLEN], // song location
	order[3][256][2],
	order_[3][256][2], // 3 channels * 256 orders * index & transposition
	scrln[256],
	scrln_[256], // 256 score pattern lengths ranging from 0 (void) to 128 (full)
	score[256][128][2],
	score_[256][128][2], // 256 score patterns * 128 rows * note+instrument
	instr[256][25],
	instr_[256][25], // 256 instruments * 8 bytes + 16 ASCIZ + NULL
	clipboard[3][256][2];
int clipboardtype,clipboardwidth,clipboarditems;

// general purpose operations --------------------------------- //

#define MEMZERO(v) memset((v),0,sizeof((v)))
#define MEMFULL(v) memset((v),255,sizeof((v)))
#define MEMSAVE(v,w) memcpy((v),(w),sizeof((w)))
#define MEMLOAD(v,w) memcpy((v),(w),sizeof((v)))
INLINE int itoradix(int x) { return x<10?x+48:((x-9)+64); } // base36: turns 0..35 into 0123..XYZ;
INLINE int radixtoi(int x) { return x<65?x-48:((x+9)&15); } // flawed on purpose, allowing ":" as 10
char *strcln(unsigned char *s) // remove trailing whitespaces from a string
{
	if (!s)
		return NULL;
	unsigned char *r=s;
	while (*r) ++r;
	while (r>s&&(*--r<33)) *r=0;
	return s;
}
char *strpad(char *s,int l) // pad (or clip!) string with up to `l` spaces
{
	char *r=s;
	while (l&&*r) ++r,--l;
	while (l--) *r++=' ';
	*r=0;
	return s;
}
int strint(const char *s,int i) // perform strchr(s,i) and sanitize the output
	{ const char *t=strchr(s,i); return t?t-s:-1; }

INLINE void keepwithin(int *i,int l,int h) { if (*i<l) *i=l; else if (*i>h) *i=h; }

int flag_q=0,flag_q_last,flag_q_next; // onscreen percentage counters
#define flag_q_init(i) flag_q_last=(i)+1
void flag_q_loop(int i) { if (!flag_q&&!(7&++flag_q_next)) fprintf(stderr,"%02d%%\x0D",100*i/flag_q_last); }
void flag_q_exit(int i) { if (!flag_q) fprintf(stderr,"%d.\n",i); }

// song file I/O ---------------------------------------------- //

char musical_[]="::::::";
char *musical(int n,int i) // note formatting, f.e. `n=57,i=1` into "A-4:01"
{
	if (n>0&&n<=(CHIPNSFX_SIZE-3))
	{
		--n;
		musical_[0]=notes1[n%12];
		musical_[1]=notes2[n%12];
		musical_[2]=itoradix(n/12);
	}
	else
	{
		if (n==CHIPNSFX_SIZE-2) n='=';
		//else if (n==CHIPNSFX_SIZE-1) n='?';
		else if (n==CHIPNSFX_SIZE-0) n='^';
		else n='.';
		musical_[0]=musical_[1]=musical_[2]=n;
		i=0; // special codes lack instruments
	}
	if (i)
		musical_[4]=itoradix(i/16),musical_[5]=itoradix(i%16);
	else
		musical_[4]=musical_[5]='.';
	return musical_;
}
INLINE int addnote(int n,int d) // transpose `n` by `d` and normalise
{
	if (n<1)
		return 0; // minimum!
	if (n>(CHIPNSFX_SIZE-0))
		return (CHIPNSFX_SIZE-0); // maximum!
	if (n>(CHIPNSFX_SIZE-4))
		return n; // SFX, "===", ???", "^^^"
	if ((n+=d)<1)
		return 1; // C-0...
	if (n>(CHIPNSFX_SIZE-4))
		return (CHIPNSFX_SIZE-4); // ...B-8
	return n;
}

void newsong(void) // destroys all song data and starts anew
{
	hertz=50; divisor=6; transpose=0; loopto=-1;
	strcpy(title1,"Song title"); strcpy(title2,"Description");
	MEMZERO(order); MEMZERO(score); MEMZERO(scrln); MEMZERO(instr);
	order[1][0][0]=scrln[0]=scrln[1]=scrln[2]=orders=1;
	order[2][0][0]=instrs=2;
	instr[1][0]=instr[1][1]=0xFF; strcpy(&instr[1][8],"Piano");
}

char flag_nn=0; // song transposition flag

unsigned char *loadgets(unsigned char *t) { return (!(t&&fgets(t,MAXSTRLEN,f)))?NULL:strcln(t); }
int fclosein(void) { if (f!=stdin) fclose(f); return 0; }
int loadsong(char *s) // destroys the old song and loads a new one; 0 is ERROR
{
	char *r; int i,j;
	if (!strcmp("-",s))
		f=stdin;
	else if (!*s||!(f=fopen(s,"r")))
		return 0;
	loadgets(tmp);
	if (strcmp((239==tmp[0]&&187==tmp[1]&&191==tmp[2])?&tmp[3]:tmp,chipnsfxid)) // skip unsigned UTF-8 BOM
		return fclosein();
	// header: text+vars
	newsong();
	loadgets(title1);
	loadgets(title2);
	loadgets(tmp); // skip the separator gap
	// "hertz/divisor +trasposition"
	if (!loadgets(tmp)) return fclosein();
	hertz=strtol(tmp,&r,10);
	divisor=strtol(++r,&r,10);
	transpose=strtol(r,NULL,10)+flag_nn;
	//if (!flag_q) printf("%s --- %s\n%d/%d Hz %+d Trans\nOrders:",title1,title2,hertz,divisor,transpose);
	// orders: "#00+00 #01+00 #02+12"
	for (i=0;;)
	{
		if (!loadgets(r=tmp)) return fclosein();
		if (*r!='#') break; // end of order list
		for (j=0;j<3;++j)
		{
			order[j][i][0]=strtol(++r,&r,16); // skip '#'
			order[j][i][1]=strtol(r,&r,10);
			++r; // skip space
		}
		if (*r)//=='*'
			loopto=i;
		//if (!flag_q) printf(" %02X",i);
		++i;
	}
	if ((orders=i)>255) return fclosein(); // overrun!
	// scores: "#00: A-4:01 ...:..", etc.
	//if (!flag_q) printf("\nScores:");
	for (;;)
	{
		if (!loadgets(r=tmp)) return fclosein();
		if (*r!='#') break; // end of score list
		i=strtol(++r,&r,16)&255; // check overrun?
		for (j=0;j<128;++j)
		{
			int n; while ((n=*r)&&n<33) ++r;
			if (!n) break; // end of this score
			if ((n=strint(notes1,n))>=0)
			{
				#if 1
				++r;
				#else
				if (*++r=='+')
				{
					++r;
					n=score[i][j-1][0]+12*(radixtoi(ucase(*r++)-4));
				}
				else
				#endif
				{
					if (*r++=='#')
						++n;
					n+=12*radixtoi(ucase(*r++))+1;
				}
				if (n<1||n>(CHIPNSFX_SIZE-3))
					n=CHIPNSFX_SIZE-3; // old C-B -> new C-9
				score[i][j][1]=strtol(++r,0,16);
			}
			else
			{
				n=*r=='^'?CHIPNSFX_SIZE-0:*r=='='?CHIPNSFX_SIZE-2:0;
				score[i][j][1]=0;
			}
			score[i][j][0]=n;
			while (*r>32) ++r;
		}
		scrln[i]=j;
		//if (!flag_q) printf(" %02X",i);
	}
	// instruments: ":02 FFFE 0000 141 Ding"
	//if (!flag_q) printf("\nInstrs:");
	for (instrs=0;;)
	{
		if (!loadgets(r=tmp)) return fclosein();
		if (*r!=':') break; // end of instrument list
		if ((i=strtol(++r,&r,16))>0&&i<255)
		{
			j=strtol(++r,&r,16);
			instr[i][0]=j>>8; instr[i][1]=j;
			j=strtol(++r,&r,16);
			instr[i][2]=j>>8; instr[i][3]=j;
			j=strtol(++r,&r,16);
			instr[i][4]=j>>8; instr[i][5]=j;
			if (*r)
				++r;
			strcpy(&instr[i][8],r);
			if (instrs<i)
				instrs=i;
			//if (!flag_q) printf(" %02X",i);
		}
	}
	if (++instrs>255) return fclosein(); // overrun!
	#if 0 // let's tolerate this; it can happen in unfinished songs
	for (i=0;i<256;++i) // check for unknown instruments in song
		for (j=0;j<128;++j)
			if (score[i][j][1]>instrs)
				score[i][j][1]=score[i][j][0]=0; // unknown? zero!
	#endif
	//if (!flag_q) printf(crlf);
	if ((char*)filepath!=s) strcpy(filepath,s);
	return fclosein(),1;
}
int savesong(char *s) // saves the current song onto a file; 0 is ERROR
{
	int i,j,k;
	/*if (!strcmp("-",s))
		f=stdout;
	else*/ if (!s||!(f=fopen(s,"w")))
		return 0;
	fprintf(f,"%s\n%s\n%s\n%s%i/%i %+03i\n",chipnsfxid,title1,title2,chipnsfxlf,hertz,divisor,transpose-flag_nn);
	for (i=0;i<orders;++i)
		fprintf(f,"#%02X%+03i #%02X%+03i #%02X%+03i%s\n",
			order[0][i][0],(signed char)order[0][i][1],
			order[1][i][0],(signed char)order[1][i][1],
			order[2][i][0],(signed char)order[2][i][1],
			i==loopto?" *":"");
	fputs(chipnsfxlf,f);
	for (i=0;i<256;++i)
	{
		k=0;
		for (j=0;j<orders;++j)
			k|=(order[0][j][0]==i)||(order[1][j][0]==i)||(order[2][j][0]==i);
		if (k) // skip unused scores!
		{
			k=scrln[i];
			fprintf(f,"#%02X",i);
			for (j=0;j<k;++j)
				fprintf(f," %s",musical(score[i][j][0],score[i][j][1]));
			fputs(crlf,f);
		}
	}
	fputs(chipnsfxlf,f);
	for (i=1;i<instrs;++i)
	{
		sprintf(tmp,":%02X %02X%02X %02X%02X %01X%02X %s",i,
			instr[i][0],instr[i][1],
			instr[i][2],instr[i][3],
			instr[i][4],instr[i][5],
			&instr[i][8]);
		j=strlen(tmp);
		while (tmp[--j]==' ') // shorten instrument name
			;
		tmp[++j]='\n';
		tmp[++j]=0;
		fputs(tmp,f);
	}
	fputs(chipnsfxlf,f);
	/*if (f!=stdout)*/ fclose(f);
	return 1;
}

// target output ---------------------------------------------- //

// the song undergoes several levels of optimisation to store
// as little data as possible: looking for loops and repetitions,
// storing just the instrument params that change between notes, etc

int flag_c=80,savechar; // maximum and current line widths
char *savecomma(void)
{
	if (savechar>=flag_c)
		return savechar=0,"\n  db ";
	else
		return savechar>0?",":" db ";
}

int save_hits[256],save_chan[256],save_tiny[256],savesizes,save_full[3]; // global playback values, used to detect repetitions of pattern orders
int play_pose,past_pose,play_size,past_size,save_size[256],play_flag[6],past_flag[6],save_flag[256][6]; // <0 unused, 0..255 single, 256 multiple
signed char save_pack[256],save_packscore[256][128],save_packflags[256][128]; int save_packorder[3][256]; // repetition tables; +n = length, -n = -offset

int saveplay(int n,int i) // play a note with an instrument; 0 means NO NOTE
{
	if ((n>0&&n<CHIPNSFX_SIZE-2)) // note or SFX?
	{
		if (i) // instrument?
		{
			if (play_flag[0]=instr[i][0]) // mute? ignore volume envelope
				play_flag[1]=instr[i][1];
			if (play_flag[2]=instr[i][2]) // clear? ignore noise envelope
				play_flag[3]=instr[i][3];
			if (n<(CHIPNSFX_SIZE-3)) // SFX: ignore
				play_flag[4]=(play_flag[5]=instr[i][5])?instr[i][4]:0; // force type 0 on empty SFX
		}
		else // portamento
		{
			if (n<(CHIPNSFX_SIZE-3)) // SFX: ignore
				play_flag[4]=1,play_flag[5]=0;
		}
		return 1; // valid
	}
	return 0; // neither!
}
void savenote(int q,int n,int d) // generate (or scan) instrument and note data, including delta transposition
{
	if (play_size)
	{
		if (past_size!=play_size)
		{
			past_size=play_size;
			if (q)
				savechar+=fprintf(f,"%s$6F+%i",savecomma(),play_size);
		}
		if (n>0&&n<CHIPNSFX_SIZE-2) // note or SFX?
		{
			if (!play_flag[4]||play_flag[5]) // ignore changes within a portamento!
			{
				if (past_flag[0]!=play_flag[0])
				{
					past_flag[0]=play_flag[0];
					if (q)
						savechar+=fprintf(f,"%s$00F8,$%02X",savecomma(),play_flag[0]);
				}
				if (((past_flag[1]!=play_flag[1])&&play_flag[0]))
				{
					past_flag[1]=play_flag[1];
					if (q)
						savechar+=fprintf(f,"%s$00FE,$%02X",savecomma(),play_flag[1]);
				}
				if (past_flag[2]!=play_flag[2])
				{
					past_flag[2]=play_flag[2];
					if (q)
						savechar+=fprintf(f,"%s$00FA,$%02X",savecomma(),play_flag[2]);
				}
				if (((past_flag[3]!=play_flag[3])&&play_flag[2]))
				{
					past_flag[3]=play_flag[3];
					if (q)
						savechar+=fprintf(f,"%s$00FF,$%02X",savecomma(),play_flag[3]);
				}
			}
			if (((past_flag[4]!=play_flag[4]||past_flag[5]!=play_flag[5])&&n<(CHIPNSFX_SIZE-3)))
			{
				past_flag[4]=play_flag[4],past_flag[5]=play_flag[5];
				if (q)
					savechar+=fprintf(f,"%s$00F%c,$%02X",savecomma(),'C'+play_flag[4],play_flag[5]);
			}
		}
		if (q)
		{
			if (n>(CHIPNSFX_SIZE-4))
				savechar+=fprintf(f,"%s$006%c",savecomma(),'F'+n-CHIPNSFX_SIZE); // SFX/gap/padding/rest
			else
			{
				n+=d; if (n>0&&n<=(CHIPNSFX_SIZE-4))
					savechar+=fprintf(f,"%s%i",savecomma(),n-1); // note
				else
					savechar+=fprintf(f,"%s$006F",savecomma()); // overflow = rest
			}
		}
	}
}
void savescore(int q,int j,int d) // generate (or scan) pattern data, including delta transposition
{
	int a,b,c,i,l;
	if ((i=0)>=(l=scrln[j]))
		return;
	if (q) savechar=0;
	play_size=a=0;
	while (i<l)
	{
		if (q&&((c=save_packscore[j][i])<0)) // end loop
		{
			savenote(q,a,d);
			savechar+=fprintf(f,"%s$00F4,0",savecomma());
			play_size=a=0;
			i-=c;
		}
		else
		{
			if (c=score[j][i][0])
			{
				savenote(q,a,d);
				a=c;
				if (q&&(c=save_packscore[j][i])>0) // set loop
				{
					savechar+=fprintf(f,"%s$00F4,%i",savecomma(),c);
					// force reset if required
					if ((c=save_packflags[j][i])&(1<<7))
						past_size=256;
					for (b=0;b<6;++b)
						if (c&(1<<b))
							past_flag[b]=256;
				}
				saveplay(a,score[j][i][1]);
				play_size=0;
			}
			play_size+=savesizes;
			++i;
		}
	}
	savenote(q,a,d); // don't peek
	if (q) fputs(crlf,f);
}
void savechaninit(void) // generate (or scan) setup
{
	MEMZERO(past_flag);
	MEMZERO(play_flag);
	past_flag[0]=play_flag[0]=255; // exception: initial volume is always max
	past_pose=play_pose=0;
	past_size=play_size=1;
}
#define SAVECHANDIRT(x,y,q) (x=((q)&&((x==y)||(x<0)))?y:256)
int savechanprev(int n) // which pattern precedes `n`? 256 if multiple!
{
	int j=-1,k;
	if (!(n&256))
		for (int c=0;c<3;++c)
			for (int i=0;i<orders;++i)
				if (order[c][i][0]==n)
				{
					if (i==0||i==loopto)
						return 256;
					if (order[c][i][1]!=order[c][i-1][1])
						return 256;
					if (j==(k=order[c][i-1][0])||j<0)
						j=k;
					else
						return 256;
				}
	return j;
}
int savechannext(int p) // which pattern follows `p`? 256 if multiple!
{
	int j=-1,k;
	if (!(p&256))
		for (int c=0;c<3;++c)
			for (int i=0;i<orders;++i)
				if (order[c][i][0]==p)
				{
					if (i==loopto-1||i==orders-1)
						return 256;
					if (order[c][i][1]!=order[c][i+1][1])
						return 256;
					if (j==(k=order[c][i+1][0])||j<0)
						j=k;
					else
						return 256;
				}
	return j;
}
void savechanexit(int q) // generate (or scan) end of song
{
	for (int i=0;i<256;++i)
		for (int c=0;c<3;++c)
		{
			int j=order[c][i][0],k;
			if (save_chan[j]==q&&save_hits[j]>1)
			{
				if (((k=savechanprev(j))&256)||(k!=order[c][i-1][0])||savechannext(k)!=j)
					fprintf(f,"%s%c%02x:\n",title0,'a'+q,j);
				past_size=save_size[j];
				MEMSAVE(past_flag,save_flag[j]);
				savescore(1,j,0);
				if (((k=savechannext(j))&256)||(k!=order[c][i+1][0])||savechanprev(k)!=j)
					fprintf(f," db $00F1\n");
				save_chan[j]=-1; // don't touch it ever again
			}
		}
}

void savepackorderz(int q,int c,int i,int l) // look for loops within orders, see below
{
	while (i<l)
	{
		int j,bestoffset=0,bestlength=0,bestsaving=0;
		for (j=i;++j<l;)
		{
			int k=0,m;
			while (((j+k)<l)&&(m=order[c][i+k][0])==order[c][j+k][0]&&order[c][i+k][1]==order[c][j+k][1]&&!(q&&save_pack[m]))
				++k;
			k-=k%(j-i); // align loop
			int saving=0;
			for (m=i;m<j;++m)
				saving+=save_tiny[order[c][m][0]]?1:3;
			saving=(saving*k/(j-i))-4;
			if ((bestsaving<saving)) // check whether we actually save any data
				bestsaving=saving,bestlength=k,bestoffset=j-i;
		}
		if (bestsaving)
		{
			save_packorder[c][i]=1+(bestlength/bestoffset);
			for (j=0;j<bestoffset;++j)
				save_pack[order[c][i++][0]]|=!q; // avoid using this pattern again
			save_packorder[c][i]=-bestlength;
			for (j=0;j<bestlength;++j)
				save_pack[order[c][i++][0]]|=!q; // ...if there's a later time!
		}
		else
			++i;
	}
}
void savepackorders(int q) // `q` is NONZERO if this is the second pass that must avoid clashing with the first
{
	for (int c=0;c<3;++c)
		if (loopto<0)
			savepackorderz(q,c,0,save_full[c]);
		else
			savepackorderz(q,c,0,loopto),savepackorderz(q,c,loopto,save_full[c]);

}
void savepackscores(int q) // `q` is NONZERO if this is the second pass that must avoid clashing with the first
{
	for (int n=0,l;n<256;++n)
		if ((l=scrln[n])>1&&!(q&&save_pack[n]))
		{
			past_size=save_size[n];
			MEMSAVE(past_flag,save_flag[n]);
			for (int i=0,j,k;i<l;)
			{
				int bestsaving=0,bestoffset,bestlength,best_size,best_flag[6];
				if (score[n][i][0])
				{
					j=i;
					while (++j<l)
					{
						k=0;
						while (((j+k)<l)&&score[n][i+k][0]==score[n][j+k][0]&&score[n][i+k][1]==score[n][j+k][1])
							++k;
						// align loop to a valid length
						k-=(k%(j-i));
						if (j+k<l)
							while (k>0&&!score[n][j+k][0])
								k-=(j-i);
						if (k>0)
						{
							// examine whether the loop is profitable
							int m,o,saving=0,temp_size=past_size,temp_flag[6];
							MEMSAVE(temp_flag,past_flag);
							MEMSAVE(play_flag,past_flag);
							play_size=0;
							for (m=i;m<j;++m)
							{
								if (saveplay(score[n][m][0],score[n][m][1]))
								{
									++saving; // note
									if (play_size&&(temp_size!=play_size))
										temp_size=play_size,++saving; // warning, size can be delayed!
									for (o=0;o<4;++o)
										if (temp_flag[o]!=play_flag[o])
											temp_flag[o]=play_flag[o],saving+=2; // flag
									if (temp_flag[4]!=play_flag[4]||temp_flag[5]!=play_flag[5])
										temp_flag[4]=play_flag[4],temp_flag[5]=play_flag[5],saving+=2; // 00FC/00FD
									play_size=0;
								}
								play_size+=savesizes;
							}
							if (temp_size!=play_size)
								++saving; // delayed size
							if (past_size!=play_size)
								--saving; // current size
							for (o=0;o<4;++o)
								if (past_flag[o]!=play_flag[o])
									saving-=2; // flag
							if (past_flag[4]!=play_flag[4]||past_flag[5]!=play_flag[5])
								saving-=2; // 00FC/00FD
							saving=(saving*k/(j-i))-4;
							if (saving>bestsaving)
								bestsaving=saving,bestlength=k,bestoffset=j-i,best_size=play_size,MEMSAVE(best_flag,play_flag);
						}
					}
				}
				if (bestsaving)
				{
					save_pack[n]|=!q; // avoid using this pattern again
					// store which flags must be reset
					k=(best_size!=past_size&&past_size>=0)?(1<<7):0;
					for (j=0;j<6;++j)
						if (best_flag[j]!=past_flag[j]&&past_flag[j]>=0)
							k|=1<<j;
					save_packflags[n][i]=k;
					save_packscore[n][i]=1+(bestlength/bestoffset);
					i+=bestoffset;
					save_packscore[n][i]=-bestlength;
					i+=bestlength;
					past_size=best_size;
					MEMSAVE(past_flag,best_flag);
				}
				else
				{
					if (saveplay(score[n][i][0],score[n][i][1]))
						MEMSAVE(past_flag,play_flag),past_size=0;
					past_size+=savesizes;
					++i;
				}
			}
		}
}

char flag_a=0,flag_b=0,flag_bb[3]={0,1,2},flag_ll=0,flag_r=0,flag_t=0;

void savesonginit(void) // insert dummy padding
{
	int a,b,c,i,j,k,l;
	for (i=0;i<256;++i)
		if (k=scrln[i])
		{
			if (!score[i][0][0])
				score[i][0][0]=CHIPNSFX_SIZE-1; // empty beginnings require one dummy note
			do
			{
				l=a=b=0,c=j=1;
				while (j<=k)
				{
					if (score[i][j][0]||j==k)
					{
						if ((b==c)&&(c*2==a)&&(a+b+c==j))
							++l,score[i][j-b-a][0]=CHIPNSFX_SIZE-1; // [aabc... -> [aAbc...
						else if ((c==a)&&(a*2==b))
							++l,score[i][j-b][0]=CHIPNSFX_SIZE-1,b=c; // ..abbc.. -> ..abBc..
						else if ((j==k)&&(a==b)&&(b*2==c))
							++l,score[i][j-a][0]=CHIPNSFX_SIZE-1,c=b; // ...abcc] -> ...abcC]
						a=b,b=c,c=0;
					}
					++c,++j;
				}
			}
			while (l); // iteration is important!
		}
}
void savesongexit(void) // remove dummy padding
{
	for (int i=0;i<256;++i)
		for (int j=0;j<scrln[i];++j)
			if (score[i][j][0]==CHIPNSFX_SIZE-1)
				score[i][j][0]=0;
}

int savedata(char *s) // exports the current song as an INCLUDE file; 0 is ERROR
{
	int a,b,c,i=256,j,k;
	savesonginit();
	// -t: check whether timings can be abridged
	savesizes=1;
	if (flag_t&&divisor>1) // ensure i=256 beforehand
	{
		for (i=0;i<256;++i)
			if (((k=scrln[i])*divisor)>128)
			{
				// perform a deeper analysis
				j=a=0;
				while (j<k)
				{
					if (score[i][j++][0])
					{
						if (a>128) // overflow?
							break;
						a=0; // new note, reset
					}
					a+=divisor;
				}
				if (a>128)
					break;
			}
		if (i>=256)
			savesizes=divisor; // timings can be simplified
	}
	if ((flag_t>1&&i<256)||!s||!(f=(strcmp(s,"-")?fopen(s,flag_a?"a":"w"):stdout)))
		return savesongexit(),0;
	fprintf(f,"; " MY_FILE " %i Hz\n; %s\n; %s\n; %s",hertz,title1,title2,chipnsfxlf);
	if (flag_ll)
		fprintf(f,"%sa_z:\n dw %sa-$-2\n dw %sb-$-2\n dw %sc-$-2\n",title0,title0,title0,title0);
	// simplify loops: ABC.LOOP.XYZXYZXYZ -> ABC.LOOP.XYZ
	for (c=0;c<3;++c)
		if (loopto>=0)
		{
			k=0;
			while (++k<orders-loopto)
			{
				if (!((orders-loopto)%k))
				{
					for (i=loopto+k;i<orders;++i)
						if ((order[c][i][0]!=order[c][i-k][0])||(order[c][i][1]!=order[c][i-k][1]))
							break;
					if (i>=orders)
						break;
				}
			}
			save_full[c]=loopto+k;
		}
		else
			save_full[c]=orders;
	// check for empty tracks
	for (c=0;c<3;++c)
	{
		for (i=0;i<save_full[c];++i)
		{
			j=order[c][i][0];
			for (k=0;k<scrln[j];++k)
				if ((a=score[j][k][0])&&(a<(CHIPNSFX_SIZE-2)))
					break;
			if (k<scrln[j])
				break;
		}
		if (i>=save_full[c])
			save_full[c]=0;
	}
	// perform preliminary playback: build flags and statistics
	MEMFULL(save_flag);
	MEMFULL(save_size);
	MEMFULL(save_chan);
	MEMZERO(save_hits);
	MEMZERO(save_tiny);
	MEMZERO(save_pack);
	MEMZERO(save_packscore);
	MEMZERO(save_packorder);
	MEMZERO(save_packflags);
	for (c=0;c<3;++c)
	{
		savechaninit();
		for (i=0;i<save_full[c];++i)
		{
			j=order[c][i][0];
			SAVECHANDIRT(save_size[j],play_size,1);
			for (a=0;a<6;++a)
				SAVECHANDIRT(save_flag[j][a],play_flag[a],1);
			savescore(0,j,0); // update flags
		}
		if (loopto>=0) // ensure consistency around the looping extremes
		{
			for (i=loopto;i<save_full[c];++i) // special case: look for tracks that loop before playing anything
			{
				j=order[c][i][0];
				SAVECHANDIRT(save_size[j],play_size,1);
				for (a=0;a<6;++a)
					SAVECHANDIRT(save_flag[j][a],play_flag[a],1);
				savescore(0,j,0); // update flags again
			}
		}
	}
	// look for empty or almost empty patterns
	for (i=0;i<256;++i)
	{
		j=scrln[i]-1;
		while (j>0&&(!(k=score[i][j][0]))) // ..||k>=(CHIPNSFX_SIZE-0)))
			--j;
		save_tiny[i]=(j<=0); // pattern has <2 notes
	}
	// pack all loops in the song and the patterns
	switch (flag_r)
	{
		case +0: // --
			savepackscores(0);
			savepackorders(1);
			break;
		case +2: // r-
			savepackorders(0);
			savepackscores(1);
			break;
		case +3: // rR
			savepackorders(0);
			savepackscores(0);
			break;
		//case +1: // -R
	}
	// assign primary, secondary and tertiary pattern categories
	for (c=0;c<3;++c)
		for (i=0;i<save_full[c];)
		{
			if ((j=save_packorder[c][i])<0)
				i-=j; // compressed repetitions become simplified
			else
			{
				++save_hits[j=order[c][i][0]]; // secondary and tertiary patterns have >1 here
				if ((k=save_chan[j])!=c)
					save_chan[j]=k<0?c:3; // tertiary patterns are tagged "3"
				++i;
			}
		}
	for (i=0;i<256;++i)
		if (save_tiny[i]&&save_hits[i]>1)
			if ((savechanprev(i)&256)&&(savechannext(i)&256)) // not in a chain!
				save_hits[i]=1; // lone patterns with just a note become primary
	// export the song's three channels
	for (b=0;b<3;++b)
	{
		c=flag_bb[b];
		savechaninit();
		// channel header
		fprintf(f,"%s%c:\n",title0,'a'+c);
		if (savesizes<divisor&&save_full[c])
			fprintf(f," db $00F5,%i\n",divisor);
		// export the channel's primary patterns
		for (i=0;i<save_full[c];)
		{
			if ((j=save_packorder[c][i])<0)
			{
				fprintf(f," db $00F4,0\n");
				i-=j;
			}
			else
			{
				play_pose=transpose+(signed char)order[c][i][1];
				if (i==loopto)
				{
					fprintf(f,"%s%c_l:\n",title0,'a'+c);
					k=save_full[c];
					while (--k>i&&save_tiny[order[c][k][0]])
						;
					j=transpose+(signed char)order[c][k][1];
					if (past_pose!=j)
						past_pose=256;
					past_size=save_size[j=order[c][i][0]];
					MEMSAVE(past_flag,save_flag[j]);
				}
				if ((j=save_packorder[c][i])>0)
				{
					fprintf(f," db $00F4,%i\n",j);
					k=i;
					while (save_packorder[c][++k]>=0)
						;
					while (--k>i&&save_tiny[order[c][k][0]])
						;
					j=transpose+(signed char)order[c][k][1];
					if (past_pose!=j)
						past_pose=256;
					past_size=save_size[j=order[c][i][0]];
					MEMSAVE(past_flag,save_flag[j]);
				}
				j=order[c][i][0];
				if (past_pose!=play_pose)
					if (past_pose==256||!save_tiny[j]||save_hits[j]>1)
						fprintf(f," db $00F6,%+i\n",past_pose=play_pose);
				if (save_hits[j]>1)
				{
					k=savechanprev(j);
					if ((k&256)||k!=order[c][i-1][0]||savechannext(k)!=j)
					{
						k=save_chan[j];
						a=!(flag_b&(1<<(c+(k<3?0:3))));
						fprintf(f," db $00F%c\n  d%c %s%c%02x-$-%c\n",a?'2':'3',a?'w':'b',title0,'a'+k,j,'1'+a);
					}
					savescore(0,j,0); // update flags
				}
				else
					savescore(1,j,play_pose-past_pose);
				++i;
			}
		}
		// channel footer
		if (loopto<0||!i)
			savechar+=fprintf(f," db $00F0\n");
		else
			savechar+=fprintf(f," db $00F2\n  dw %s%c_l-$-2\n",title0,'a'+c);
		// export the channel's secondary patterns
		savechanexit(c);
	}
	// export the song's tertiary patterns
	savechanexit(3);
	fprintf(f,"; %s",chipnsfxlf);
	if (f!=stdout)
		fclose(f);
	return savesongexit(),1;
}

// song playback, see EXPORT and TRACKER ---------------------- //

#define SND_QUANTUM 441
#define SND_QUALITY (SND_QUANTUM*100) // 100 frames/second
#define SND_FIXED 8 // extra bits in comparison to the YM3B standard, 2 MHz
const int snd_freqs[]={(0x3BB907),(0x385EEB),(0x3534F9),(0x32387D),(0x2F66E9),(0x2CBDD4),
	(0x2A3AF9),(0x27DC33),(0x259F7C),(0x2382E9),(0x2184AD),(0x1FA314)}; // C-0..B#0 @ 4<<SND_FIXED MHz
const unsigned char snd_ampls[16]={0,1,1,1,2,3,4,5,8,11,15,21,30,43,60,85}; // = (255/3)*(2^((n-15)/2))

int snd_notefreq(int i) // calculate wavelength (not really frequency) of note `i`
{
	return (i<0||i>(CHIPNSFX_SIZE-4))?0:(((snd_freqs[i%12])>>(i/12))+1)/2;
}

int snd_playing=0,snd_looping=0,snd_order=0,snd_score=0,snd_testn,snd_testd,snd_testi=0,
	snd_ticks=0,snd_count=0,snd_noise=0,snd_noises,snd_noiser=1,snd_noisez=0,snd_noised=0;
int snd_bool[3]={1,1,1},snd_note[3],snd_freq[3],snd_freq0[3],snd_ampl[3],snd_ampl0[3],snd_ampld[3],
	snd_brake[3],snd_noisy[3],snd_nois0[3],snd_noisd[3],snd_sfx[3],snd_sfx0[3],snd_sfxd[3],snd_freqz[3];

void snd_stop(void)
{
	snd_playing=0;
	snd_ampl[0]=snd_ampl[1]=snd_ampl[2]=0;
}
void snd_play(int m) // 0=normal,!0=pattern
{
	if (orders&&scrln[order[0][0][0]]&&scrln[order[1][0][0]]&&scrln[order[2][0][0]])
	{
		int c;
		for (snd_noisez=snd_noised=c=0;c<3;++c)
		{
			snd_freq[c]=snd_ampl[c]=snd_noisy[c]=snd_sfx[c]=
				snd_freq0[c]=snd_freqz[c]=snd_ampld[c]=
				snd_brake[c]=snd_nois0[c]=snd_noisd[c]=
				snd_sfx0[c]=snd_sfxd[c]=0;
			snd_note[c]=snd_ampl0[c]=255;
		}
		snd_count=snd_ticks=snd_noise=0;
		snd_looping=m;
		snd_playing=1;
	}
}

int flag_n=0,flag_zz=0;
int snd_playnote(int c,int n,int d,int i) // channel, note, transposition, instrument; 1 if it extends the current note, 0 if new
{
	if (n>0)
	{
		if (n==(CHIPNSFX_SIZE-2))
			return snd_brake[c]=!snd_brake[c],1; // brake
		else if (n<(CHIPNSFX_SIZE-0))
		{
			if (n>(CHIPNSFX_SIZE-4))
			{
				snd_note[c]=(CHIPNSFX_SIZE-4);
				n=0; // sfx
			}
			else
			{
				snd_note[c]=(n+=transpose+d-1);
				n=snd_notefreq(n); // note
			}
			snd_sfx[c]=0;
			if (i) // instrument
			{
				snd_freq0[c]=n;
				snd_ampl[c]=snd_ampl0[c]=instr[i][0];
				snd_ampld[c]=instr[i][1];
				snd_brake[c]=0;
				if (n=snd_noisy[c]=snd_nois0[c]=instr[i][2])
				{
					snd_noisez=n;
					if ((n=snd_noisd[c]=instr[i][3])!=0x80)
						snd_noised=n;
				}
				snd_sfx0[c]=instr[i][4];
				snd_sfxd[c]=instr[i][5];
			}
			else // portamento
				return snd_sfxd[c]=n,snd_sfx0[c]=-1;
		}
		else
			snd_ampl[c]=0; // rest
		return 0;
	}
	return 1;
}
void snd_playchan(int c) // update channel `c`
{
	int i,j;
	if (snd_ampl[c]) // ignore mute channels
	{
		if (!snd_freq0[c])
			snd_freq[c]=0; // allow pure noise
		if (!snd_brake[c])
		{
			i=(unsigned char)snd_ampld[c];
			if (i>=0x60&&i<0xA0)
			{
				if (j=(signed char)(i<<3)) // zero-depth: 0x60 static and 0x80 dynamic
				{
					if (i&0x80) // timbre or tremolo
						if (snd_ampl[c]!=snd_ampl0[c])
							j=0;
					snd_ampl[c]=snd_ampl0[c]+j;
					keepwithin(&snd_ampl[c],0,255);
				}
			}
			else
			{
				// basic amplitude envelope
				if ((snd_ampl[c]+=(signed char)i)<0)
					snd_ampl[c]=0;
				else if (snd_ampl[c]>255)
					snd_ampl[c]=255;
			}
		}
		if (snd_noisd[c]==0x80) // special case: instant noise
			snd_noisy[c]=0;
		if (snd_freq0[c]&&snd_sfxd[c])
		{
			if (!snd_sfx0[c]) // arpeggio
			{
				if ((snd_sfx[c]+=0x40)>((i=(snd_sfxd[c]&0x0F))?0x80:0x40))
					snd_sfx[c]=0;
				if (snd_sfx[c]&0x80)
					;
				else if (snd_sfx[c]&0x40)
					i=snd_sfxd[c]>>4;
				else
					i=0;
				// special cases
				if (i==13)
					i=24; // D=24
				else if (i>13)
				{
					i=(i&1)?-12:-24; // F=-12, E=-24
					snd_sfx[c]=0x40; // lock arpeggio
				}
				snd_freq0[c]=snd_notefreq(snd_note[c]+i);
			}
			else if (snd_sfx0[c]>0) // vibrato et al.
			{
				j=snd_freq0[c]>>(8+1+SND_FIXED); // proportional v.
				if (!(i=(snd_sfxd[c]&7))) // slow glissando?
					i=1;
				i=(j+1)<<(i-1);
				if (snd_sfxd[c]&8)
					i=-i; // signed vibrato
				if (j=(snd_sfxd[c]&0xF0))
				{
					if (((snd_sfx[c]+=16)&0xF0)==j)
					{
						if (j=(snd_sfxd[c]&0x07)) // vibrato?
						{
							if (2&(snd_sfx[c]=((snd_sfx[c]+1)&3))) // +I -I -I +I ...
							//if (!(snd_sfx[c]=((snd_sfx[c]+1)&1))) // ultra soft vibrato
								i=-i;
						}
						else
							snd_sfx[c]=0; // slow glissando
						snd_freq0[c]-=i<<(1+SND_FIXED);
					}
				}
				else if (snd_sfxd[c]) // glissando
				{
					snd_freq0[c]-=i<<(1+SND_FIXED);
					if (snd_sfxd[c]==8) // detect flanging?
						snd_sfxd[c]=0; // handle flanging
				}
				if (snd_freq0[c]&(1<<(8+6+SND_FIXED)))
					snd_freq0[c]=0; // safety!
			}
			else // portamento
				snd_freq0[c]=(snd_sfxd[c]+snd_freq0[c]+(snd_sfxd[c]>snd_freq0[c]))/2;
		}
	}
}

// read one frame of score
int snd_frame(int h) // returns 0 if SONG OVER
{
	int c,i,q=3;
	if (snd_count<=0)
	{
		if (snd_noisez)
		{
			if ((i=snd_noised)==0x7F) // noisy crunch
				snd_noisez=256-snd_noisez;
			else
			{
				snd_noisez+=(signed char)i;
				keepwithin(&snd_noisez,1,255);
			}
		}
		if (snd_ticks<=0)
		{
			if (snd_order>=orders)
				if (snd_playing&&(loopto<0||snd_looping||!scrln[order[0][snd_order=loopto][0]]||(h&&!--flag_n)))
					snd_stop();
			if (snd_playing)
			{
				for (q=c=0;c<3;++c) // walk thru the scores
				{
					i=order[c][snd_order][0];
					if (snd_playnote(c,score[i][snd_score][0],(signed char)order[c][snd_order][1],score[i][snd_score][1]))
						snd_playchan(c);
				}
				if ((++snd_score)>=scrln[i])
				{
					snd_score=0;
					if (!snd_looping)
						++snd_order;
				}
				snd_ticks+=divisor;
			}
		}
		for (c=0;c<q;++c)
			snd_playchan(c);
		--snd_ticks;
		snd_count+=100;
		for (c=0;c<3;++c)
			snd_freqz[c]=(snd_freq0[c]*(2+(flag_zz&1)))/2; // wave shape
	}
	snd_count-=hertz;
	return snd_playing;
}

// play one frame of sound
int SND_DEPTH=0,SND_DEPTHSTEP,SND_DEPTHLEFT,SND_DEPTHMASK; // oversampling
int snd_waves(unsigned char *t) // returns frame size in bytes
{
	char a[3],b[3]; // small cache to reduce calculations: a = final amplitude, b = note is clear
	for (int c=0;c<3;++c)
		a[c]=(snd_ampl[c]&&(snd_bool[c]||!snd_playing))?
			#ifdef EXPERIMENTAL //
			snd_ampld[c]==0X60?-2: // zero-depth static: whole hard envelope
			snd_ampld[c]==0X80?-1: // zero-depth dynamic: half hard envelope
			#endif // EXPERIMENTAL
			snd_ampls[snd_ampl[c]>>4]:0,b[c]=!snd_noisy[c];
	int l,m,n=snd_noisez; if (!(n>>=2)) n=1; // 2 for CPC, 3 for ST
	n<<=1+SND_FIXED;
	for (int j=0;j<SND_QUANTUM;++j)
	{
		int z=SND_DEPTHLEFT;
		for (int i=1<<SND_DEPTH;i>0;--i)
		{
			static int w=0; w%=SND_QUALITY; // self-adjusting step
			int k=(w+=SND_DEPTHSTEP)/SND_QUALITY;
			for (int c=0;c<3;++c)
				if (a[c]) // channel is active? update it!
				{
					snd_freq[c]=(m=(l=snd_freq0[c]<<1)?(snd_freq[c]+k)%l:0);
					if (snd_noises|b[c]) // output is high?
					#ifdef EXPERIMENTAL //
						if (a[c]<0&&l) // hard envelope?
						{
							static unsigned int o=0; // 0..snd_freq0-1
							static unsigned char p=0; // 0..15 (ampli)
							z+=snd_ampls[(p+=(o+=k)/l)&15]; o%=l;
						}
						else
					#endif // EXPERIMENTAL
						if (!l||m<snd_freqz[c])
							z+=a[c];
				}
			if ((snd_noise+=k)>=n) // update noise generator
			{
				if (snd_noiser&1) snd_noiser+=0x48000; // LFSR x2
				snd_noises=(snd_noises+(snd_noiser>>=1))&1;
				snd_noise%=n;
			}
		}
		*t++=z>>SND_DEPTH;
		SND_DEPTHLEFT=z&SND_DEPTHMASK;
	}
	return SND_QUANTUM;
}

// export output ---------------------------------------------- //

int fputiiii(int x) { fputc(x,f); fputc(x>>8,f); fputc(x>>16,f); return fputc(x>>24,f); }

int savewave(char *s,int i) // exports the current song as a WAVE file; 0 is ERROR
{
	int l=0;
	if (!s||!(f=fopen(s,"wb")))
		return 0;
	fwrite("RIFFFFFFWAVEfmt \x10\x00\x00\x00\x01\x00\x01\x00",1,24,f);
	fputiiii(SND_QUALITY);
	fputiiii(SND_QUALITY);
	fwrite("\x01\x00\x08\x00""dataaaaa",1,12,f);
	if (i<0)
		i=0; // beware of non-looping songs!
	snd_order=i;
	//loopto=-1; // forbid looping
	snd_play(0);
	flag_q_init(orders);
	for (;;)
	{
		flag_q_loop(snd_order);
		if (!snd_frame(!0))
			break;
		l+=fwrite(tmp,1,snd_waves(tmp),f);
	}
	fseek(f,40,SEEK_SET);
	fputiiii(l);
	l+=36;
	fseek(f,4,SEEK_SET);
	fputiiii(l);
	fclose(f);
	flag_q_exit(l+8);
	return 1;
}

int saveym3b(char *s,int i) // exports the current song as an YM3B file; 0 is ERROR
{
	int l=0,o=0;
	// measure song in advance
	if (i<0)
		i=0; // beware of non-looping songs!
	snd_order=i;
	while (i<orders)
	{
		if (i==loopto)
			o=l;
		l+=scrln[order[0][i++][0]];
	}
	l*=divisor;
	o*=divisor;
	if (hertz<50)
	{
		l<<=1;
		o<<=1;
	}
	else if (hertz>50)
	{
		l>>=1;
		o>>=1;
	}
	char *y=(char*)malloc(l*14);
	if (!y)
		return 0;
	if (!s||!(f=fopen(s,"wb")))
	{
		free(y);
		return 0;
	}
	fwrite(o?"YM3b":"YM3!",1,4,f);
	//loopto=-1; // forbid looping
	snd_play(i=0);
	flag_q_init(l);
	while (i<l)
	{
		flag_q_loop(i);
		snd_frame(!0); snd_frame(!0); // reduce 100 Hz to 50 Hz
		int j;
		y[i     ]=j=(snd_freq0[0]>>SND_FIXED); // YM3B @ 2 MHz
		y[i+l   ]=j>>8;
		y[i+l* 2]=j=(snd_freq0[1]>>SND_FIXED);
		y[i+l* 3]=j>>8;
		y[i+l* 4]=j=(snd_freq0[2]>>SND_FIXED);
		y[i+l* 5]=j>>8;
		y[i+l* 6]=snd_noisez>>3; // 0..31
		y[i+l* 7]=(snd_noisy[0]?0:8)|(snd_noisy[1]?0:16)|(snd_noisy[2]?0:32)|((snd_freq0[0]&&snd_ampl[0])?0:1)|((snd_freq0[1]&&snd_ampl[1])?0:2)|((snd_freq0[2]&&snd_ampl[2])?0:4);
		y[i+l* 8]=snd_ampl[0]>>4; // 0..15
		y[i+l* 9]=snd_ampl[1]>>4;
		y[i+l*10]=snd_ampl[2]>>4;
		y[i+l*11]=y[i+l*12]=0;
		y[i+l*13]=-1; // IDLE
		++i;
	}
	fwrite(y,1,l*=14,f);
	if (o)
		l+=4,fputiiii(o);
	free(y);
	fclose(f);
	flag_q_exit(l+4);
	return 1;
}

// Win32/SDL2 services ---------------------------------------- //

#ifdef SDL2_DOUBLE_QUEUE
#define SDL2_FLAG_EXCL 1
#else
#define SDL2_FLAG_EXCL 0
#endif
char flag_m=0,flag_p=0,flag_pp=0,flag_uu=0,flag_xx=0,flag_excl=SDL2_FLAG_EXCL; // these command line options are useless without the tracker

#ifndef DEBUG

// different behaviors between Win32 and POSIX

#ifdef _WIN32
HANDLE gui_find_d; WIN32_FIND_DATA gui_find_e; int gui_win32units; BYTE gui_find_z[1<<9];
#define PATH_SEPARATOR '\\'
#define gui_findname ((char*)gui_find_e.cFileName)
#define gui_findsize (gui_find_e.nFileSizeLow)
int gui_findinit(char *p) { sprintf(gui_find_z,"%s*",p); return !!(gui_find_d=FindFirstFile(gui_find_z,&gui_find_e)); } // "Z:\ABC\DEF\*"
int gui_findnext(void) { return FindNextFile(gui_find_d,&gui_find_e); }
int gui_findlast(void) { return gui_find_d&&FindClose(gui_find_d); }
int gui_findtest(const char *x)
{
	if (!x)
		return (gui_find_e.dwFileAttributes&0x16)==0x10&&strcmp(gui_findname,"."); // is a visible directory
	if (gui_find_e.dwFileAttributes&0x16)
		return 0;
	int i=strlen(gui_findname),j=strlen(x);
	return i>j&&!strcasecmp(&gui_findname[i-j],x); // is a file and extensions match
}
#define utf8_display(s) (s)
#else
DIR *gui_find_d; struct dirent *gui_find_e; unsigned char *gui_posixpath,gui_find_z[1<<9];
#define PATH_SEPARATOR '/'
#define gui_findname (gui_find_e->d_name)
int gui_findsize;
int gui_findinit(char *p) { gui_posixpath=p; return !!((gui_find_d=opendir(p))&&(gui_find_e=readdir(gui_find_d))); } // "/ABC/DEF/"
int gui_findnext(void) { return !!(gui_find_e=readdir(gui_find_d)); }
int gui_findlast(void) { return gui_find_d&&closedir(gui_find_d); }
int gui_findtest(const char *x)
{
	gui_findsize=0;
	if (*gui_findname=='.'&&strcmp(gui_findname,"..")) return 0; // is invisible
	sprintf(gui_find_z,"%s%s",gui_posixpath,gui_findname);
	struct stat a; if (stat(gui_find_z,&a)<0) return 0;
	gui_findsize=a.st_size;
	if (S_ISDIR(a.st_mode))
		return !x; // only true if `x` is NULL
	if (!x)
		return 0; // files must match extension
	int i=strlen(gui_findname),j=strlen(x);
	return i>j&&!strcasecmp(&gui_findname[i-j],x); // is a file and extensions match
}
char utf8_buffer[1<<9];
char *utf8_display(char *s) // prepare a UTF-8 string for display; returns a temporary buffer with 8-bit ANSI codes
{
	int i,j,k; char *t=utf8_buffer;
	while (i=(INT8)*s++)
	{
		if (i<0)
		{
			if (i>=-64)
			{
				j=0; while ((k=(INT8)*s)<-64) j=j*64+k+128,++s;
				if (i<-32) i=((i+64)<<6)+j;
				else if (i<-16) i=((i+32)<<12)+j;
				else i=((i+16)<<18)+j;
			}
			if (i&-256) // i.e. it's either <0 or >=256
				i='?'; // won't fit on 8 bits :-(
		}
		*t++=i;
	}
	return *t=0,utf8_buffer;
}
#endif

// shared constants and definitions

unsigned char gui_buffer[MAXSTRLEN],gui_string[1<<9]; int key_code,key_char,key_flag=0,mouse_px,mouse_py;
void gui_redraw(int k); // see below

int activepanel=0,activeoctave=4,activeparameter=0; // UI focus and cursors
int activescore=0,activescoreitem=0,activeinstr=1,activeinstritem=0,activeorder=0,activeorderitem=0;
int selectscore=0,selectscoreitem=0,selectorder=0,selectorderitem=0; // to use with SHIFT+cursors

void snd_testz(void) // play single instrument if required
{
	if (snd_testi&&snd_count<=0) // must happen after SND_FRAME!
	{
		if (!snd_playing)
			snd_playnote(1,snd_testn,snd_testd,snd_testi);
		snd_testi=0;
	}
}

#ifdef SDL2
unsigned short ico16b[32*32] = {
	#include "chipnsfx.h"
	};

int gui_weight=1;

#define GUI_WIDTH 80
#define GUI_HEIGHT 40

#define GUI_ESCAPE SDL_SCANCODE_ESCAPE
#define GUI_F1 SDL_SCANCODE_F1
#define GUI_F2 SDL_SCANCODE_F2
#define GUI_F3 SDL_SCANCODE_F3
#define GUI_F4 SDL_SCANCODE_F4
#define GUI_F5 SDL_SCANCODE_F5
#define GUI_F6 SDL_SCANCODE_F6
#define GUI_F7 SDL_SCANCODE_F7
#define GUI_F8 SDL_SCANCODE_F8
#define GUI_F9 SDL_SCANCODE_F9
#define GUI_F10 SDL_SCANCODE_F10
#define GUI_F11 SDL_SCANCODE_F11
#define GUI_F12 SDL_SCANCODE_F12

#define GUI_HOME SDL_SCANCODE_HOME
#define GUI_END SDL_SCANCODE_END
#define GUI_PRIOR SDL_SCANCODE_PAGEUP
#define GUI_NEXT SDL_SCANCODE_PAGEDOWN
#define GUI_UP SDL_SCANCODE_UP
#define GUI_DOWN SDL_SCANCODE_DOWN
#define GUI_LEFT SDL_SCANCODE_LEFT
#define GUI_RIGHT SDL_SCANCODE_RIGHT
#define GUI_INSERT SDL_SCANCODE_INSERT
#define GUI_DELETE SDL_SCANCODE_DELETE
#define GUI_UNDO SDL_SCANCODE_BACKSPACE
#define GUI_MOUSE 32767 // fictional
#define GUI_TAB SDL_SCANCODE_TAB
#define GUI_ENTER SDL_SCANCODE_RETURN
#define GUI_SHIFT (key_flag&KMOD_SHIFT)
#define GUI_CONTROL (key_flag&KMOD_CTRL)
#define GUI_SHUTDOWN (key_code==GUI_ESCAPE||key_code==GUI_F10)

#include "cpcec-a7.h" // onscreen_chrs
BYTE onscreen_chrz[length(onscreen_chrs)];
#define GUI_PIXELS 12

DWORD gui_colour_rgb4[16]={
	0x000000,0x000080,0x800000,0x800080,0x008000,0x008080,0x808000,0xC0C0C0,
	0x808080,0x0000FF,0xFF0000,0xFF00FF,0x00FF00,0x00FFFF,0xFFFF00,0xFFFFFF};

SDL_Window *hwnd=NULL; SDL_Surface *hdib=NULL;
int gui_posx,gui_posy,gui_fore,gui_back,gui_dirt;
SDL_Surface *hico=NULL; int hsnd=0;
#define SND_FRAMES 32 // 100Hz packets

int gui_init(void) // sets interface and sound up. 0 is ERROR
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_EVENTS|SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)<0)
		return 0;
	if (!(hwnd=SDL_CreateWindow("",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,GUI_WIDTH*8,GUI_HEIGHT*GUI_PIXELS,0)))
		return SDL_Quit(),0;
	hdib=SDL_CreateRGBSurface(0,GUI_WIDTH*8,GUI_HEIGHT*GUI_PIXELS,32,0x00FF0000,0x0000FF00,0x000000FF,0); // = DWORD 0x00RRGGBB
	if (hdib->pitch!=GUI_WIDTH*32) // can this ever happen?
		return SDL_DestroyWindow(hwnd),SDL_Quit(),0;
	SDL_StartTextInput();
	if (flag_m)
		hsnd=0;
	else
	{
		SDL_AudioSpec spec; SDL_zero(spec);
		spec.channels=1; spec.freq=SND_QUALITY; spec.format=AUDIO_S16; // SDL2 rejects AUDIO_U8 in some distros!
		spec.samples=SND_QUANTUM*0; // 0 = SDL2 default; users may try 1, 2, 3...
		if (hsnd=SDL_OpenAudioDevice(NULL,0,&spec,NULL,0))
			SDL_PauseAudioDevice(hsnd,0);
	}
	hico=SDL_CreateRGBSurfaceFrom(ico16b,32,32,16,32*2,0xF00,0xF0,0xF,0xF000); // ARGB4444
	SDL_SetWindowIcon(hwnd,hico);
	return 1;
}
void gui_exit(void) // shuts interface and sound down
{
	#if 0 // SDL_Quit cleans everything up
	if (hsnd)
		SDL_ClearQueuedAudio(hsnd),SDL_CloseAudioDevice(hsnd);
	SDL_StopTextInput();
	SDL_DestroyWindow(hwnd);
	SDL_FreeSurface(hdib);
	SDL_FreeSurface(hico);
	#endif
	SDL_Quit();
}
void gui_title(char *s) // sets window title
{
	if (/*!s||*/!*s)
		SDL_SetWindowTitle(hwnd,MY_NAME);
	else
	{
		/*char *r=strrchr(s,PATH_SEPARATOR); if (!r) r=s; else ++r;*/ // filename only
		sprintf(gui_buffer,"%s - " MY_NAME,/*r*/s); SDL_SetWindowTitle(hwnd,gui_buffer);
	}
}
void gui_colour(unsigned char i) // sets colour attribute, low nibble is paper, high nibble is pen
{
	gui_back=gui_colour_rgb4[i&15];
	gui_fore=gui_colour_rgb4[i>>4];
}
void gui_mkfont(void)
{
	const BYTE *s=onscreen_chrs; BYTE *t=onscreen_chrz,z; int n=length(onscreen_chrs);
	switch (gui_weight&3)
	{
		case 0: // condensed
			do
				*t++=*s++;
			while (--n); break;
		case 1: // normal
			do
				z=*s++,*t++=(z>>1)|z;
			while (--n); break;
		case 2: // bold
			do
				z=*s++,*t++=(z<<1)|(z>>1)|z;
			while (--n); break;
		case 3: // italic
			do
				{ z=*s++>>1; if ((n%GUI_PIXELS)<=GUI_PIXELS/2) z|=z<<1; *t++=z; }
			while (--n); break;
	}
}
#define gui_locate(x,y) (gui_posx=(x),gui_posy=(y)) // moves cursor to pos (x,y) where x=0 is leftmost column and y=0 is top line
#define gui_printf(...) sprintf(gui_buffer,__VA_ARGS__),gui_output(gui_buffer,-1)
void gui_output(unsigned char *s,int l) // prints a string (or a fragment thereof) on the screen
{
	gui_dirt=2; int z; while (l--&&(z=*s++))
	{
		int x,y; unsigned const char *t; if (z<28) z=32; else if (z>=127) z='?';
		if (z<32) // simple graphics: low, mid, high and full vertical bars; we're assuming GUI_PIXELS==12
			t=&"\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377\377"[(z-28)*3];
		else
			t=&onscreen_chrz[(z-32)*GUI_PIXELS];
		DWORD *v=&((DWORD*)hdib->pixels)[(gui_posx++)*8+gui_posy*GUI_WIDTH*8*GUI_PIXELS];
		for (y=0;y<GUI_PIXELS;++y,v+=GUI_WIDTH*8-8)
			for (z=*t++,x=8;x--;z<<=1)
				*v++=z&128?gui_fore:gui_back;
	}
}
int gui_listen(int k) // redraws the update, keeps the sound alive and waits for a keypress; returns 0 on SHUTDOWN
{
	key_char=key_code=0; SDL_Event event; k|=8;
	for (;;)
	{
		if (hsnd) // play sound
		{
			BYTE hsnd_shadow[SND_QUANTUM]; WORD hsnd_buffer[SND_QUANTUM];
			int q=flag_excl?SND_FRAMES:SND_FRAMES/2; // "-!": slow but safe
			for (int r=SDL_GetQueuedAudioSize(hsnd)/sizeof(hsnd_buffer);r<=q;++r) // keep the buffer full!
			{
				snd_frame(0); snd_testz(); snd_waves(hsnd_shadow);
				static int o=0; for (int n,i=0;i<SND_QUANTUM;++i) // normalise SDL2 AUDIO_S16 signal
					n=hsnd_shadow[i],hsnd_buffer[i]=(257*(n+o+(n>o?2:0)))/4,o=n;
				SDL_QueueAudio(hsnd,hsnd_buffer,sizeof(hsnd_buffer));
			}
		}
		gui_redraw(k); k=snd_playing?8:0;
		static char cheap=1; if (!--cheap)
		{
			cheap=4; // 1 = 100%, 2 = 50%, etc
			if (gui_dirt>1) // redraw bitmap?
				gui_dirt=SDL_BlitScaled(hdib,NULL,SDL_GetWindowSurface(hwnd),NULL)>=0;
			if (gui_dirt) // redraw window?
				SDL_UpdateWindowSurface(hwnd),gui_dirt=0;
		}
		while (SDL_PollEvent(&event))
			switch (event.type)
			{
				case SDL_WINDOWEVENT:
					if (event.window.event==SDL_WINDOWEVENT_EXPOSED)
						gui_dirt|=1; // redraw window, but not bitmap
					break;
				case SDL_MOUSEWHEEL:
					if (event.wheel.direction==SDL_MOUSEWHEEL_FLIPPED) event.wheel.y=-event.wheel.y;
					if (event.wheel.y<0) return key_code=SDL_SCANCODE_PAGEDOWN,key_char=key_flag=0,1;
					if (event.wheel.y) return key_code=SDL_SCANCODE_PAGEUP,key_char=key_flag=0,1;
					break;
				case SDL_MOUSEBUTTONDOWN: // better than SDL_MOUSEBUTTONUP?
					if (event.button.button!=SDL_BUTTON_LEFT) break;
					key_code=GUI_MOUSE; key_char=key_flag=0;
					SDL_Surface *htmp=NULL; htmp=SDL_GetWindowSurface(hwnd);
					mouse_px=event.button.x*GUI_WIDTH/htmp->w;
					mouse_py=event.button.y*GUI_HEIGHT/htmp->h;
					return 1;
				case SDL_KEYDOWN:
					if (event.key.keysym.mod&KMOD_ALT)
					{
						if (event.key.keysym.scancode==SDL_SCANCODE_RETURN) // ALT+RETURN: toggle full screen
							{ static int z=0; SDL_SetWindowFullscreen(hwnd,z^=SDL_WINDOW_FULLSCREEN_DESKTOP); k=-1; }
						//else if (event.key.keysym.scancode==SDL_SCANCODE_UP)
							//SDL_SetWindowSize(hwnd,GUI_WIDTH*8*2,GUI_HEIGHT*GUI_PIXELS*2);
						//else if (event.key.keysym.scancode==SDL_SCANCODE_DOWN)
							//SDL_SetWindowSize(hwnd,GUI_WIDTH*8,GUI_HEIGHT*GUI_PIXELS);
						break; // ignore other ALT+KEY combinations, Win32 needs them
					}
					key_flag=event.key.keysym.mod; key_code=event.key.keysym.scancode;
					if (key_code==SDL_SCANCODE_KP_ENTER)
						key_code=SDL_SCANCODE_RETURN; // merge both "^M" keys together
					if (GUI_CONTROL&&event.key.keysym.sym>='a'&&event.key.keysym.sym<='z')
						key_char=event.key.keysym.sym&31; // SDL2 won't generate "^A" codes on its own!
					return 1;
				case SDL_TEXTINPUT: // can follow and override SDL_KEYDOWN
					if ((key_char=event.text.text[0])<0)
						key_char=(key_char&31)*64+(event.text.text[1]&63); // minimal UTF-8, this program is limited to 8-bit chars anyway
					return 1;
				case SDL_DROPFILE:
					strcpy(gui_string,event.drop.file);
					SDL_free(event.drop.file);
					key_code=GUI_MOUSE-1; key_char=key_flag=0;
					return 1;
				case SDL_QUIT:
					return 0; // quit!!
			}
		SDL_Delay(1000/100); // sleep and wait if the user does nothing
	}
}

#else // WIN32

int GUI_WIDTH,GUI_HEIGHT;

#define GUI_ESCAPE VK_ESCAPE
#define GUI_F1 VK_F1
#define GUI_F2 VK_F2
#define GUI_F3 VK_F3
#define GUI_F4 VK_F4
#define GUI_F5 VK_F5
#define GUI_F6 VK_F6
#define GUI_F7 VK_F7
#define GUI_F8 VK_F8
#define GUI_F9 VK_F9
#define GUI_F10 VK_F10
#define GUI_F11 VK_F11
#define GUI_F12 VK_F12

#define GUI_HOME VK_HOME
#define GUI_END VK_END
#define GUI_PRIOR VK_PRIOR
#define GUI_NEXT VK_NEXT
#define GUI_UP VK_UP
#define GUI_DOWN VK_DOWN
#define GUI_LEFT VK_LEFT
#define GUI_RIGHT VK_RIGHT
#define GUI_INSERT VK_INSERT
#define GUI_DELETE VK_DELETE
#define GUI_UNDO VK_BACK
#define GUI_MOUSE 32767 // fictional
#define GUI_TAB VK_TAB
#define GUI_ENTER VK_RETURN
#define GUI_SHIFT (key_flag&(SHIFT_PRESSED))
#define GUI_CONTROL (key_flag&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
#define GUI_SHUTDOWN (key_code==GUI_ESCAPE||key_code==GUI_F10||(key_code==GUI_F4&&(key_flag&(LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))))

HWAVEOUT hsnd=NULL;
#define SND_FRAMES 32 // 100Hz packets
unsigned char hsnd_buffer[SND_FRAMES][SND_QUANTUM];
WAVEHDR hsnd_hdr[SND_FRAMES]; int hsnd_index=0;

DWORD iTerminalMode,iKeyboardMode; HANDLE hTerminal,hKeyboard;
CONSOLE_SCREEN_BUFFER_INFO pConsoleScreenBufferInfo; CONSOLE_CURSOR_INFO pConsoleCursorInfo;

void gui_clrscr(int i) // clear screen
{
	COORD dwCursor={0,0}; int j; // bug on Windows 10 if NULL instead of (PDWORD)&j
	FillConsoleOutputAttribute(hTerminal,i,GUI_HEIGHT*GUI_WIDTH,dwCursor,(PDWORD)&j);
	FillConsoleOutputCharacter(hTerminal,' ',GUI_HEIGHT*GUI_WIDTH,dwCursor,(PDWORD)&j);
}
void gui_locate(int x,int y) // moves cursor to pos (x,y) where x=0 is leftmost column and y=0 is top line
{
	COORD dwCursor={x,y};
	SetConsoleCursorPosition(hTerminal,dwCursor);
}
int gui_init(void) // sets interface and sound up. 0 is ERROR
{
	GetConsoleScreenBufferInfo(hTerminal=GetStdHandle(STD_OUTPUT_HANDLE),&pConsoleScreenBufferInfo);
	GUI_WIDTH=pConsoleScreenBufferInfo.srWindow.Right-pConsoleScreenBufferInfo.srWindow.Left+1;
	GUI_HEIGHT=pConsoleScreenBufferInfo.srWindow.Bottom-pConsoleScreenBufferInfo.srWindow.Top+1;
	if (GUI_WIDTH<80||GUI_HEIGHT<25)
		return 0;
	GetConsoleMode(hTerminal,&iTerminalMode);
	SetConsoleMode(hTerminal,0);
	GetConsoleMode(hKeyboard=GetStdHandle(STD_INPUT_HANDLE),&iKeyboardMode);
	SetConsoleMode(hKeyboard,ENABLE_WINDOW_INPUT|ENABLE_MOUSE_INPUT);
	GetConsoleCursorInfo(hTerminal,&pConsoleCursorInfo);
	pConsoleCursorInfo.bVisible=0;
	SetConsoleCursorInfo(hTerminal,&pConsoleCursorInfo);
	gui_clrscr(0);
	WAVEFORMATEX wfex; memset(&wfex,0,sizeof(wfex));
	wfex.wFormatTag=WAVE_FORMAT_PCM, wfex.cbSize=0;
	wfex.wBitsPerSample=8, wfex.nChannels=wfex.nBlockAlign=1; // 8-bit mono:
	wfex.nSamplesPerSec=wfex.nAvgBytesPerSec=SND_QUALITY; // 1 sample = 1 byte
	if (flag_m||waveOutOpen(&hsnd,-1,&wfex,0,0,0))
		hsnd=0;
	else
	{
		SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS); // sound playback needs higher priority!
		for (int i=0;i<SND_FRAMES;++i)
		{
			memset(&hsnd_hdr[i],0,sizeof(WAVEHDR));
			hsnd_hdr[i].lpData=hsnd_buffer[i];
			hsnd_hdr[i].dwBufferLength=SND_QUANTUM;
			waveOutPrepareHeader(hsnd,&hsnd_hdr[i],sizeof(WAVEHDR));
		}
	}
	return 1;
}
void gui_exit(void) // shuts interface and sound down
{
	waveOutReset(hsnd);
	for (int i=0;i<SND_FRAMES;++i)
		waveOutUnprepareHeader(hsnd,&hsnd_hdr[i],sizeof(WAVEHDR));
	waveOutClose(hsnd);

	gui_clrscr(pConsoleScreenBufferInfo.wAttributes);
	//gui_locate(0,GUI_HEIGHT-1);
	SetConsoleTextAttribute(hTerminal,pConsoleScreenBufferInfo.wAttributes);
	SetConsoleMode(hTerminal,iTerminalMode);
	SetConsoleMode(hKeyboard,iKeyboardMode);
	pConsoleCursorInfo.bVisible=1;
	SetConsoleCursorInfo(hTerminal,&pConsoleCursorInfo);
	CloseHandle(hTerminal);
	CloseHandle(hKeyboard);
}
void gui_title(char *s) // sets window title
{
	if (/*!s||*/!*s)
		SetConsoleTitle(MY_NAME);
	else
	{
		/*char *r=strrchr(s,PATH_SEPARATOR); if (!r) r=s; else ++r;*/ // filename only
		sprintf(gui_buffer,"%s - " MY_NAME,/*r*/s); SetConsoleTitle(gui_buffer);
	}
}
void gui_colour(unsigned char z) // sets colour attribute, low nibble is paper, high nibble is pen
{
	//static unsigned char zzz=0; if (zzz!=z) zzz=z,
	SetConsoleTextAttribute(hTerminal,0
		|(z&0x01?BACKGROUND_BLUE:0)
		|(z&0x02?BACKGROUND_RED:0)
		|(z&0x04?BACKGROUND_GREEN:0)
		|(z&0x08?BACKGROUND_INTENSITY:0)
		|(z&0x10?FOREGROUND_BLUE:0)
		|(z&0x20?FOREGROUND_RED:0)
		|(z&0x40?FOREGROUND_GREEN:0)
		|(z&0x80?FOREGROUND_INTENSITY:0));
}
#define gui_printf(...) sprintf(gui_buffer,__VA_ARGS__),gui_output(gui_buffer,strlen(gui_buffer))
#define gui_output(s,l) WriteConsole(hTerminal,(s),(l),NULL,NULL) // prints a string on the screen
int gui_listen(int k) // redraws the update, keeps the sound alive and waits for a keypress; returns 0 on SHUTDOWN
{
	key_char=key_code=0; k|=8; INPUT_RECORD pBuffer;
	MMTIME session_mmtime; memset(&session_mmtime,0,sizeof(session_mmtime));
	session_mmtime.wType=TIME_SAMPLES;
	for (;;)
	{
		gui_redraw(k); k=snd_playing?8:0;
		if (hsnd)
		{
			static int p=0; waveOutGetPosition(hsnd,&session_mmtime,sizeof(MMTIME));
			int q=flag_excl?SND_FRAMES*SND_QUANTUM:SND_FRAMES*SND_QUANTUM/2; // "-!": slow but safe
			while ((int)(p-session_mmtime.u.sample)<q) // keep the queue full while avoiding overflows
			{
				unsigned char *z=hsnd_buffer[hsnd_index];
				snd_frame(0); snd_testz(); snd_waves(z);
				static int o=0; for (int n,i=0;i<SND_QUANTUM;++i) // normalise PCM 1CH 8BIT signal
					n=z[i],z[i]=128-(n+o+(n>o?2:0))/4,o=n;
				while (waveOutWrite(hsnd,&hsnd_hdr[hsnd_index],sizeof(WAVEHDR))==WAVERR_STILLPLAYING) Sleep(1000/100); // catch WAVERR_STILLPLAYING if it ever happens!
				hsnd_index=(hsnd_index+1)%SND_FRAMES; p+=SND_QUANTUM;
			}
		}
		DWORD i; PeekConsoleInput(hKeyboard,&pBuffer,1,(PDWORD)&i);
		if (i)
		{
			ReadConsoleInput(hKeyboard,&pBuffer,1,(PDWORD)&i);
			if ((pBuffer.EventType&KEY_EVENT)&&(pBuffer.Event.KeyEvent.bKeyDown))
			{
				key_flag=pBuffer.Event.KeyEvent.dwControlKeyState;
				key_code=pBuffer.Event.KeyEvent.wVirtualKeyCode;
				key_char=pBuffer.Event.KeyEvent.uChar.AsciiChar&0xFF;
				if (key_code|key_char) return 1;
			}
			else if ((pBuffer.EventType&MOUSE_EVENT)&&!pBuffer.Event.MouseEvent.dwEventFlags&&pBuffer.Event.MouseEvent.dwButtonState==1)
			{
				key_code=GUI_MOUSE; key_char=key_flag=0;
				mouse_px=pBuffer.Event.MouseEvent.dwMousePosition.X; mouse_py=pBuffer.Event.MouseEvent.dwMousePosition.Y;
				return 1;
			}
		}
		Sleep(1000/100); // sleep and wait if the user does nothing
	}
}

#endif

// shared procedures and functions

int dirty,can_undo,can_redo;
void inc_dirty(void) { dirty=dirty<1?1:2; } // 0 -> 1, 1 -> 2, 2 -> 2
void dec_dirty(void) { dirty=dirty>1?2:0; } // 0 -> 0, 1 -> 0, 2 -> 2
void undo_set(void)
{
	loopto_=loopto;
	orders_=orders;
	instrs_=instrs;
	MEMSAVE(order_,order);
	MEMSAVE(score_,score);
	MEMSAVE(scrln_,scrln);
	MEMSAVE(instr_,instr);
	inc_dirty();
	can_undo=1;
	can_redo=0;
}
INLINE void intswp(int *i,int *j)
{
	int k=*i; *i=*j; *j=k;
}
void *memswp(void *t,void *s,int l)
{
	char a,*r=(char*)t,*q=(char*)s;
	while (l--)
		a=*r,*r++=*q,*q++=a;
	return t;
}
void undo_get(int redo)
{
	if ((!flag_uu&&can_undo)||((redo&&can_redo)||(!redo&&can_undo)))
	{
		intswp(&loopto,&loopto_);
		intswp(&orders,&orders_);
		intswp(&instrs,&instrs_);
		memswp(order,order_,sizeof(order));
		memswp(score,score_,sizeof(score));
		memswp(scrln,scrln_,sizeof(scrln));
		memswp(instr,instr_,sizeof(instr));
		if (!flag_uu)
		{
			if (dirty<2)
				dirty=1-dirty; // 0 -> 1, 1 -> 0, 2 ->
		}
		else
		{
			can_redo=!(can_undo=redo);
			redo?inc_dirty():dec_dirty();
		}
	}
}

INLINE int intmin(int x,int y) { return x<y?x:y; }
INLINE int intmax(int x,int y) { return x>y?x:y; }

unsigned char gui_upink0,gui_upink1,gui_upink2,gui_upink3;
void gui_upinit(int q) // colours of the panel contents; `q` means the panel is active
{
	gui_upink0=q?0xF0:0x80;
	gui_upink1=q?0x0F:0x08;
	gui_upink2=q?0xF9:0x81;
	gui_upink3=q?0x9F:0x18;
}
void gui_upline(char *s,int l,int m,int n,int q) // print `l` chars of line `s` in a panel: if `q` is true, selected range is `m..n`
{
	gui_colour(q?gui_upink2:gui_upink0);
	if (m>=0&&m<l&&n>m&&n<=l) // avoid accidents!
	{
		gui_output(s,m);
		gui_colour(q?gui_upink3:gui_upink1);
		gui_output(&s[m],n-m);
		gui_colour(q?gui_upink2:gui_upink0);
		gui_output(&s[n],l-n);
	}
	else
		gui_output(s,l);
}
void gui_redraw(int q) // redraws parts of interface on request
{
	if (flag_pp&&snd_playing) // panels may need scrolling and redrawing during playback
	{
		if (activescore!=snd_score)
			q|=8+1,selectscore=activescore=snd_score;
		if (activeorder!=snd_order)
			q|=8+4,selectorder=activeorder=snd_order;
	}
	int i,j,k,l=GUI_HEIGHT-1,m=0,n=0; // avoid warnings; m and n are set when required
	if (q&1) // left panel?
	{
		gui_upinit(activepanel==0);
		k=scrln[order[0][activeorder][0]];
		for (i=2;i<l;++i)
		{
			j=i-(GUI_HEIGHT+1)/2+activescore;
			gui_locate(0,i);
			if (j<(n=0)||j>=128)
				strcpy(gui_buffer," -- ------ ------ ------ ");
			else
			{
				#if 1 // hybrid: " 00..C7"
				sprintf(gui_buffer," %c%c ",itoradix(j/10),48+(j%10));
				#elif 0 // decimal: " 00".."127"
				sprintf(gui_buffer,"%2i%c ",j/10,48+(j%10));
				#else // hexadecimal: "00".."7F"
				sprintf(gui_buffer," %02X ",j);
				#endif
				for (m=0;m<3;++m)
					sprintf(&gui_buffer[4+7*m],"%s ",
						musical(score[order[m][activeorder][0]][j][0],score[order[m][activeorder][0]][j][1]));
				if (j>=k)
					gui_buffer[0]=' ',gui_buffer[1]=gui_buffer[2]='/';
				if (j>=intmin(activescore,selectscore)&&j<=intmax(activescore,selectscore))
				{
					m=4+intmin(activescoreitem,selectscoreitem)*7;
					n=10+intmax(activescoreitem,selectscoreitem)*7;
				}
			}
			gui_upline(gui_buffer,25,m,n,j==activescore);
		}
	}
	if (q&2) // central panel?
	{
		gui_upinit(activepanel==1);
		for (i=2;i<l;++i)
		{
			j=i-(GUI_HEIGHT+1)/2+activeinstr;
			gui_locate(25,i);
			if (j<(n=0)||j>=256)
				strcpy(gui_buffer," -- ---------------- ----------- ");
			else if (!j)
			{
				strcpy(gui_buffer," .. << PORTAMENTO >> ........... ");
				n=(m=4)+16;
			}
			else
			{
				sprintf(gui_buffer," %02X %-16s %02X%02X%02X%02X%01X%02X ",j,
					&instr[j][8],
					instr[j][0],instr[j][1],
					instr[j][2],instr[j][3],
					instr[j][4],instr[j][5]);
				if (j>=instrs)
					gui_buffer[1]=gui_buffer[2]='/';
				n=(m=4+activeinstritem)+1;
			}
			{ if (j!=activeinstr) n=0; } gui_upline(gui_buffer,33,m,n,n);
		}
	}
	if (q&4) // right panel?
	{
		gui_upinit(activepanel==2);
		for (i=2;i<l;++i)
		{
			j=i-(GUI_HEIGHT+1)/2+activeorder;
			gui_locate(58,i);
			if (j<(n=0)||j>=256)
				sprintf(gui_buffer," -- ----- ----- ----- ");
			else
			{
				int a=(scrln[order[0][j][0]]==scrln[order[1][j][0]]&&scrln[order[1][j][0]]==scrln[order[2][j][0]])?' ':'!';
				sprintf(gui_buffer," %02X %02X%+03i%c%02X%+03i%c%02X%+03i ",j,
					order[0][j][0],(signed char)order[0][j][1],a,
					order[1][j][0],(signed char)order[1][j][1],a,
					order[2][j][0],(signed char)order[2][j][1]);
				if (j==loopto)
					gui_buffer[3]=gui_buffer[9]=gui_buffer[15]='*';//gui_buffer[1]=gui_buffer[2]='*';
				if (j>=orders)
					gui_buffer[1]=gui_buffer[2]='/';
				if (j>=intmin(activeorder,selectorder)&&j<=intmax(activeorder,selectorder))
				{
					m=4+intmin(activeorderitem,selectorderitem)*6;
					n=9+intmax(activeorderitem,selectorderitem)*6;
				}
			}
			gui_upline(gui_buffer,22,m,n,j==activeorder);
		}
	}
	if (q&8) // status bar?
	{
		gui_locate(0,l);
		gui_colour(activepanel==3?0x0F:0xF8);
		sprintf(gui_buffer," Octave %i  Transpose %+03i  %03i/%02i Hz  %s "
			#if 1 // decimal: " 00".."127"
			"%02X.%03i"
			#else // hexadecimal: "00".."7F"
			" %02X.%02X"
			#endif
			" %c%c%c  " MY_FILE " " MY_DATE " F1:Help ",
			activeoctave,transpose,hertz,divisor,hsnd?(snd_playing?(snd_looping?"LOOP":"PLAY"):"STOP"):"MUTE",snd_order,snd_score,
			snd_playing?vubars[flag_xx&1][snd_ampl[0]>>4]:'-',snd_playing?vubars[flag_xx&1][snd_ampl[1]>>4]:(dirty?'*':'-'),snd_playing?vubars[flag_xx&1][snd_ampl[2]>>4]:'-');
		if (activepanel==3)
		{
			switch (activeparameter)
			{
				case 0:
					m=21;
					n=24;
					break;
				case 1:
					m=26;
					n=29;
					break;
				case 2:
					m=30;
					n=32;
					break;
			}
			gui_output(gui_buffer,m);
			gui_colour(0xF9);
			gui_output(&gui_buffer[m],n-m);
			gui_colour(0x0F);
		}
		else
			n=0;
		gui_output(&gui_buffer[n],strlen(gui_buffer)-n);
	}
	if (q&16) // panel captions?
	{
		gui_locate(0,1);
		gui_colour(activepanel==0?0x0A:0xA2);
		gui_printf(" ##    %c      %c      %c   ",snd_bool[0]?'A':'-',snd_bool[1]?'B':'-',snd_bool[2]?'C':'-');
		gui_colour(activepanel==1?0x0C:0xC4);
		gui_printf(" ##    Instrument    AmplNoisSFX ");
		gui_colour(activepanel==2?0x09:0x91);
		gui_printf(" ##   %c     %c     %c   ",snd_bool[0]?'A':'-',snd_bool[1]?'B':'-',snd_bool[2]?'C':'-');
	}
	if (q&32) // screen caption?
	{
		gui_locate(0,0);
		gui_colour(0xF8);
		gui_printf(strlen(title1)>38?" %.36s.. ":" %-38s ",title1);
		gui_printf(strlen(title2)>38?" %.36s.. ":" %-38s ",title2);
	}
}

int stack_pp; // temporarily disable PLAYBACK FOLLOW!
void push_pp(void) { stack_pp=flag_pp; flag_pp=0; }
void pop_pp(void) { flag_pp=stack_pp; }

int gui_readme(char *s) // show text
{
	int i,lx=0,ly=1,x=0,y;
	char c,*r=s;
	while (*r)
	{
		if ((c=*r++)=='\n')
		{
			if (x>lx)
				lx=x;
			x=0;
			++ly;
		}
		else
		{
			if (c=='\t')
				x|=7;
			++x;
		}
	}
	if (x>lx)
		lx=x;
	gui_colour(0x0F);
	lx+=2;
	y=(GUI_HEIGHT-ly)/2;
	x=(80-lx)/2;
	ly+=y;
	r=s;
	while (y<ly)
	{
		gui_locate(x,y);
		char *t=gui_buffer;
		*t++=' ';
		i=0;
		while ((c=*r++)&&(c!='\n'))
			if (c=='\t')
				do *t++=' '; while (++i&7);
			else
				++i,*t++=c;
		*t=0;
		strpad(gui_buffer,lx);
		gui_output(gui_buffer,lx);
		++y;
	}
	for (;;)
	{
		push_pp(); i=gui_listen(0); pop_pp();
		if (!i||key_code==GUI_ENTER||key_code==GUI_MOUSE||GUI_SHUTDOWN)
			break;
	}
	return -1; // for gui_redraw()
}
#define gui_readmef(...) ( sprintf(tmp,__VA_ARGS__), gui_readme(tmp) )

int gui_really(const char *m) // yes or no: ZERO is NO
{
	int i,j=strlen(m)+23;
	gui_colour(0x0F);
	gui_locate((80-j)/2,GUI_HEIGHT/2);
	gui_printf(" %s! Are you sure? [Y/N] ",m);
	gui_buffer[0]=0; strpad(gui_buffer,j);
	gui_locate((80-j)/2,GUI_HEIGHT/2-1);
	gui_output(gui_buffer,j);
	gui_locate((80-j)/2,GUI_HEIGHT/2+1);
	gui_output(gui_buffer,j);
	do
	{
		push_pp(); i=gui_listen(0); pop_pp(); key_char=ucase(key_char);
		if (key_code==GUI_ENTER||key_char=='Y'||key_char=='S'||key_char=='O'||key_char=='J')
			return 1;
		if (key_code==GUI_MOUSE)
			return (mouse_px>=(80-j)/2&&mouse_px<(80-j)/2+j&&mouse_py>=GUI_HEIGHT/2-1&&mouse_py<=GUI_HEIGHT/2+1); // MOUSE: click on button YES, click outside NO
	}
	while (i&&!GUI_SHUTDOWN&&key_char!='N');
	return 0;
}
int gui_really_overwrite(void) { return gui_really("Overwrite file"); }
int gui_prompt(char *s,int l,int x,int y) // enter text: ZERO is ABORT
{
	int i,j,z;
	i=j=strlen(s);
	strcpy(tmp,s); // must not use gui_buffer, gui_redraw() writes on it!
	strpad(tmp,l);
	for (;;)
	{
		if (i<0)
			i=0;
		else if (i>j)
			i=j;
		gui_colour(0xF0);
		gui_locate(x+l,y);
		gui_output(" ",1);
		gui_locate(x-1,y);
		gui_output(" ",1);
		gui_output(tmp,i);
		if (i<l)
		{
			gui_colour(0x0F);
			gui_output(tmp+i,1);
			if (i<l-1)
			{
				gui_colour(0xF0);
				gui_output(tmp+i+1,l-i-1);
			}
		}
		push_pp(); z=gui_listen(0); pop_pp();
		if (!z||GUI_SHUTDOWN)
			return 0;
		if (key_code==GUI_ENTER)
		{
			tmp[j]=0;
			strcpy(s,tmp);
			return 1;
		}
		if (key_code==GUI_MOUSE)
		{
			if (mouse_py!=y)//||mouse_px<x||mouse_px>=x+l)
				return 0; // ESCAPE!
			i=mouse_px<x?0:(mouse_px<=x+j)?mouse_px-x:j; // MOVE
		}
		else if (key_code==GUI_HOME||key_code==GUI_UP)
			i=0;
		else if (key_code==GUI_END||key_code==GUI_DOWN)
			i=j;
		else if (key_code==GUI_LEFT)
			--i;
		else if (key_code==GUI_RIGHT)
			++i;
		else if ((key_code==GUI_UNDO&&i>0)||(key_code==GUI_DELETE&&i<j))
		{
			if (key_code==GUI_UNDO)
				--i;
			for (z=i;z<l-1;++z) tmp[z]=tmp[z+1];
			tmp[z]=' '; --j;
		}
		else if (j<l&&key_char>=32&&key_char<127) // sorry, nothing but pure ASCII here :-(
		{
			for (z=l-1;z>i;--z) tmp[z]=tmp[z-1];
			tmp[i++]=key_char; ++j;
		}
	}
}

#define GUI_MAX_FILES (9<<9) // max. files in list
#define GUI_AVG_FNAME (1<<4) // avg. filename size
#define gui_notyet(i,s) (((i)<GUI_MAX_FILES-1)&&((unsigned char*)(s)<&gui_filechar[sizeof(gui_filechar)-MAXSTRLEN]))
unsigned char gui_filechar[GUI_MAX_FILES*GUI_AVG_FNAME],*gui_filename[GUI_MAX_FILES],gui_filepath[8<<9],gui_filelast[256],gui_padded80[]=" %-78s ";
int gui_filesize[GUI_MAX_FILES];
void gui_sortfilelist(int i,int l) // sort subset of files (comb sort)
{
	int q,w=l,z=i+l;
	do
	{
		if (!(q=(w=w*3/4)))
			w=1; // retry till no changes
		for (int j=i,k=i+w;k<z;++j,++k)
			if (strcasecmp(gui_filename[j],gui_filename[k])>0)
			{
				q=1;
				char *n=gui_filename[j];
				gui_filename[j]=gui_filename[k];
				gui_filename[k]=n;
				int x=gui_filesize[j];
				gui_filesize[j]=gui_filesize[k];
				gui_filesize[k]=x;
			}
	}
	while (q);
}
char *gui_truepath(char *s) // turn filename `s` (complete or partial) into a complete path at `gui_filepath`; returns a pointer to the filename alone
{
	#ifdef _WIN32
	char *r; GetFullPathName(*s?s:"|",MAXSTRLEN,gui_filepath,&r); return r; // a dummy filename will do
	#else
	if (*s) realpath(s,gui_filepath); else { realpath(".",gui_filepath); if (gui_filepath[length(gui_filepath)-1]!='/') strcat(gui_filepath,"/"); }
	return strrchr(gui_filepath,'/')+1; // OSes like Alpine Linux expect "." and may NOT append '/'! LINUX also expects the buffer to be quite big!
	#endif
}
int gui_browse(char *fullpath,int savemode,const char *extens,const char *caption) // select file: ZERO is ABORT
{
	gui_locate(0,GUI_HEIGHT/4-1);
	gui_colour(0xF8);
	gui_printf(gui_padded80,caption);
	char *s,*r=gui_truepath(fullpath); strcpy(gui_filelast,r); *r=0;
	int i,j,activefile,filenames;
	for (;;)
	{
		// define exact path

		r=gui_filechar; filenames=0;

		#ifdef _WIN32
		// build drive list
		strcpy(s=gui_findname,"A:\\"); i=0;
		while (*gui_findname<='Z')
		{
			if ((gui_win32units>>i)&1)
				gui_filesize[filenames]=0,gui_filename[filenames++]=r,
				*r++=*s,*r++=':',*r++='\\',*r++=0;
			++i; ++*gui_findname;
		}
		#endif

		// build folder list
		activefile=filenames;
		// list directories
		if (gui_findinit(gui_filepath))
		{
			j=filenames;
			do if (gui_notyet(filenames,r)&&gui_findtest(NULL))
			{
				gui_filesize[filenames]=0,gui_filename[filenames++]=r;
				s=gui_findname;
				while (*r++=*s++)
					;
				--r; *r++=PATH_SEPARATOR,*r++=0;
			}
			while (gui_findnext());
			gui_sortfilelist(j,filenames-j);
		}
		gui_findlast();

		// build file list
		if (gui_findinit(gui_filepath))
		{
			j=filenames;
			do if (gui_notyet(filenames,r)&&gui_findtest(extens))
			{
				gui_filesize[filenames]=gui_findsize,gui_filename[filenames++]=r;
				s=gui_findname;
				while (*r++=*s++)
					;
			}
			while (gui_findnext());
			gui_sortfilelist(j,filenames-j);
		}
		gui_findlast();

		// finish list
		if (gui_notyet(filenames,r)&&savemode)
		{
			s="<new file>";
			gui_filename[filenames++]=r;
			while (*r++=*s++)
				;
		}
		i=*r=0; int update=1;
		#ifdef _WIN32
		while (i<filenames&&strcasecmp(gui_filename[i],gui_filelast))
			++i;
		#else
		while (i<filenames&&strcmp(gui_filename[i],gui_filelast))
			++i;
		#endif
		activefile=i<filenames?i:savemode?filenames-1:activefile;

		// handle user logic
		for (int dblclk=0;;) // show list and handle keys
		{
			keepwithin(&activefile,0,filenames-1);
			if (update)
			{
				gui_upinit(1);
				for (i=GUI_HEIGHT/4;i<=GUI_HEIGHT*3/4;++i)
				{
					int k;
					j=i-((GUI_HEIGHT-1)/2)+activefile;
					if (j>=0&&j<filenames)
					{
						k=strlen(r=utf8_display(gui_filename[j])); // notice that our UTF-8 logic merely reduces characters to bytes
						if (gui_filesize[j])
							if (k>68)
								k=68,sprintf(gui_buffer," %.66s.. %9i ",r,gui_filesize[j]);
							else
								sprintf(gui_buffer," %-68s %9i ",r,gui_filesize[j]);
						else
							if (k>78)
								k=78,sprintf(gui_buffer," %.76s.. ",r);
							else
								sprintf(gui_buffer,(char*)gui_padded80,r);
					}
					else
						k=0,sprintf(gui_buffer,gui_padded80,"");
					gui_locate(0,i);
					if (j!=activefile) k=0;
					gui_upline(gui_buffer,80,1,1+k,j==activefile);
				}
				gui_locate(0,i);
				gui_colour(0xF8);
				gui_printf(gui_padded80,utf8_display(gui_filepath));
			}
			push_pp(); update=gui_listen(0); pop_pp();
			if (!update||GUI_SHUTDOWN)
				return 0;
			update=0; if (key_code==GUI_ENTER)
				break;
			if (key_code==GUI_MOUSE)
			{
				if (mouse_py<GUI_HEIGHT/4-1||mouse_py>GUI_HEIGHT*3/4+1)
					return 0; // ESCAPE!
				if (i=mouse_py-((GUI_HEIGHT-1)/2))
					dblclk=0;
				else if (++dblclk>1)
					break;
				activefile+=i; update=1;
			}
			else switch(dblclk=0,key_code)
			{
				case GUI_UP:
					--activefile;
					update=1;
					break;
				case GUI_DOWN:
					++activefile;
					update=1;
					break;
				case GUI_LEFT:
				case GUI_PRIOR:
					activefile-=GUI_HEIGHT/2;
					update=1;
					break;
				case GUI_RIGHT:
				case GUI_NEXT:
					activefile+=GUI_HEIGHT/2;
					update=1;
					break;
				case GUI_HOME:
					activefile=0;
					update=1;
					break;
				case GUI_END:
					activefile=filenames-1;
					update=1;
					break;
				default:
					key_char=ucase(key_char);
					for (i=activefile+1;i<filenames;++i)
						if (ucase(*gui_filename[i])==key_char)
							break;
					if (i>=filenames)
						for (i=0;i<activefile;++i)
							if (ucase(*gui_filename[i])==key_char)
								break;
					if (i!=activefile)
					{
						activefile=i;
						update=1;
					}
					break;
			}
		}

		// what to do with the user's choice?
		if (*gui_filename[activefile]=='<') // new file?
		{
			*gui_filename[activefile]=0;
			if (!gui_prompt(gui_filename[activefile],80-2,1,(GUI_HEIGHT-1)/2))
				return 0;
			if (!(r=strrchr(gui_filename[activefile],'.'))||(r<strrchr(gui_filename[activefile],PATH_SEPARATOR))) // did the user write an extension?
				strcat(gui_filename[activefile],extens);
			strcat(gui_filepath,gui_filename[activefile]);
			if (savemode&&(f=fopen(gui_filepath,"r"))) // does the file already exist?
				if (fclose(f),!gui_really_overwrite()) return 0; // quit if the user says no!
			return strcpy(fullpath,gui_filepath),1;
		}
		#ifdef _WIN32
		else if (gui_filename[activefile][1]==':') // a drive?
			strcpy(gui_filepath,gui_filename[activefile]),*gui_filelast=0;
		#endif
		else if (strchr(gui_filename[activefile],PATH_SEPARATOR)) // a folder?
		{
			#ifdef _WIN32
			if (strcmp(gui_filename[activefile],strcpy(gui_filelast,"..\\")))
			#else
			if (strcmp(gui_filename[activefile],strcpy(gui_filelast,"../")))
			#endif
				strcat(gui_filepath,gui_filename[activefile]);
			else if ((r=strrchr(gui_filepath,PATH_SEPARATOR))>strchr(gui_filepath,PATH_SEPARATOR))
			{
				*r=0; s=strrchr(gui_filepath,PATH_SEPARATOR);
				*r=PATH_SEPARATOR; strcpy(gui_filelast,++s);
				*s=0;
			}
		}
		else // a file!
		{
			strcat(gui_filepath,gui_filename[activefile]);
			if (savemode&&(strcmp(fullpath,gui_filepath))) // is this a different file?
				if (!gui_really_overwrite()) return 0; // quit if the user says no!
			return strcpy(fullpath,gui_filepath),1;
		}
	}
}

// auxiliary tracker functions -------------------------------- //

int transposesong(void) // apply current transposition to whole song, return redraw bits
{
	if (transpose)
	{
		undo_set();
		for (int m=0;m<orders;++m)
		{
			order[0][m][1]+=transpose;
			order[1][m][1]+=transpose;
			order[2][m][1]+=transpose;
		}
		transpose=0;
		return 4|8;
	}
	return 0;
}
int comparescores(unsigned char order1[128][2],unsigned char order2[128][2],int l)
{
	int i=0,j,k,d=0,n=0;
	while (l--)
	{
		j=order1[i][0];
		k=order2[i][0];
		if (!j||!k||j>(CHIPNSFX_SIZE-2)||k>(CHIPNSFX_SIZE-2))
		{
			if (j!=k)
				return 128; // give up if it isn't note VS note!
		}
		else
		{
			if (order1[i][1]!=order2[i][1])
				return 128; // different instruments, give up!
			if (j>(CHIPNSFX_SIZE-4)&&k>(CHIPNSFX_SIZE-4))
				;
			else
			{
				if (j>(CHIPNSFX_SIZE-4)||k>(CHIPNSFX_SIZE-4))
					return 128;
				if (j+d!=k)
				{
					if (n)
						return 128; // give up if delta changes twice!
					else
						d=k-j; // set delta
				}
				++n;
			}
		}
		++i;
	}
	return d; // delta
}
int charshortcuts(char k)
{
	switch (k)
	{
		case '+':
			selectorder=++activeorder;
			selectorderitem=activeorderitem;
			return 4|1;
		case '-':
			selectorder=--activeorder;
			selectorderitem=activeorderitem;
			return 4|1;
		case '*':
			++activeoctave;
			return 8;
		case '/':
			--activeoctave;
			return 8;
		case '<':
			--activeinstr;
			return 2;
		case '>':
			++activeinstr;
			return 2;
	}
	return 0;
}
int validhex(char k)
{
	if ((k>='0')&&(k<='9'))
		return k-'0';
	if ((k>='A')&&(k<='F'))
		return k-'A'+10;
	return -1;
}

int extendscores(int i) // make score longer, return redraw bits
{
	int j=0;
	if (scrln[order[0][activeorder][0]]<i)
		j=4,scrln[order[0][activeorder][0]]=i;
	if (scrln[order[1][activeorder][0]]<i)
		j=4,scrln[order[1][activeorder][0]]=i;
	if (scrln[order[2][activeorder][0]]<i)
		j=4,scrln[order[2][activeorder][0]]=i;
	return j;
}
#define extendinstrs(i) if (instrs<i) instrs=i;
#define extendorders(i) if (orders<i) orders=i;

int dupe_instr_test(int q) // is the instrument `q` in the song?
{
	int i,j;
	for (j=0;j<256;++j)
		for (i=0;i<scrln[j];++i)
			if (score[j][i][1]==q)
				return j;
	return j; // unused instrument
}
int dupe_instr_show(int q) // where in the song is the instrument `q`?
{
	if (q<256)
		q=gui_readmef("\nInstrument appears in pattern #%02X.\n",q);
	else
		q=gui_readme("\nUnused instrument.\n");
	return q;
}

char *sprintferror(const char *s) // generate an error message for the GUI
{
	sprintf(tmp,"\nError: %s!\n",s);
	return tmp;
}

#endif

// MAIN() ----------------------------------------------------- //

const char gui_chp_ext[]=".chp",gui_load_song[]="Load song",txt_load_error[]="cannot read song",txt_save_error[]="cannot save song";

int printerror1(const char *s) { fprintf(stderr,"error: %s!\n",s); return 1; }
int main(int argc,char *argv[])
{
	int i=*filepath=0,j,k; char *si=NULL,*so=NULL;
	char flag_w=0,flag_y=0/*,flag_s=0*/;
	while (++i<argc)
	{
		if (argv[i][0]=='-'&&argv[i][1])
		{
			j=0; while (i<argc&&++j&&(k=argv[i][j]))
			{
				char *optarg;
				#define OPTGET() (*(optarg=&argv[i][++j])||((i<argc+1)&&(optarg=argv[++i])))
				switch (k)
				{
					case '$':
						#ifdef SDL2
						++gui_weight;
						#endif
						break;
					case '!':
						flag_excl=1;
						break;
					case 'a':
						flag_a=1;
						break;
					case 'b':
						flag_b|=7*9; // 1+2+4
						break;
					case 'B':
						if (OPTGET())
						{
							j=0; while (k=optarg[j++])
								switch (k-'0')
								{
									case 1:
										flag_b|=1*9;
										break;
									case 2:
										flag_b|=2*9;
										break;
									case 3:
										flag_b|=4*9;
										break;
									case 4:
										flag_b|=1;
										break;
									case 5:
										flag_b|=2;
										break;
									case 6:
										flag_b|=4;
										break;
									case 7:
										if (k=(optarg[j]=='-'))
											++j;
										flag_bb[0]=k?2:1;
										flag_bb[1]=k?1:2;
										flag_bb[2]=0;
										break;
									case 8:
										if (k=(optarg[j]=='-'))
											++j;
										flag_bb[0]=k?2:0;
										flag_bb[1]=k?0:2;
										flag_bb[2]=1;
										break;
									case 9:
										if (k=(optarg[j]=='-'))
											++j;
										flag_bb[0]=k?1:0;
										flag_bb[1]=k?0:1;
										flag_bb[2]=2;
										break;
									default:
										i=argc; // HELP!
									case 0:
										break;
								}
						}
						else
							i=argc; // HELP!
						j=-1; // skip!
						break;
					case 'c':
						if (OPTGET())
							flag_c=strtol(optarg,NULL,10);
						else
							i=argc; // HELP!
						j=-1; // skip!
						break;
					//case 'F':
						//SND_QUALITY=SND_HIGHEST;
						//break;
					case 'k':
						if (OPTGET())
						{
							if (optarg[0]=='1') // QWERTZ
								strcpy(pianos,pianoz[0]);
							else if (optarg[0]=='2') // AZERTY
								strcpy(pianos,pianoz[1]);
							//else if (strlen(optarg)==strlen(pianos)) // custom
								//strcpy(pianos,optarg);
							else
								i=argc; // HELP!
						}
						else
							i=argc; // HELP!
						j=-1; // skip!
						break;
					case 'l':
						if (OPTGET())
							title0=(optarg);
						else
							i=argc; // HELP!
						j=-1; // skip!
						break;
					case 'L':
						flag_ll=1;
						break;
					case 'm':
						flag_m=1;
						break;
					case 'n':
						if (OPTGET())
							flag_n=atoi(optarg);
						else
							i=argc; // HELP!
						j=-1; // skip!
						break;
					case 'N':
						if (OPTGET())
							flag_nn=strtol(optarg,NULL,10);
						else
							i=argc; // HELP!
						j=-1; // skip!
						break;
					case 'p':
						flag_p=1;
						break;
					case 'P':
						flag_pp=1;
						break;
					case 'q':
						flag_q=1;
						break;
					case 'Q':
						++SND_DEPTH;
						break;
					case 'r':
						flag_r|=2;
						break;
					case 'R':
						flag_r|=1;
						break;
					/*case 's':
						flag_s=1;
						break;*/
					case 't':
						flag_t=1;
						break;
					case 'T':
						flag_t=2;
						break;
					/*case 'u':
						flag_uu=0;
						break;*/
					case 'U':
						flag_uu=1;
						break;
					case 'v':
						flag_q=0;
						break;
					case 'w':
						flag_w=1;
						break;
					case 'W':
						flag_w=2;
						break;
					case 'y':
						flag_y=1;
						break;
					case 'X':
						++flag_xx;
						break;
					case 'Y':
						flag_y=2;
						break;
					case 'Z':
						++flag_zz;
						break;
					case 'h':
					case '?':
					default:
						i=argc; // HELP!
				}
			}
		}
		else if (!si)
			si=argv[i];
		else if (!so)
			so=argv[i];
		else
			i=argc; // HELP!
	}
	#ifdef DEBUG
	if (i>argc||!so)
	#else
	if (i>argc)
	#endif
		return printf(
			MY_FILE " " MY_DATE " (C) " MY_SELF "\n\n"
			"usage: " MY_FILE " [option..] source "
			#ifdef DEBUG
			"target"
			#else
			"[target]"
			#endif
			"\n"
			"\t-v\tverbose output\n"
			"\t-q\tquiet, no output\n"
			"\t-l STR\tset label prefix\n"
			"\t-L\tembed header\n"
			"\t-c N\tmaximum target width\n"
			"\t-b\tgenerate short calls only\n"
			"\t-B N\tfine-tune short calls for channel[s] N\n"
			"\t-r\talternative loop search\n"
			"\t-R\tdon't look for loops at all\n"
			"\t-t\tsimplify timings\n"
			"\t-T\tdon't store tempo at all\n"
			"\t-a\tappend to target\n"
			//"\t-F\t48000 Hz playback\n"
			"\t-Z\trectangular waves\n"
			"\t-Q\tenable oversampling\n"
			"\t-w/-W\texport song/loop as WAVE\n"
			"\t-y/-Y\texport song/loop as YM3B\n"
			"\t-n N\texport N loops\n"
			"\t-N N\ttranspose N semitones\n"
			#ifndef DEBUG
			"\t-k1\tset German keymap\n"
			"\t-k2\tset French keymap\n"
			//"\t-k STR\tset custom keymap\n"
			"\t-m\tmute, no sound\n"
			"\t-p\tplay song\n"
			"\t-U\tdistinct Undo and Redo\n"
			"\t-P\tcursor follows playback\n"
			"\t-X\tshow hexadecimal amplitudes\n"
			"\t-!\tcompatible audio mode\n"
			#endif
			"\n" GPL_3_INFO "\n"
			),1;

	flag_c-=10; // margin for ",$00FX,$XX"
	if (!so) /*if (!SND_DEPTH)*/ SND_DEPTH=1; // the tracker always assumes -Q
	else if (SND_DEPTH>4) SND_DEPTH=4; // overkill?
	SND_DEPTHSTEP=125000<<(1+SND_FIXED-SND_DEPTH);
	SND_DEPTHMASK=(1<<SND_DEPTH)-1;

// console operations ----------------------------------------- //

	#ifdef DEBUG
	if (!loadsong(si))
		return printerror1(txt_load_error);
	#else
	if (!si)
		newsong();
	else if (!loadsong(si))
		return printerror1(txt_load_error);
	#endif
	if (flag_n<1)
		flag_n=1;
	/*if (flag_s)
		return savesong(so)?0:printerror1(txt_save_error);*/
	if (flag_w)
		return so&&savewave(so,flag_w>1?loopto:0)?0:printerror1("cannot save WAVE");
	if (flag_y)
		return so&&saveym3b(so,flag_y>1?loopto:0)?0:printerror1("cannot save YM3B");
	#ifdef DEBUG
	#else
	if (so)
	#endif
		return savedata(so)?0:printerror1(txt_save_error);

// the tracker itself ----------------------------------------- //

	#ifndef DEBUG // enable UI!
	if (!gui_init())
		return printerror1("cannot create UI");
	undo_set();
	if (*filepath) { gui_truepath(filepath); strcpy(filepath,gui_filepath); }
	gui_title(filepath);
	flag_q=1; // show no messages!
	dirty=can_undo=can_redo=0;
	int update=flag_n=-1; // infinite
	if (flag_p&&hsnd)
		snd_play(0);
	#ifdef _WIN32
	{
		char scratch[]="A:\\"; DWORD dummy; i=0;
		while (scratch[0]<='Z') // build cache at boot time, it's faster and avoids later conflicts
		{
			if (GetDriveType(scratch)>2&&GetDiskFreeSpace(scratch,&dummy,&dummy,&dummy,&dummy))
				gui_win32units|=1<<i;
			++i; ++scratch[0];
		}
	}
	#endif
	#ifdef SDL2
	gui_mkfont();
	#endif
	for (;;)
	{
		// boundaries
		keepwithin(&activeparameter,0,3-1);
		keepwithin(&activescoreitem,0,3-1);
		keepwithin(&activeinstritem,0,28-1);
		keepwithin(&activeorderitem,0,3-1);
		keepwithin(&activescoreitem,0,3-1);
		keepwithin(&activeinstritem,0,28-1);
		keepwithin(&activeorderitem,0,3-1);
		keepwithin(&activescore,0,128-1);
		keepwithin(&activeinstr,0,256-1);
		keepwithin(&activeorder,0,256-1);
		keepwithin(&activeoctave,0,8);
		keepwithin(&hertz,25,100);
		keepwithin(&divisor,1,99);
		keepwithin(&transpose,-99,+99);
		keepwithin(&selectscore,0,128-1);
		keepwithin(&selectscoreitem,0,3-1);
		keepwithin(&selectorderitem,0,3-1);
		int l,m,n,o,p,x,y,z,writeable=!(flag_pp&&snd_playing);

		while (!gui_listen(update&~8)||GUI_SHUTDOWN)
		{
			//if (gui_really(dirty?"Quit without saving":"Quit")) // confirm all exits
			if (dirty?gui_really("Quit without saving"):(!GUI_SHUTDOWN||gui_really("Quit"))) // confirm when dirty or ESCAPE/F10, redundant otherwise
				return gui_exit(),0;
			update=-1; // redraw everything!
		}
		// merge several synonyms
		if (key_code==GUI_INSERT)
		{
			if (GUI_SHIFT)
				key_char=(key_code='V')&31; // C-v: PASTE
			else if (GUI_CONTROL)
				key_char=(key_code='C')&31; // C-c: COPY
		}
		else if (key_code==GUI_DELETE)
		{
			if (GUI_SHIFT)
				key_char=(key_code='X')&31; // C-x: CUT
			else if (GUI_CONTROL)
				key_char=(key_code='Z')&31; // C-z: UNDO
		}
		update=0; if (key_code==GUI_MOUSE)
		{
			if (!mouse_py) // screen caption
			{
				key_code=mouse_px<40?GUI_F11:GUI_F12;
			}
			else if (mouse_py<GUI_HEIGHT-1)
			{
				if (mouse_px<25) // score panel
				{
					key_code=GUI_F2; selectscoreitem=activescoreitem=(mouse_px-3)/7;
					if (writeable) selectscore=activescore=activescore+mouse_py-(GUI_HEIGHT+1)/2;
				}
				else if (mouse_px<58) // instrument panel
				{
					key_code=GUI_F3; activeinstritem=(mouse_px-25-4);
					activeinstr=activeinstr+mouse_py-(GUI_HEIGHT+1)/2;
				}
				else // order panel
				{
					update=1; // update the score panel too!
					key_code=GUI_F4; selectorderitem=activeorderitem=(mouse_px-58-3)/6;
					if (writeable) selectorder=activeorder=activeorder+mouse_py-(GUI_HEIGHT+1)/2;
				}
			}
			else // parameter panel
			{
				if (mouse_px<36) // select a oparameter
				{
					key_code=GUI_F9; activeparameter=mouse_px<25?0:mouse_px<30?1:2;
				}
				else if (mouse_px<58-5) // pause/resume
				{
					key_code=snd_playing?GUI_F8:GUI_F7;
				}
				else
					key_code=GUI_F1; // help!
			}
		}
		switch (key_code)
		{
			case GUI_F1: // HELP
				if (GUI_CONTROL)
					update=16,snd_bool[0]=snd_bool[1]=snd_bool[2]=1;
				else update=gui_readme(
					"\n-=- " MY_NAME " (C) " MY_SELF " -=-\n" "\n"
					"F2\tgo to pattern panel\t+ ^PU\tnext order\n"
					"F3\tgo to instrument panel\t- ^PD\tpast order\n"
					"F4\tgo to order list panel\t* ^Rt\tnext octave\n"
					"F5\tplay song\t\t/ ^Lt\tpast octave\n"
					"F6\tplay pattern\t\t> ^Dn\tnext instrument\n"
					"F7\tplay from cursor\t< ^Up\tpast instrument\n"
					"F8\tstop playback\t\tENTER\tget note instrument\n"
					"F9\tgo to parameter panel\tSPACE\tset note instrument\n"
					"F11\tedit song title\t\tF12\tedit description\n"
					"^F1..F4\ttoggle channel on or off (Shift: solo channel)\n"
					"^Q/A\traise/lower by a semitone (Shift: by an octave)\n"
					"^E/D\tlook for repeated elements (Shift: extend/erase)\n"
					"^T/G\tshift selected notes, instrs. or patterns up/down\n"
					"^W\tlevel transposition\t^U\tlook for repeated data\n"
					"^K\tset last note/instr./pattern\t^P\ttoggle follow\n"
					"^L\tset 'loop to' pattern\t^Z\tundo\t^Y\tredo\n"
					"^X\tcut\t^C\tcopy\t^V\tpaste\t^B\tappend\n"
					"^N\tnew song\t^O\tload song\tF1\thelp\n"
					"^R\treload song\t^S\tsave song\tESC\tquit\n"
					//" S D   G H J   2 3   5 6 7   9 0        1   Rest    .   Clear\n"
					//"Z X C V B N M Q W E R T Y U I O P       L   C-9 (white noise)\n"
					);
			case GUI_MOUSE: // dummy
				break;
			case GUI_F2: // GO TO PATTERN PANEL
			case GUI_F3: // GO TO INSTRUMENT PANEL
			case GUI_F4: // GO TO ORDER LIST PANEL
				i=key_code-GUI_F2; // assuming GUI_F4=GUI_F3+1=GUI_F2+2
				if (GUI_CONTROL)
				{
					update=16;
					if (GUI_SHIFT)
					{
						snd_bool[0]=snd_bool[1]=snd_bool[2]=0;
						snd_bool[i]=1;
					}
					else
						snd_bool[i]=!snd_bool[i];
				}
				else
				{
					update=(1<<activepanel)|(1<<i)|16;
					activepanel=i;
				}
				break;
			case GUI_F9: // GO TO PARAMETER PANEL
				update=(1<<activepanel)|16;
				activepanel=3;
				break;
			case GUI_F5: // PLAY FULL SONG
			case GUI_F6: // PLAY PATTERN
			case GUI_F7: // PLAY FROM CURSOR
				if (hsnd)
				{
					if (key_code==GUI_F7&&snd_playing&&flag_pp)
						snd_looping=0;
					else if (key_code==GUI_F6&&snd_playing&&flag_pp)
						snd_looping=1;
					else
					{
						snd_stop();
						snd_order=key_code==GUI_F5?(GUI_CONTROL&&loopto>0?loopto:0):activeorder;
						snd_score=key_code!=GUI_F7?0:activescore;
						snd_play(key_code==GUI_F6);
					}
				}
				break;
			case GUI_F8: // STOP PLAYBACK
				snd_stop();
				break;
			case GUI_F11: // EDIT SONG TITLE
				gui_prompt(title1,80-2,1,0);//39,1
				update=32;
				break;
			case GUI_F12: // EDIT DESCRIPTION
				gui_prompt(title2,80-2,1,0);//39,41
				update=32;
				break;
			case GUI_UP:
				if (GUI_CONTROL)
				{
					update=2;
					--activeinstr;
				}
				else switch (activepanel)
				{
					case 0:
						if (writeable)
						{
							update=1;
							--activescore;
							if (!(GUI_SHIFT))
							{
								selectscore=activescore;
								selectscoreitem=activescoreitem;
							}
						}
						break;
					case 1:
						update=2;
						--activeinstr;
						break;
					case 2:
						if (writeable)
						{
							update=4|1;
							--activeorder;
							if (!(GUI_SHIFT))
							{
								selectorder=activeorder;
								selectorderitem=activeorderitem;
							}
						}
						break;
					case 3:
						switch (activeparameter)
						{
							case 0:
								++transpose;
								break;
							case 1:
								hertz*=2;
								break;
							case 2:
								++divisor;
								break;
						}
						break;
				}
				break;
			case GUI_DOWN:
				if (GUI_CONTROL)
				{
					update=2;
					++activeinstr;
				}
				else switch (activepanel)
				{
					case 0:
						if (writeable)
						{
							update=1;
							++activescore;
							if (!(GUI_SHIFT))
							{
								selectscore=activescore;
								selectscoreitem=activescoreitem;
							}
						}
						break;
					case 1:
						update=2;
						++activeinstr;
						break;
					case 2:
						if (writeable)
						{
							update=4|1;
							++activeorder;
							if (!(GUI_SHIFT))
							{
								selectorder=activeorder;
								selectorderitem=activeorderitem;
							}
						}
						break;
					case 3:
						switch (activeparameter)
						{
							case 0:
								--transpose;
								break;
							case 1:
								hertz/=2;
								break;
							case 2:
								--divisor;
								break;
						}
						break;
				}
				break;
			case GUI_LEFT:
				if (GUI_CONTROL)
				{
					--activeoctave;
				}
				else switch (activepanel)
				{
					case 0:
						update=1;
						--activescoreitem;
						if (!(GUI_SHIFT))
						{
							selectscore=activescore;
							selectscoreitem=activescoreitem;
						}
						break;
					case 1:
						update=2;
						--activeinstritem;
						break;
					case 2:
						update=4;
						--activeorderitem;
						if (!(GUI_SHIFT))
						{
							selectorder=activeorder;
							selectorderitem=activeorderitem;
						}
						break;
					case 3:
						--activeparameter;
						break;
				}
				break;
			case GUI_RIGHT:
				if (GUI_CONTROL)
				{
					++activeoctave;
				}
				else switch (activepanel)
				{
					case 0:
						update=1;
						++activescoreitem;
						if (!(GUI_SHIFT))
						{
							selectscore=activescore;
							selectscoreitem=activescoreitem;
						}
						break;
					case 1:
						update=2;
						++activeinstritem;
						break;
					case 2:
						update=4;
						++activeorderitem;
						if (!(GUI_SHIFT))
						{
							selectorder=activeorder;
							selectorderitem=activeorderitem;
						}
						break;
					case 3:
						++activeparameter;
						break;
				}
				break;
			case GUI_PRIOR:
				if (GUI_CONTROL)
				{
					if (writeable)
					{
						selectorder=--activeorder;
						selectorderitem=activeorderitem;
						update=4|1;
					}
					break;
				}
				update=1<<activepanel;
				switch (activepanel)
				{
					case 0:
						if (writeable)
						{
							activescore-=16;
							if (!(GUI_SHIFT))
							{
								selectscore=activescore;
								selectscoreitem=activescoreitem;
							}
						}
						break;
					case 1:
						activeinstr-=16;
						break;
					case 2:
						if (writeable)
						{
							update|=1;
							activeorder-=16;
							if (!(GUI_SHIFT))
							{
								selectorder=activeorder;
								selectorderitem=activeorderitem;
							}
						}
						break;
					case 3:
						switch (activeparameter)
						{
							case 0:
								transpose+=12;
								break;
							case 1:
								hertz=100;
								break;
							case 2:
								divisor+=10;
								break;
						}
						break;
				}
				break;
			case GUI_NEXT:
				if (GUI_CONTROL)
				{
					if (writeable)
					{
						selectorder=++activeorder;
						selectorderitem=activeorderitem;
						update=4|1;
					}
					break;
				}
				update=1<<activepanel;
				switch (activepanel)
				{
					case 0:
						if (writeable)
						{
							activescore+=16;
							if (!(GUI_SHIFT))
							{
								selectscore=activescore;
								selectscoreitem=activescoreitem;
							}
						}
						break;
					case 1:
						activeinstr+=16;
						break;
					case 2:
						if (writeable)
						{
							update|=1;
							activeorder+=16;
							if (!(GUI_SHIFT))
							{
								selectorder=activeorder;
								selectorderitem=activeorderitem;
							}
						}
						break;
					case 3:
						switch (activeparameter)
						{
							case 0:
								transpose-=12;
								break;
							case 1:
								hertz=25;
								break;
							case 2:
								divisor-=10;
								break;
						}
						break;
				}
				break;
			case GUI_HOME:
				update=1<<activepanel;
				switch (activepanel)
				{
					case 0:
						if (writeable)
						{
							activescore=0;
							if (!(GUI_SHIFT))
							{
								selectscore=activescore;
								selectscoreitem=activescoreitem;
							}
						}
						break;
					case 1:
						activeinstr=1;
						break;
					case 2:
						if (writeable)
						{
							update|=1;
							activeorder=/*GUI_ALT?loopto:*/0;
							if (!(GUI_SHIFT))
							{
								selectorder=activeorder;
								selectorderitem=activeorderitem;
							}
						}
						break;
					case 3:
						activeparameter=0;
						break;
				}
				break;
			case GUI_END:
				update=1<<activepanel;
				switch (activepanel)
				{
					case 0:
						if (writeable)
						{
							activescore=scrln[order[0][activeorder][0]]-1;
							if (!(GUI_SHIFT))
							{
								selectscore=activescore;
								selectscoreitem=activescoreitem;
							}
						}
						break;
					case 1:
						activeinstr=instrs-1;
						break;
					case 2:
						if (writeable)
						{
							update|=1;
							activeorder=orders-1;
							if (!(GUI_SHIFT))
							{
								selectorder=activeorder;
								selectorderitem=activeorderitem;
							}
						}
						break;
					case 3:
						activeparameter=3-1;
						break;
				}
				break;

			case GUI_UNDO: // BACKSPACE
				update=(1<<activepanel)|16;
				switch (activepanel)
				{
					case 0:
						if (writeable)
							--activescore;
						selectscore=activescore;
						selectscoreitem=activescoreitem;
						break;
					case 1:
						if (activeinstr>0&&activeinstritem>0)
							if (--activeinstritem<16)//-1
								if (writeable)
								{
									undo_set();
									memmove(&instr[activeinstr][activeinstritem+8],
										&instr[activeinstr][activeinstritem+9],16-1-activeinstritem);
									instr[activeinstr][8+16-1]=' ';
								}
						break;
					case 2:
						update|=1;
						if (writeable)
							--activeorder;
						selectorder=activeorder;
						selectorderitem=activeorderitem;
						break;
					case 3:
						--activeparameter;
						break;
				}
				break;
			case GUI_TAB:
				update=(1<<activepanel)|16;
				if (GUI_CONTROL)
				{
					if (GUI_SHIFT)
						update|=1<<(activepanel=(activepanel-1+4)%4);
					else
						update|=1<<(activepanel=(activepanel+1)%4);
				}
				else if (GUI_SHIFT)
					switch (activepanel)
					{
						case 0:
							selectscore=activescore;
							selectscoreitem=--activescoreitem;
							break;
						case 1:
							activeinstritem=activeinstritem<16+1?0:16;
							break;
						case 2:
							selectorder=activeorder;
							selectorderitem=--activeorderitem;
							break;
						case 3:
							--activeparameter;
							break;
					}
				else
					switch (activepanel)
					{
						case 0:
							selectscoreitem=++activescoreitem;
							selectscore=activescore;
							break;
						case 1:
							activeinstritem=activeinstritem<16+1?16+1:16+11;
							break;
						case 2:
							selectorderitem=++activeorderitem;
							selectorder=activeorder;
							break;
						case 3:
							++activeparameter;
							break;
					}
				break;
			case GUI_INSERT:
			case GUI_DELETE:
				#define INTMINMAX(x,y,a,b) ((a<=b)?(x=a,y=b):(x=b,y=a)) // where `x`, `y`, `a` and `b` are variables
				#define INTMINMAX_SCORE (INTMINMAX(m,n,activescore,selectscore),INTMINMAX(o,p,activescoreitem,selectscoreitem))
				#define INTMINMAX_ORDER (INTMINMAX(m,n,activeorder,selectorder),INTMINMAX(o,p,activeorderitem,selectorderitem))
				if (writeable)
				{
					k=(key_code==GUI_DELETE)?-1:+1;
					switch (activepanel)
					{
						case 0:
							update=1;
							undo_set();
							INTMINMAX_SCORE;
							for (j=o;j<=p;++j)
							{
								l=order[j][activeorder][0];
								if (k>0)
								{
									memmove(score[l][n+1],score[l][m],(128-1-n)*sizeof(score[l][m]));
									memset(score[l][m],0,(n-m+1)*sizeof(score[l][m]));
								}
								else
								{
									memmove(score[l][m],score[l][n+1],(128-1-n)*sizeof(score[l][m]));
									memset(score[l][128-1-n+m],0,(n-m+1)*sizeof(score[l][m]));
								}
							}
							break;
						case 1:
							if (k<0&&activeinstr>0&&activeinstritem<16)
								if (writeable)
								{
									update=2;
									undo_set();
									memmove(&instr[activeinstr][activeinstritem+8],
										&instr[activeinstr][activeinstritem+9],16-1-activeinstritem);
									instr[activeinstr][8+16-1]=' ';
								}
							break;
						case 2:
							if ((k>0&&orders<256)||(k<0&&orders>=activeorder))
							{
								update=4|1;
								undo_set();
								INTMINMAX_ORDER;
								for (j=o;j<=p;++j)
									if (k>0)
									{
										memmove(order[j][n+1],order[j][m],(256-1-n)*sizeof(order[l][m]));
										//memset(order[j][m],*?*,...);
									}
									else
									{
										memmove(order[j][m],order[j][n+1],(256-1-n)*sizeof(order[l][m]));
										//memset(order[j][256-1-n+m],*?*,...);
									}
								if (p-o==2)
								{
									orders+=k*(1+n-m);
									if (m<loopto)
										loopto+=k*(1+n-m);
								}
							}
							break;
					}
				}
				break;
			case GUI_ENTER:
				switch (activepanel)
				{
					case 0:
						update=2,activeinstr=score[order[activescoreitem][activeorder][0]][activescore][1];
						break;
					case 1:
						if (activeinstritem>16)
							snd_testi=activeinstr,snd_testd=0,snd_testn=CHIPNSFX_SIZE-3; // SFX: C-9
						break;
				}
				break;
			#ifdef SDL2
			case GUI_MOUSE-1: // DROPFILE!
				update=-1;
				if (!dirty||gui_really(gui_load_song))
				{
					snd_stop();
					if (!loadsong(gui_string))
						filepath[0]=0,gui_readme(sprintferror(txt_load_error));
					else
					{
						undo_set();
						dirty=can_undo=can_redo=activescore=selectscore=activeorder=selectorder=0;
						activeinstr=1;
						gui_title(filepath);
					}
				}
				break;
			#endif
			default:
				switch (key_char)
				{
					case  1: // C-a: LOWER BY SEMITONE/OCTAVE
					case 17: // C-q: RAISE BY SEMITONE/OCTAVE
						if (writeable)
						{
							k=(key_char==1)?-1:+1;
							if (GUI_SHIFT)
								k*=12;
							update=activepanel==2?4:1;
							if (activepanel<3)
								undo_set();
							switch (activepanel)
							{
								case 0:
									INTMINMAX_SCORE;
									for (j=o;j<=p;++j)
										for (i=m,l=order[j][activeorder][0];i<=n;++i)
											score[l][i][0]=addnote(score[l][i][0],k);
									break;
								case 1:
									for (i=0;i<256;++i)
										for (j=0;j<128;++j)
											if (score[i][j][1]==activeinstr)
												score[i][j][0]=addnote(score[i][j][0],k);
									break;
								case 2:
									INTMINMAX_ORDER;
									for (j=o;j<=p;++j)
										for (i=m;i<=n;++i)
											order[j][i][1]=intmax(intmin(k+(signed char)order[j][i][1],+99),-99);
									break;
								case 3:
									transpose+=k;
									break;
							}
						}
						break;
					case 23: // C-w: "FLATTEN" SCORE TRANSPOSITION
						switch (activepanel)
						{
							case 3:
								update=transposesong();
								break;
							case 0:
							case 2:
								if (activepanel==0)
									m=n=activeorder,INTMINMAX(o,p,activescoreitem,selectscoreitem);
								else
									INTMINMAX_ORDER;
								for (j=o;j<=p;++j)
									for (i=m;i<=n;++i)
									{
										k=order[j][i][0];
										if (l=(signed char)order[j][i][1])
										{
											if (!update)
												undo_set();
											for (z=0;z<128;++z)
												score[k][z][0]=addnote(score[k][z][0],l);
											for (z=0;z<orders;++z)
											{
												if (order[0][z][0]==k)
													order[0][z][1]-=l;
												if (order[1][z][0]==k)
													order[1][z][1]-=l;
												if (order[2][z][0]==k)
													order[2][z][1]-=l;
											}
											update=4|1;
										}
									}
								break;
						}
						break;
					case 11: // C-k: SET LAST SCORE/INSTR/ORDER
						update=1<<activepanel;
						switch (activepanel)
						{
							case 0:
								update|=4; // fix "00+00!01+00!02+00"
								scrln[order[0][activeorder][0]]=
									scrln[order[1][activeorder][0]]=
									scrln[order[2][activeorder][0]]=
									activescore+1;
								break;
							case 1:
								instrs=activeinstr+1;
								break;
							case 2:
								orders=activeorder+1;
								break;
						}
						break;
					case 12: // C-l: SET "LOOPTO"
						update=4;
						loopto=loopto==activeorder?-1:activeorder;
						break;
					case 10: // C-j [!]
						#ifdef SDL2
						/*if (GUI_SHIFT)
							--gui_weight;
						else*/
							++gui_weight;
						gui_mkfont(); update=-1;
						#endif
					case  9: // C-i [!]
					case  8: // C-h [!]
						break;
					case 13: // C-m: MISCELLANEOUS, ETC.
						if (activepanel==0)
							if (GUI_SHIFT) // selected notes = 1st NOTE of active INSTR
							{
								if (writeable)
								{
									undo_set();
									INTMINMAX(m,n,activescore,selectscore);
									memcpy(tmp,score[order[activescoreitem][activeorder][0]][m],(l=(n-m+1))*sizeof(score[0][0]));
									for (j=0;j<256;++j)
										for (i=0;i<(128+1-l);++i)
										{
											z=score[j][i][0]-tmp[0];
											for (k=0;k<l;++k)
											{
												if (score[j][i+k][1]!=tmp[k*sizeof(score[0][0])+sizeof(score[0][0][0])])
													break;
												x=score[j][i+k][0]; y=tmp[k*sizeof(score[0][0])];
												if (!x||x>(CHIPNSFX_SIZE-4)||!y||y>(CHIPNSFX_SIZE-4))
												{
													if (x!=y)
														break;
												}
												else if ((x-y)!=z)
													break;
											}
											if (k>=l)
											{
												k=addnote(score[j][i][0],12*(activeoctave-4));
												memset(score[j][i],0,l*sizeof(score[0][0]));
												score[j][i][0]=k;
												score[j][i][1]=(!k||k>CHIPNSFX_SIZE-2)?0:activeinstr;
											}
										}
									update=1;
								}
							}
							else // DETECT ARPEGGIO
							{
								if (activescore+2<scrln[i=order[activescoreitem][activeorder][0]])
								{
									j=score[i][activescore+0][0];
									k=score[i][activescore+1][0]-j;
									l=score[i][activescore+2][0]-j;
									if (k>0&&k<=24&&l>=0&&l<=24)
										update=gui_readmef("\nArpeggio 00-%02i-%02i.\n",k,l);
								}
							}
						break;
					case 14: // C-n: NEW SONG
						update=-1;
						if (!dirty||gui_really("New song"))
						{
							snd_stop();
							newsong();
							undo_set();
							filepath[0]=dirty=can_undo=can_redo=activescore=selectscore=activeorder=selectorder=0;
							activeinstr=1;
							gui_title(filepath);
						}
						break;
					case 15: // C-o: LOAD SONG...
						update=-1;
						if (!dirty||gui_really(gui_load_song))
						{
							if (gui_browse(filepath,0,gui_chp_ext,gui_load_song))
							{
								snd_stop();
								if (!loadsong(filepath))
									filepath[0]=0,gui_readme(sprintferror(txt_load_error));
								else
								{
									undo_set();
									dirty=can_undo=can_redo=activescore=selectscore=activeorder=selectorder=0;
									activeinstr=1;
									gui_title(filepath);
								}
							}
						}
						break;
					case 18: // C-r: RELOAD SONG
						update=-1;
						if (filepath[0]&&(!dirty||gui_really("Reload song")))
						{
							snd_stop();
							if (!loadsong(filepath))
								filepath[0]=0,gui_readme(sprintferror(txt_load_error));
							else
								undo_set(),dirty=can_undo=can_redo=0;
						}
						break;
					case 19: // C-s: SAVE SONG
						update=-1;
						if (gui_browse(filepath,1,gui_chp_ext,"Save song"))
						{
							if (!savesong(filepath))
								gui_readme(sprintferror(txt_save_error));
							else
								dirty=0,gui_title(filepath);
						}
						break;
					case  2: // C-b: PASTE AND ADVANCE
					case 22: // C-v: PASTE
						if (writeable)
							if (clipboardtype==activepanel&&clipboardwidth&&clipboarditems)
							{
								undo_set();
								switch (activepanel)
								{
									case 0:
										update=1;
										m=intmin(activescore,selectscore);
										o=intmin(activescoreitem,selectscoreitem);
										for (j=0;j<clipboarditems&&j+o<3;++j)
											for (i=0,l=order[j+o][activeorder][0];i<clipboardwidth&&i+m<128;++i)
												MEMSAVE(score[l][i+m],clipboard[j][i]);
										update|=extendscores(i+m);
										if (key_char!=22) // ADVANCE?
										{
											selectscore=activescore=i+m;
											selectscoreitem=activescoreitem=o;
										}
										break;
									case 2:
										update=4|1;
										m=intmin(activeorder,selectorder);
										o=intmin(activeorderitem,selectorderitem);
										for (i=0;i<clipboardwidth&&(l=i+m)<256;++i)
											for (j=0;j<clipboarditems&&j+o<3;++j)
												MEMSAVE(order[j+o][l],clipboard[j][i]);
										extendorders(i+m);
										if (key_char!=22) // ADVANCE?
										{
											selectorder=activeorder=i+m;
											selectorderitem=activeorderitem=o;
										}
										break;
								}
							}
						break;
					case  3: // C-c: COPY
					case 24: // C-x: CUT
						if (writeable)
						{
							clipboardtype=activepanel;
							switch (activepanel)
							{
								case 0:
									clipboardwidth=(n=intmax(activescore,selectscore))-(m=intmin(activescore,selectscore))+1;
									clipboarditems=(p=intmax(activescoreitem,selectscoreitem))-(o=intmin(activescoreitem,selectscoreitem))+1;
									for (j=0;j<clipboarditems;++j)
										for (i=0,l=order[j+o][activeorder][0];i<clipboardwidth;++i)
											MEMSAVE(clipboard[j][i],score[l][i+m]);
									if (key_char==24) // CUT?
									{
										update=1;
										undo_set();
										for (j=o;j<=p;++j)
											for (i=m,l=order[j][activeorder][0];i<=n;++i)
												score[l][i][0]=score[l][i][1]=0;
									}
									break;
								case 2:
									clipboardwidth=(n=intmax(activeorder,selectorder))-(m=intmin(activeorder,selectorder))+1;
									clipboarditems=(p=intmax(activeorderitem,selectorderitem))-(o=intmin(activeorderitem,selectorderitem))+1;
									for (i=0;i<clipboardwidth;++i)
										for (j=0,l=i+m;j<clipboarditems;++j)
											MEMSAVE(clipboard[j][i],order[j+o][l]);
									if (key_char==24) // CUT?
									{
										update=4;
										undo_set();
										for (i=m;i<=n;++i)
											for (j=o;j<=p;++j)
												order[j][i][0]=order[j][i][1]=0;
									}
									break;
							}
						}
						break;
					case 20: // C-t: SHIFT UP
					case  7: // C-g: SHIFT DOWN
						if (writeable)
						{
							k=(key_char==20)?-1:+1;
							switch (activepanel)
							{
								case 0:
									INTMINMAX_SCORE;
									if ((k<0&&m+k>=0)||(k>0&&n<scrln[order[activescoreitem][activeorder][0]]-k))
									{
										update=1;
										undo_set();
										if (GUI_SHIFT)
											k=k<0?-m:scrln[order[activescoreitem][activeorder][0]]-n-1;
										int gap,src,tgt;
										if (k<0)
											gap=-k,src=m+k,tgt=n+1+k;
										else
											gap=k,src=n+1,tgt=m;
										for (j=o;j<=p;++j)
										{
											l=order[j][activeorder][0];
											memcpy(tmp,score[l][src],sizeof(score[l][m])*gap);
											memmove(score[l][m+k],score[l][m],sizeof(score[l][m])*(n-m+1));
											memcpy(score[l][tgt],tmp,sizeof(score[l][m])*gap);
										}
										selectscore+=k;
										activescore+=k;
									}
									break;
								case 1:
									if ((k<0&&activeinstr>0-k)||(k>0&&activeinstr>0&&activeinstr<instrs-k))
									{
										update=2+1;
										undo_set();
										if (GUI_SHIFT)
										{
											if (k<0)
												k=1-activeinstr;
											else
												k=instrs-activeinstr-1;
										}
										int gap,src,tgt;
										if (k<0)
											gap=-k,src=activeinstr+k,tgt=activeinstr+1+k;
										else
											gap=k,src=activeinstr+1,tgt=activeinstr;
										o=intmin(activeinstr,activeinstr+k);
										p=intmax(activeinstr,activeinstr+k);
										memcpy(tmp,instr[activeinstr],sizeof(instr[0]));
										memmove(instr[tgt],instr[src],sizeof(instr[0])*gap);
										memcpy(instr[activeinstr+k],tmp,sizeof(instr[0]));
										for (i=0;i<256;++i)
											for (j=0;j<128;++j)
												if ((l=score[i][j][1])==activeinstr)
													score[i][j][1]+=k;
												else if (l>=o&&l<=p)
													score[i][j][1]-=k<0?-1:1;
										activeinstr+=k;
									}
									break;
								case 2:
									INTMINMAX_ORDER;
									if ((k<0&&m+k>=0)||(k>0&&n<orders-k))
									{
										update=4;
										undo_set();
										if (GUI_SHIFT)
										{
											if (k<0)
												k=-m;
											else
												k=orders-n-1;
										}
										int gap,src,tgt;
										if (k<0)
											gap=-k,src=m+k,tgt=n+1+k;
										else
											gap=k,src=n+1,tgt=m;
										for (j=o;j<=p;++j)
										{
											memcpy(tmp,order[j][src],sizeof(order[j][m])*gap);
											memmove(order[j][m+k],order[j][m],sizeof(order[j][m])*(n-m+1));
											memcpy(order[j][tgt],tmp,sizeof(order[j][m])*gap);
										}
										selectorder+=k;
										activeorder+=k;
									}
									break;
							}
						}
						break;
					case 25: // C-y: REDO
						if (writeable) update=-1,undo_get(1);
						break;
					case 26: // C-z: UNDO
						if (writeable) update=-1,undo_get(0);
						break;
					case  4: // C-d: DETECT USELESS DATA
						switch (activepanel)
						{
							case 1:
								if (activeinstr>0&&activeinstr<instrs)
								{
									j=dupe_instr_test(activeinstr);
									if (GUI_SHIFT) // remove unused instrument
									{
										if (j<256)
											;
										else
										{
											strcln(&instr[activeinstr][8]);
											for (i=j=0;i<9;++i)
												j|=instr[activeinstr][i];
											if (j) // not empty?
											{
												undo_set();
												MEMZERO(instr[activeinstr]);
												update=2;
											}
										}
									}
									else
										update=dupe_instr_show(j);
								}
								break;
							case 0:
							case 2:
								if (activeorder<orders)
								{
									// LOOK FOR IDENTICAL SCORES
									if (activepanel==0)
										m=n=activeorder,INTMINMAX(o,p,activescoreitem,selectscoreitem);
									else
										INTMINMAX_ORDER;
									for (j=o;j<=p;++j)
										for (i=m;i<=n;++i)
										{
											k=order[j][i][0];
											for (z=0;z<m;++z)
											{
												if (k!=(y=order[0][z][0])&&(l=scrln[y])==scrln[k]&&(x=comparescores(score[y],score[k],l))!=128)
													break;
												if (k!=(y=order[1][z][0])&&(l=scrln[y])==scrln[k]&&(x=comparescores(score[y],score[k],l))!=128)
													break;
												if (k!=(y=order[2][z][0])&&(l=scrln[y])==scrln[k]&&(x=comparescores(score[y],score[k],l))!=128)
													break;
											}
											if (z<m)
											{
												if (GUI_SHIFT) // merge identical pattern
												{
													if (!update)
														undo_set();
													order[j][i][0]=y;
													order[j][i][1]+=x;
													update=4|1;
												}
												else
												{
													update=gui_readmef("\nPattern #%02X is equal to #%02X%+03i.\n",k,y,(signed char)(order[j][i][1]+x));
													i=j=256; // force break
												}
											}
										}
									if (!update)
										update=gui_readme("\nNo previous duplicates.\n");
								}
								break;
						}
						break;
					case  5: // C-e: EXTEND CURRENT DATA
						switch (activepanel)
						{
							case 1:
								if (activeinstr>0&&activeinstr<instrs)
								{
									j=dupe_instr_test(activeinstr);
									if (GUI_SHIFT) // clone instrument
									{
										if (j<256)
										{
											if (instrs<255)
											{
												undo_set();
												MEMSAVE(instr[instrs],instr[activeinstr]);
												activeinstr=instrs++;
												update=2;
											}
											else
												update=gui_readme("\nToo many instruments!\n");
										}
									}
									else
										update=dupe_instr_show(j);
								}
								break;
							case 0:
							case 2:
								if (activeorder<orders)
								{
									// LOOK FOR SHARED REFERENCES
									if (activepanel==0)
										m=n=activeorder,INTMINMAX(o,p,activescoreitem,selectscoreitem);
									else
										INTMINMAX_ORDER;
									for (j=o;j<=p;++j)
										for (i=m;i<=n;++i)
										{
											k=order[j][i][0];
											if (GUI_SHIFT) // FORCE!
												z=-1;
											else
												for (z=0;z<m;++z)
													if (k==order[0][z][0]||k==order[1][z][0]||k==order[2][z][0])
														break;
											if (z<m)
											{
												if (GUI_SHIFT)
												{
													// LOOK FOR UNUSED PATTERNS
													MEMZERO(save_hits);
													for (y=0;y<256;++y)
														save_hits[order[0][y][0]]=save_hits[order[1][y][0]]=save_hits[order[2][y][0]]=1;
													for (y=0;y<256;++y)
														if (!save_hits[y])
															break;
													if (y<256)
													{
														// EXTEND!
														if (!update)
															undo_set();
														MEMSAVE(score[y],score[k]);
														scrln[y]=scrln[k];
														order[j][i][0]=y;
														update=4|1;
													}
													else
													{
														update=gui_readme("\nToo many patterns!\n");
														i=j=256; // force break
													}
												}
												else
												{
													update=gui_readmef("\nPattern first used in order #%02X.\n",z);
													i=j=256; // force break
												}
											}
										}
									if (!update)
										update=gui_readme("\nNo previous references.\n");
								}
								break;
						}
						break;
					case 21: // C-u: PURGE ALL REPEATED PATTERNS, SORT AND CLEAN ORDERS
						switch (activepanel)
						{
							case 3:
								update=transposesong();
								break;
							case 1:
								for (k=1,p=0;k<instrs;++k)
									if (dupe_instr_test(k)<256)
										;
									else
									{
										strcln(&instr[k][8]);
										for (i=j=0;i<9;++i)
											j|=instr[k][i];
										if (j) // not empty?
										{
											if (GUI_SHIFT)
											{
												if (!p)
													undo_set();
												MEMZERO(instr[k]);
											}
											++p;
										}
									}
								if (GUI_SHIFT)
									update=2;
								else
									update=gui_readmef("\n%i unused instruments.\n",p);
								break;
							case 0:
								// how many times is each pattern used?
								MEMZERO(save_hits);
								for (j=0;j<3;++j)
									for (i=0;i<orders;++i)
										++save_hits[order[j][i][0]];
								// FIND AND MERGE DUPLICATES: warning, not always optimal!
								for (o=p=0,i=1;i<256;++i)
									if (save_hits[i])
										for (j=0;j<i;++j)
											if (save_hits[j])
												if ((l=scrln[i])&&l==scrln[j]&&(m=comparescores(score[i],score[j],l))!=128)
													for (++o,k=0;k<orders;++k)
														for (n=0;n<3;++n)
															if (order[n][k][0]==i)
															{
																if (GUI_SHIFT)
																{
																	if (!p)
																		undo_set();
																	order[n][k][0]=j;
																	order[n][k][1]-=m;
																	if (!--save_hits[i])
																	{
																		MEMZERO(score[i]);
																		scrln[i]=0;
																	}
																}
																++p;
															}
								if (GUI_SHIFT)
									update=1|4;
								else
									update=gui_readmef("\n%i pattern duplicates.\n",p);
								break;
							case 2:
								// LOOK FOR CLASHES AND FIX THEM, IF ANY
								for (j=k=0;j<3;++j)
									for (i=0;i<orders;++i)
										if ((l=order[j][i][0])>k)
											i=j=256; // force break
										else if (l==k)
										{
											if ((signed char)order[j][i][1])
												i=j=256; // force break
											else
												++k;
										}
								if (GUI_SHIFT)
								{
									if (j>3)
									{
										undo_set();
										for (j=k=0;j<3;++j)
											for (i=0;i<orders;++i)
											{
												if ((l=order[j][i][0])>k) // sort entries
												{
													memswp(score[l],score[k],sizeof(score[0]));
													m=scrln[l];
													scrln[l]=scrln[k];
													scrln[k]=m;
													for (p=0;p<3;++p)
														for (o=0;o<orders;++o)
															if ((m=order[p][o][0])==k)
																order[p][o][0]=l;
															else if (m==l)
																order[p][o][0]=k;
													l=k;
												}
												if (l==k)
												{
													if (n=((signed char)order[j][i][1])) // fix transpositions
													{
														for (m=0;m<128;++m)
															score[l][m][0]=addnote(score[l][m][0],n);
														for (m=0;m<orders;++m)
														{
															if (order[0][m][0]==l)
																order[0][m][1]-=n;
															if (order[1][m][0]==l)
																order[1][m][1]-=n;
															if (order[2][m][0]==l)
																order[2][m][1]-=n;
														}
													}
													++k;
												}
											}
										update=4|1;
									}
								}
								else
									update=gui_readmef("\nOrders are %s sorted.\n",(j>3)?"already":"not");
								break;
						}
						break;
					case  6: // C-f: TOGGLE FLAG_ZZ
						++flag_zz;
						break;
					case 16: // C-p: TOGGLE FLAG_PP
						if (GUI_SHIFT)
							flag_xx=!flag_xx;
						else
							flag_pp=!flag_pp;
						//break;
					case 27: // C-[ (ESC)
					case 28: // C-\ (FS)
					case 29: // C-] (GS)
					case 30: // C-^ (RS)
					case 31: // C-_ (US)
					case  0: // C-@ <NUL>
						break;
					default: // text characters
						if (writeable)
						{
							k=ucase(key_char);
							switch (activepanel)
							{
								case 0:
									if (k==' ') // set instrument on notes
									{
										undo_set();
										INTMINMAX_SCORE;
										for (j=o;j<=p;++j)
											for (i=m,l=order[j][activeorder][0];i<=n;++i)
												if ((k=score[l][i][0])&&k<(CHIPNSFX_SIZE-2))
													score[l][i][1]=activeinstr;
										activescore=selectscore=i;
										activescoreitem=selectscoreitem=o;
										update=1;
									}
									else if ((n=strint(pianos,lcase(key_char)))>=0) // type note
									{
										if (n<12+12+6)
											k=n+activeoctave*12+(n<=12), // NOTE
											k=k<=(CHIPNSFX_SIZE-4)?k:(CHIPNSFX_SIZE-3); // SFX
										else if (n==12+12+6+0)
											k=CHIPNSFX_SIZE-3; // SFX
										else if (n==12+12+6+2)
											k=CHIPNSFX_SIZE-2; // BRAKE
										else if (n==12+12+6+1)
											k=CHIPNSFX_SIZE-0; // REST
										else
											k=0; // ERASE
										undo_set();
										update=(k&&k<(CHIPNSFX_SIZE-2))?activeinstr:0;
										INTMINMAX_SCORE;
										for (j=o;j<=p;++j)
											for (i=m,l=order[j][activeorder][0];i<=n;++i)
											{
												score[l][i][0]=k;
												score[l][i][1]=update;
											}
										snd_testn=k;
										snd_testd=(signed char)order[activescoreitem][activeorder][1];
										snd_testi=update;
										activescore=selectscore=i;
										activescoreitem=selectscoreitem=o;
										update=1|extendscores(activescore);
									}
									else if (i=charshortcuts(k))
										update=i;
									break;
								case 1:
									if (activeinstritem>16)
									{
										if (i=charshortcuts(k))
											update=i;
										else if ((l=validhex(k))>=0) // type instrument digits
										{
											update=2;
											undo_set();
											j=activeinstritem-17;
											if (j>7)
												++j;
											i=instr[activeinstr][j/2];
											if (j%2)
												i=(i&0xF0)+l;
											else
												i=(i&0x0F)+16*l;
											instr[activeinstr][j/2]=i;
											++activeinstritem;
											extendinstrs(activeinstr+1);
											instr[activeinstr][4]&=1;
										}
										else if (k==' ') // play A-octave
										{
											snd_testn=1+(9+12*activeoctave);
											snd_testd=0;
											snd_testi=activeinstr;
										}
									}
									else if (activeinstritem<16) // type instrument name
									{
										update=2;
										undo_set();
										strpad(&instr[activeinstr][8],16);
										memmove(&instr[activeinstr][activeinstritem+9],
											&instr[activeinstr][activeinstritem+8],16-1-activeinstritem);
										instr[activeinstr][activeinstritem+++8]=key_char;
										extendinstrs(activeinstr+1);
									}
									break;
								case 2:
									if ((l=validhex(k))>=0) // type order digits
									{
										update=4|1;
										undo_set();
										order[activeorderitem][activeorder][0]=
											order[activeorderitem][activeorder][0]*16+l;
										extendorders(activeorder+1);
									}
									else if (k==' ') // write instrument on patterns
									{
										update=4|1;
										undo_set();
										INTMINMAX_ORDER;
										for (j=o;j<=p;++j)
											for (i=m;i<=n;++i)
												for (l=0,z=order[j][i][0];l<128;++l)
													if ((k=score[z][l][0])&&k<(CHIPNSFX_SIZE-2))
														score[z][l][1]=activeinstr;
										activeorder=selectorder=i;
										activeorderitem=selectorderitem=o;
									}
									else if (i=charshortcuts(k))
										update=i;
									break;
								case 3:
									if (i=charshortcuts(k))
										update=i;
									break;
							}
						}
				}
		}
	}
	#endif
}
