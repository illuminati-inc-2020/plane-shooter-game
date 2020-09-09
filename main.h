/* Subject	: Shooting game
 * Author	: Rakesh Malik
 * Date		: 26/04/2011
 */

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#include "stopwatch.h"
#include "BMP.h"

#define WIDTH 500
#define HEIGHT 600

/* Structures */
#define SLOTS 100
struct Audio
{
    Uint8 *data;
    Uint32 dpos;
    Uint32 dlen;
} sounds[SLOTS];
#define	SLOT_MUSIC1		0,	0
#define	SLOT_MUSIC2		1,	1
#define	SLOT_BLAST		2,	9
#define	SLOT_FIRE		10,	94
#define	SLOT_POWERUP	95,	99

typedef struct Level		/* Level data structure */
{
  int bgns,bgnr;			/* Nuber of backgroung textures */
  GLuint bgtex[10];			/* Background Texture */
  char music[50];			/* Level music filename */
  int total_time;			/* Total time for level */
  int n;					/* Number of entry */
  int *time;				/* Time of arrival */
  int *plane_type;			/* Plane type */
  int *fire_type;			/* Fire type */
  int *gun_f;				/* Front gun */
  int *gun_af;				/* Additional front gun */
  int *gun_r;				/* Rear gun */
  int *gun_s;				/* Side gun */
  int *gun_sf;				/* Side front gun */
  int *gun_sr;				/* Side front gun */
  float *x,*y;				/* Arrival position */
  float *angle;				/* Arrival angle */
  float *rotation;			/* Rotating speed */
  int *powerup_type;		/* Power type carryied by enemy plane */
}Level;

typedef struct Fire			/* Structure for a Fire on screen */
{
  float x,y;				/* position on screen */
  int type;					/* Type */
  float angle;				/* Angle of firing */
  void *owner;				/* the plane who fired */
  int health;				/* 1=not hit yet, 0=hit */
  struct Fire *next;		/* Link to next fire */
}Fire;

typedef struct FireType		/* Structure for a fire type */
{
  float speed;				/* speed of fire */
  float size;				/* size of fire */
  int power;				/* Health it can take of a plane */
  int firing_delay;			/* fring delay */
  GLuint texture;			/* Texture */
  GLuint mask;				/* Mask texture */
  int sound;				/* BULLET/LASER/PLASMA/ROCKET */
}FireType;
enum FireSound{BULLET,LASER,PLASMA,ROCKET};

typedef struct Plane		/* Structure for a Plane on screen */
{
  float x,y;				/* Plane position on screen */
  int plane_type;			/* Plane type */
  int fire_type;			/* Plane fire type */
  float angle;				/* Plane angle */
  float rotation;			/* Plane angle changing rate (per loop) */
  int health;				/* Current health of plane */
  int firing_delay;			/* Current firing delay */
  char gun_f;				/* Front gun */
  char gun_af;				/* Additional front gun */
  char gun_r;				/* Rear gun */
  char gun_s;				/* Side gun */
  char gun_sf;				/* Side front gun */
  char gun_sr;				/* side rear gun */
  char powerup_type;		/* powerup_type hold by plane */
  struct Plane *next;		/* Pointer to next plane on screen */
}Plane;

typedef struct PlaneType	/* Structure for a plane type */
{
  float speed;  			/* Speed of plane */
  int max_firing_delay;		/* Minimum firing delay of plane (sec) */
  float power;				/* power of crash */
  float size;				/* Size of plane */
  int max_health;			/* Maximum health of plane*/
  GLuint texture;			/* Texture */
  GLuint mask;				/* Mask texture */
  int score;				/* Score for destroying */
}PlaneType;

typedef struct Crash		/* Structure for a crash to be printed */
{
  float x,y;				/* Position on screen */
  float size;				/* Current size */
  float maxsize;			/* Maximum size */
  struct Crash *next;		/* Link to next crash on screen */
}Crash;

typedef struct Powerup		/* Structure for a powerup on screen */
{
  float x,y;				/* Position on scren */
  char type;				/* Powerup type */
  int angle;				/* angle */
  float dirx,diry;			/* direction of movement */  
  int vanish_time;			/* time it vanishes */
  struct Powerup *next;		/* Link to next powerup on screen */
}Powerup;

typedef struct PowerupType	/* Structure for a powerup type */
{
  GLuint texture;			/* Powerup texture */
}PowerupType ;

typedef struct Boss			/* Structure for a boss plane */
{
  GLfloat x,y;				/* Position */
  GLfloat angle;			/* Angle */
  GLfloat size;				/* Size */ 
  GLfloat speed;			/* Speed */
  int health;				/* Current total health of boss */
  int maxhealth;			/* Maximum total health of boss */
  int ngun;					/* Number of guns */
  struct BossGun			/* Structure for a gun of boss */
  {
    GLfloat x,y;			/* Gun position */
    GLfloat size;			/* Gun size */
    GLfloat angle;			/* Gun firing angle */
    int fire_type;			/* Gun fire type */
    int maxhealth;			/* Gun maximum health */
    int health;				/* Gun health */
    int firing_delay;		/* Firing delay of gun */
  }
  gun[20];					/* Guns */
  int nstate;				/* Number of states */
  struct BossState			/* Structure for a state of boss */
  {
    GLfloat x,y;			/* Destination in current state */ 
    GLfloat angle;			/* Angle in current state */
    int duration;			/* Duration of current state */
    int gun_flag[20];		/* gun[i]=(1)activated/(0)deactivated for i=0(1)5 */
  }
  state[20];				/* States */
  int current_state;		/* Current state */
  int state_end_time;		/* time at which current state ends */
  GLuint texture;			/* Texture */
  GLuint mask;				/* Texture mask */
}Boss;

/* Variables */
char keyboeard_buffer[10];
int enable_player_fire=0;								/* =1(Player firing  enebled/0(Disabled */
int gameover;											/* =1(Game Over)/0(Not Game Over) */
int paused;												/* =1(Paused)/0(Resumed) */
GLfloat xdest,ydest;									/* Current destination player position */
int Cwidth,Cheight;										/* Current width and height of window */
unsigned long score; 									/* Score */
StopWatch level_time;									/* Time of current level */
unsigned long loop_number=0;							/* counts number of loops (used for generating random number */
Level level_data;										/* Level data */
int level;												/* Current level */
int level_entry_index;									/* current level entry index */
int level_complete;										/* =1(level complete)/0(level running) */
int level_complete_time;								/* level_time.time value when level is completed */
GLfloat bgpart;											/* Part of current background texture printed (0=none,1=full)*/
int bgnum;												/* Index of current background texture */
int game_complete=0;									/* =1(Game complete)/(0)game not complete */  
int planes;												/* Number of planes */
int bombs;												/* Number of bombs */
int bombing_delay;										/* Delay for repeated charging of bomb */
int armor;												/* Armor of plane */
int shield;												/* Remaining shield time (sec) */
int fn;													/* number of fire types */
FireType *fire;											/* Fire types */
Fire **fire_list;										/* Firing list */
GLuint bomb_tex,bomb_texmask;							/* Foreground bomb texture */
int pn;													/* number of fire types */
PlaneType *plane;										/* Plane types */
Plane player_plane;										/* laney plane structure */
Plane **plane_list;										/* Enemy plane list */
Crash **crash_list;										/* List of crashes */
GLuint crash_tex,crash_texmask;							/* Crash Texture */
GLuint smoke_tex[4],smoke_texmask[4];					/* Smoke Textures */
int pun;												/* number of powerup types */
Powerup **powerup_list;									/* List of powerups on screen */
PowerupType *powerup;									/* List of powerup types */
Boss boss;												/* Boss of current level */

GLuint font_tex[127],font_texmask[127];


/* Function Prototypes */
void display(void);												/* GLUT display function */
void reshape(int width, int height);							/* GLUT reshape function */
void mouse(int button, int state,int x, int y);					/* GLUT mouse function */
void keyboard(unsigned char key,int x, int y);					/* GLUT keyboard function */
void motion(int x, int y);										/* GLUT motion function */
void passivemotion(int x, int y);								/* GLUT passive motion function */

void mixaudio(void *unused, Uint8 *stream, int len);			/* Mixes the sounds in buffer */
void InitAudio(void);											/* Initializes SDL for audio */
void CloseAudio(void);											/* Closes SDL */
void PlaySound(char *file,int start_slot,int end_slot,int force);
																/* Plays a given sound */
void LoopSound(int start_slot,int end_slot);					/* Repeats sound if ended */

void load_font(void);											/* Loads font */
void GL_printf(GLfloat x,GLfloat y,GLfloat size,const char *fmt,...);
																/* Prints a string of given format */
void newgame(void);												/* Loads newgame */
void new_level(void);											/* Load new level */
void load_level_data(int lev);									/* Loads data of given level */
void load_plane_data(void);										/* Loads data of all planes */
void load_fire_data(void);										/* Loads data of all type of fires */
void load_crash_data(void);										/* Loads textures of crash */
void load_powerup_data(void);									/* Loads textures of powerups */
void load_boss_data(int level);
void load_texture(GLuint *texture,const char *name);			/* Loads a given texture */
void background(void);											/* Draws background */
void foreground(void);											/* Draws Foreground */
void reset_player(void);										/* Reset all attributes of player plane */
void handle_player(void);										/* Handle all player operations */
void handle_enemy(void);										/* Handles al enemy operations */
void handle_collision(void);									/* Handles plane-plane/fire-plane collision */
void handle_fire(void);											/* Handles all firing operations of player and enemy  */
void shoot_by(Plane *p);										/* Makes a plane shoot by all its activated plane */
void create_fire(GLfloat x,GLfloat y,int fire_type,GLfloat angle,void *owner);		
																/* Creates a fire */
void handle_powerup();											/* Handles powerups moving on screen */
void new_powerup(GLfloat x,GLfloat y,char powerup_type);		/* Generates new powerup on screen */
void crash(GLfloat x,GLfloat y,GLfloat size);					/* Adds new entry in crash list */
void take_powerup(int type);									/* Adds a powerup to player_plane */
void lost_powerup(void);										/* Generates lost powerups on screen */
void handle_shield(void);										/* Handles invincibility shield */
void charge_bomb(void);											/* Handles bomb */
void handle_boss(void);											/* Handles boss of current level */
void draw_plane(Plane p);										/* Draws a given plane on screen */
void draw_fire(Fire f);											/* Draws a given fire on screen */
void draw_crash(void);											/* Draws a crash */
void draw_powerup(Powerup pu);									/* Draws a powerup on screen */
void draw_boss(void);											/* Draws boss of current level */
void draw_smoke(GLfloat x,GLfloat y);							/* Draws smoke at a given point */
void vertex(GLfloat X,GLfloat Y,GLfloat size,GLfloat x,GLfloat y,GLfloat z);
																/* draws a vertex from texture */
void cheat();

int main()
{
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize (WIDTH,HEIGHT);
	glutInitWindowPosition (0,0);
	glutCreateWindow ("Sky Warrior");
	glutSetCursor(GLUT_CURSOR_CROSSHAIR);
	load_font();
	load_plane_data();
	load_fire_data();
	load_crash_data();
	load_powerup_data();
	InitAudio();	
	newgame();    
		
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(passivemotion);	
	glutMainLoop();
	CloseAudio();
	return 0;
}

void display(void)
{
	static unsigned long last_tick=0;
	unsigned long delay;
	delay=(SDL_GetTicks()-last_tick);
	SDL_Delay((delay>6)?0:6-delay);
	last_tick=SDL_GetTicks();
	
	loop_number=(loop_number+1)%0xffffffff;
    increment_time(&level_time);

    background();
    if(!paused)
    {
		if(!level_complete && level_time.time>level_data.total_time) 
			handle_boss();
		handle_fire();
		handle_player();
		handle_enemy();
		handle_collision();
		handle_powerup();
		handle_shield();
    }   
    foreground();
	//GL_printf(-.8,.8,.1,"%lu MS",delay);
	glFlush ();

	if(enable_player_fire)
		shoot_by(&player_plane);
	if(level_complete && level_time.time==level_complete_time+6)
		new_level();

	cheat();

	//LoopSound(SLOT_MUSIC1);
	
	glutPostRedisplay();
	glutSwapBuffers();
}


void reshape(int width, int height)
{
	glutReshapeWindow(WIDTH,HEIGHT);
	glViewport(0,0,width,height);
	Cwidth=WIDTH;
	Cheight=HEIGHT;
}

void mouse(int button, int state,int x, int y)
{
	switch(button)
	{
		case GLUT_LEFT_BUTTON:
			enable_player_fire=!enable_player_fire;
			if(paused)
			{
				paused=0;
				start_timer(&level_time);
			}
			break;
		case GLUT_RIGHT_BUTTON:
			if(!(paused||gameover||game_complete)) 
				charge_bomb(); 
			break;
	}
}

void keyboard(unsigned char key,int x, int y)
{
	memmove(keyboeard_buffer+1,keyboeard_buffer,9);
	keyboeard_buffer[0]=key;
	switch(key)
	{
		case 'p':
		case 27:
			if(!(gameover||paused||game_complete)) 
			{
				paused=1;
				stop_timer(&level_time);
			}
			else if(paused)
			{
				paused=0;
				start_timer(&level_time);
			}
			break;
		case 'n':
			if(gameover||paused)
				newgame();
			break;
		case 'q':
			if(game_complete||gameover||paused)
				exit(0);
			break;
		case 'r':
			if(paused)
			{
				paused=0;
				start_timer(&level_time);
			}
			break;
	}
}


void passivemotion(int x, int y)
{
	xdest=2.0*(GLdouble)x/(GLdouble)Cwidth-1.0;
	ydest=1.0-2.0*(GLdouble)y/(GLdouble)Cheight;
}

void motion(int x, int y)
{
	xdest=2.0*(GLdouble)x/(GLdouble)Cwidth-1.0;
	ydest=1.0-2.0*(GLdouble)y/(GLdouble)Cheight;
	if(!paused)
		shoot_by(&player_plane);
}

void cheat()
{
	if(strncmp(keyboeard_buffer,"ssobot",6)==0)
	{	
		level_time.time=level_data.total_time;
		memset(keyboeard_buffer,0,10);
	}
	else if(strncmp(keyboeard_buffer,"gnabgib",7)==0)
	{
		bombs++;
		charge_bomb();
		memset(keyboeard_buffer,0,10);
	}
	else if(strncmp(keyboeard_buffer,"elbicnivni",10)==0)
	{
		shield=3000;
	}
	else if(strncmp(keyboeard_buffer,"1",1)==0)//(strncmp(keyboeard_buffer,"leveltxen",9)==0)
	{	
		level_complete=1;
		memset(keyboeard_buffer,0,10);
	}
	else if(strncmp(keyboeard_buffer,"erifegnahc",10)==0)
	{
		player_plane.fire_type=(player_plane.fire_type+1)%fn;
		memset(keyboeard_buffer,0,10);
	}
	else if(strncmp(keyboeard_buffer,"snuglla",7)==0)
	{
		player_plane.gun_af=1;
		player_plane.gun_sf=1;
		player_plane.gun_s=1;
		player_plane.gun_r=1;
		player_plane.gun_sr=1;
		memset(keyboeard_buffer,0,10);
	}
	else if(strncmp(keyboeard_buffer,"dogehtmai",9)==0)
	{
		plane[0].max_firing_delay=0;
		planes=5;
		player_plane.health=100;
		armor=100;
		bombs=10;
		shield=3000;
		player_plane.fire_type=4;
		player_plane.gun_af=1;
		player_plane.gun_sf=1;
		player_plane.gun_s=1;
		player_plane.gun_r=1;
		player_plane.gun_sr=1;
		memset(keyboeard_buffer,0,10);
	}
}

void background(void)
{
	int bgnext;
	if(bgnum<level_data.bgns+level_data.bgnr-1) bgnext=bgnum+1;
	else bgnext=level_data.bgns;
  
	glLoadIdentity();
	glColor4f(1.0,1.0,1.0,1.0);
	glClearDepth(1.0f);
	glClearColor(1.0,1.0,1.0,1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,level_data.bgtex[bgnum]);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
		glTexCoord2f(0,0); 			glVertex3d(-1,+1-bgpart*2,.9);
		glTexCoord2f(1,0); 			glVertex3d(+1,+1-bgpart*2,.9);
		glTexCoord2f(1,1-bgpart); 	glVertex3d(+1,-1,.9);
		glTexCoord2f(0,1-bgpart); 	glVertex3d(-1,-1,.9);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,level_data.bgtex[bgnext]);
	glBegin(GL_QUADS);
		glTexCoord2f(0,0+1-bgpart);	glVertex3d(-1,+1,.9);
		glTexCoord2f(1,0+1-bgpart);	glVertex3d(+1,+1,.9);
		glTexCoord2f(1,1);	 		glVertex3d(+1,-1+(1-bgpart)*2,.9);
		glTexCoord2f(0,1); 			glVertex3d(-1,-1+(1-bgpart)*2,.9);
	glEnd();
	glDisable(GL_TEXTURE_2D);
  
	bgpart+=.001;
	if(bgpart>=1.0000)
	{
		bgpart=0.0;
		bgnum=bgnext;
	}
}

void foreground()
{
  GLfloat i,size,x,y;					// Foreground plane size and position 
  glLoadIdentity();
  
  if(paused)
  {
    GL_printf(-0.3,0.0,0.2,"PAUSED");
    GL_printf(-0.5,-0.3,0.1,"PRESS R TO RESUME");
    GL_printf(-0.5,-0.4,0.1,"PRESS N FOR NEWGAME");
    GL_printf(-0.5,-0.5,0.1,"PRESS Q TO QUIT");
    GL_printf( 0.0,-0.90,0.05,"BY RAKESH MALIK");
  }
  if(gameover)
  {
    GL_printf(-0.6,0.2,0.3,"GAME OVER");
    GL_printf(-0.5,-0.3,0.1,"PRESS N FOR NEWGAME");
    GL_printf(-0.5,-0.4,0.1,"PRESS Q TO QUIT");
    GL_printf( 0.0,-0.90,0.05,"BY RAKESH MALIK");
  }
  if(!game_complete && level_complete)
    GL_printf(-0.6,0.0,0.2,"LEVEL COMPLETE");
  if(game_complete)
  {
    GL_printf(-0.7, 0.5,0.2,"CONGRATULATIONS");
    GL_printf(-0.7, 0.3,0.1,"YOU COMPLETED THE GAME");
    GL_printf(-0.5,-0.3,0.1,"PRESS Q TO QUIT");
    GL_printf( 0.0,-0.90,0.05,"BY RAKESH MALIK");
  }
  if(level_time.time<=3) GL_printf(-0.3,0.5,0.2,"LEVEL %d",level);
    
  /* Printing Planes */
  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  glColor4f(1,1,1,1);
  size=.075;
  for(i=0;i<planes;i++)
  {  
    x=-0.85+i*0.1;
    y=-0.85;
    glBlendFunc(GL_DST_COLOR,GL_ZERO);
    glBindTexture(GL_TEXTURE_2D,plane[0].mask);
    glBegin(GL_QUADS);
      vertex(x,y,size,0,0,0);
      vertex(x,y,size,0,1,0);
      vertex(x,y,size,1,1,0);
      vertex(x,y,size,1,0,0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D,plane[0].texture);
    glBlendFunc(GL_ONE,GL_ONE);
    glBegin(GL_QUADS);
      vertex(x,y,size,0,0,0);
      vertex(x,y,size,0,1,0);
      vertex(x,y,size,1,1,0);
      vertex(x,y,size,1,0,0);
    glEnd();
  }
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  
  /* Printing Bombs */
  glColor4f(1,1,1,1);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  for(i=0;i<bombs;i++)
  {
    x=0.90-i*0.1;
    y=-0.90;
    glBlendFunc(GL_DST_COLOR,GL_ZERO);
    glBindTexture(GL_TEXTURE_2D,bomb_texmask);
    glBegin(GL_QUADS);
      vertex(x,y,size,0,0,0);
      vertex(x,y,size,0,1,0);
      vertex(x,y,size,1,1,0);
      vertex(x,y,size,1,0,0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D,bomb_tex);
    glBlendFunc(GL_ONE,GL_ONE);
    glBegin(GL_QUADS);
      vertex(x,y,size,0,0,0);
      vertex(x,y,size,0,1,0);
      vertex(x,y,size,1,1,0);
      vertex(x,y,size,1,0,0);
    glEnd();
  }
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  
  /* Printing score */
  GL_printf( 0.5,-0.8,0.1,"%9d",score); 
  
  /* Printing Health and Armor */
  glEnable(GL_BLEND);
  glShadeModel(GL_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE);
  glBegin(GL_QUADS);
    /* Health */
    glColor4f(1.0,0.0,0.0,0.5);																		glVertex3d(-0.90,-0.90,-0.9);
    glColor4f(1.0,0.0,0.0,0.5);																		glVertex3d(-0.90,-0.95,-0.9);
    glColor4f(1.0-(GLfloat)player_plane.health/100.0,(GLfloat)player_plane.health/100.0,0.0,0.5);	glVertex3d(-0.90+(GLfloat)player_plane.health*.5/100,-0.95,-0.9);
    glColor4f(1.0-(GLfloat)player_plane.health/100.0,(GLfloat)player_plane.health/100.0,0.0,0.5);	glVertex3d(-0.90+(GLfloat)player_plane.health*.5/100,-0.90,-0.9);
    /* Drained Health */
    glColor4f(0.5,0.5,0.5,0.5);		glVertex3d(-0.90+(GLfloat)player_plane.health*.5/100,-0.90,-0.9);
    glColor4f(0.5,0.5,0.5,0.5);		glVertex3d(-0.90+(GLfloat)player_plane.health*.5/100,-0.95,-0.9);
    glColor4f(0.5,0.5,0.5,0.0);		glVertex3d(-0.40,-0.95,-0.9);
    glColor4f(0.5,0.5,0.5,0.0);		glVertex3d(-0.40,-0.90,-0.9);
    /* Armor */
    glColor4f(0.5,0.5,0.5,1.0);		glVertex3d(-0.90,-0.95,- 0.9);
    glColor4f(0.5,0.5,0.5,1.0);		glVertex3d(-0.90,-0.96,- 0.9);
    glColor4f(0.5,0.5,0.5,1.0);		glVertex3d(-0.90+(GLfloat)armor*.5/100,-0.96,-0.9);
    glColor4f(0.5,0.5,0.5,1.0);		glVertex3d(-0.90+(GLfloat)armor*.5/100,-0.95,-0.9);
  glEnd();
  glDisable(GL_BLEND);
  
  /* Printing Boss Health */
  if(level_time.time>=level_data.total_time)
  {
    glEnable(GL_BLEND);
    glShadeModel(GL_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glBegin(GL_QUADS);
      /* Health */
      glColor4f(1.0,0.0,0.0,0.5);									glVertex3d(-0.90, 0.90,-0.9);
      glColor4f(1.0,0.0,0.0,0.5);									glVertex3d(-0.90, 0.95,-0.9);
      glColor4f(1.0-(GLfloat)boss.health/boss.maxhealth,(GLfloat)boss.health/boss.maxhealth,0.0,0.5);	glVertex3d(-0.90+(GLfloat)boss.health/boss.maxhealth, 0.95,-0.9);
      glColor4f(1.0-(GLfloat)boss.health/boss.maxhealth,(GLfloat)boss.health/boss.maxhealth,0.0,0.5);	glVertex3d(-0.90+(GLfloat)boss.health/boss.maxhealth, 0.90,-0.9);
      /* Drained Health */
      glColor4f(0.5,0.5,0.5,0.5);		glVertex3d(-0.90+(GLfloat)boss.health/boss.maxhealth, 0.90,-0.9);
      glColor4f(0.5,0.5,0.5,0.5);		glVertex3d(-0.90+(GLfloat)boss.health/boss.maxhealth, 0.95,-0.9);
      glColor4f(0.5,0.5,0.5,0.0);		glVertex3d( 0.10, 0.95,-0.9);
      glColor4f(0.5,0.5,0.5,0.0);		glVertex3d( 0.10, 0.90,-0.9);
    glEnd();
    glDisable(GL_BLEND);
  }
}

void load_texture(GLuint *texture,const char *name)
{	
	BMP image;
	load_BMP(&image,name);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1,texture);
	glBindTexture(GL_TEXTURE_2D,*texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST); 
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,image.width,image.height,0,GL_RGB,GL_UNSIGNED_BYTE,image.data);
	free_BMP(&image);
}

void load_level_data(int lev)
{
	FILE *fp;
	char garbage,fname[50],fpath[100];
	int i;

	sprintf(fname,"data/lev%d",lev);
  
	if((fp=fopen(fname,"r"))==NULL)
	{
		game_complete=1;
		return;
	}
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%d",&level_data.bgns);
	for(i=0;i<level_data.bgns;i++)
	{
		fscanf(fp,"%s",fname);
		sprintf(fpath,"image/background/%s",fname);
		load_texture(&level_data.bgtex[i],fpath);
	}
	fscanf(fp,"%d",&level_data.bgnr);
	for(;i<level_data.bgns+level_data.bgnr;i++)
	{
		fscanf(fp,"%s",fname);
		sprintf(fpath,"image/background/%s",fname);
		load_texture(&level_data.bgtex[i],fpath);
	}	
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%s",fname);
	sprintf(level_data.music,"audio/%s",fname);	
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%d%d",&level_data.total_time,&level_data.n);
	level_data.time			=	(int*)		realloc(level_data.time,		level_data.n*sizeof(int));
	level_data.plane_type	=	(int*)		realloc(level_data.plane_type,	level_data.n*sizeof(int));
	level_data.fire_type	=	(int*)		realloc(level_data.fire_type,	level_data.n*sizeof(int));
	level_data.gun_f		=	(int*)		realloc(level_data.gun_f,		level_data.n*sizeof(int));
	level_data.gun_af		=	(int*)		realloc(level_data.gun_af,		level_data.n*sizeof(int));
	level_data.gun_r		=	(int*)		realloc(level_data.gun_r,		level_data.n*sizeof(int));
	level_data.gun_s		=	(int*)		realloc(level_data.gun_s,		level_data.n*sizeof(int));
	level_data.gun_sf		=	(int*)		realloc(level_data.gun_sf,		level_data.n*sizeof(int));
	level_data.gun_sr		=	(int*)		realloc(level_data.gun_sr,		level_data.n*sizeof(int));
	level_data.x			=	(float*)	realloc(level_data.x,			level_data.n*sizeof(float));
	level_data.y			=	(float*)	realloc(level_data.y,			level_data.n*sizeof(float));  
	level_data.angle		=	(float*)	realloc(level_data.angle,		level_data.n*sizeof(float));
	level_data.rotation		=	(float*)	realloc(level_data.rotation,		level_data.n*sizeof(float));
	level_data.powerup_type	=	(int*)		realloc(level_data.powerup_type,level_data.n*sizeof(int));
	while((garbage=getc(fp))!=')');
	for(i=0;i<level_data.n;i++)
	{
		fscanf(fp,"%d",&level_data.time[i]);
		fscanf(fp,"%d",&level_data.plane_type[i]);
		fscanf(fp,"%d",&level_data.fire_type[i]);
		fscanf(fp,"%d%d%d%d%d%d",&level_data.gun_f[i],&level_data.gun_af[i],&level_data.gun_r[i],&level_data.gun_s[i],&level_data.gun_sf[i],&level_data.gun_sr[i]);
		fscanf(fp,"%f%f",&level_data.x[i],&level_data.y[i]);
		fscanf(fp,"%f",&level_data.angle[i]);
		fscanf(fp,"%f",&level_data.rotation[i]);
		fscanf(fp,"%d",&level_data.powerup_type[i]);
	}
	fclose(fp);

	load_boss_data(lev);
}

void load_fire_data()
{
	FILE *fp;	
	char garbage,str[50],fpath[100];
	int i;

	/* initialization of list */
	fire_list=(Fire**)malloc(sizeof(Fire*));
	*fire_list=NULL;
  
	/* Fire type definitions */  
	if((fp=fopen("data/fire","r"))==NULL)
	{
		MessageBox(NULL,"Error","File \"data/fire\" missing",MB_ICONWARNING);
		exit(1);
	}
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%d",&fn);
	fire=(FireType*)malloc(sizeof(FireType)*fn);  
	while((garbage=getc(fp))!=')');
	for(i=0;i<fn;i++)
	{
		fscanf(fp,"%f%f%d%d%s",	&fire[i].speed,
								&fire[i].size,
								&fire[i].power,
								&fire[i].firing_delay,
								str);
		sprintf(fpath,"image/fire/%s",str);
		load_texture(&fire[i].texture,fpath);
		fscanf(fp,"%s",str);
		sprintf(fpath,"image/fire/%s",str);
		load_texture(&fire[i].mask,fpath);
		fscanf(fp,"%s",str);
		if(strcmp(str,"bullet")==0) fire[i].sound=BULLET;
		else if(strcmp(str,"laser")==0) fire[i].sound=LASER;
		else if(strcmp(str,"plasma")==0) fire[i].sound=PLASMA;
		else fire[i].sound=ROCKET;
	}
	fclose(fp);
  
	/* Foreground bomb texture */
	load_texture(&bomb_texmask,"image/bomb/bomb_mask.bmp");
	load_texture(&bomb_tex,"image/bomb/bomb.bmp");
}

void handle_fire()
{
	Fire **temp,*del;
	Plane *enemy;
	for(temp=fire_list;*temp;)
	{
		/* Deleting fire */
		if((*temp)->health<=0 || (*temp)->x>=1.5 || (*temp)->x<=-1.5 || (*temp)->y>=1.5 || (*temp)->y<=-1.5)
		{
			del=*temp;
			*temp=(*temp)->next;
			free(del);
			continue;
		}    
		/* Updating fire */
		draw_fire(**temp);
		(*temp)->x+=sin(-(*temp)->angle*3.141592654/180)*fire[(*temp)->type].speed;
		(*temp)->y+=cos(-(*temp)->angle*3.141592654/180)*fire[(*temp)->type].speed;
		temp=&(*temp)->next;
	}
  
	/* New fire by enemy */
	for(enemy=*plane_list;enemy;enemy=enemy->next)
		if(enemy->firing_delay>=plane[enemy->plane_type].max_firing_delay + fire[enemy->fire_type].firing_delay)
			shoot_by(enemy);
    
    bombing_delay--;
}

void shoot_by(Plane *p)
{
	if(p->firing_delay<plane[p->plane_type].max_firing_delay + fire[p->fire_type].firing_delay)
		return;
  
	p->firing_delay=0;
	if(p->gun_f==1)
		create_fire(p->x,p->y,p->fire_type,p->angle,p);
	if(p->gun_af==1)
	{
		create_fire(p->x,p->y,p->fire_type,p->angle+5,p);
		create_fire(p->x,p->y,p->fire_type,p->angle-5,p);
	}
	if(p->gun_r==1)
	{
		create_fire(p->x,p->y,p->fire_type,p->angle+180,p);
		create_fire(p->x,p->y,p->fire_type,p->angle+165,p);
		create_fire(p->x,p->y,p->fire_type,p->angle-165,p);
	}
	if(p->gun_s==1)
	{
		create_fire(p->x,p->y,p->fire_type,p->angle+90,p);
		create_fire(p->x,p->y,p->fire_type,p->angle-90,p);
	}
	if(p->gun_sf==1)
	{
		create_fire(p->x,p->y,p->fire_type,p->angle+45,p);
		create_fire(p->x,p->y,p->fire_type,p->angle-45,p);
	}
	if(p->gun_sr==1)
	{
		create_fire(p->x,p->y,p->fire_type,p->angle+135,p);
		create_fire(p->x,p->y,p->fire_type,p->angle-135,p);
	}
	switch(fire[p->fire_type].sound)
	{
		case BULLET:
			PlaySound("audio/bullet.wav",SLOT_FIRE,0);
			break;
		case LASER:
			PlaySound("audio/laser.wav",SLOT_FIRE,0);
			break;
		case PLASMA:
			PlaySound("audio/plasma.wav",SLOT_FIRE,0);
			break;
		case ROCKET:
			PlaySound("audio/rocket.wav",SLOT_FIRE,0);
			break;
	}
}

void create_fire(GLfloat x,GLfloat y,int fire_type,GLfloat angle,void *owner)
{
  Fire *new_fire=(Fire*)malloc(sizeof(Fire));
  new_fire->x=x;
  new_fire->y=y;
  new_fire->type=fire_type;
  new_fire->angle=angle;
  new_fire->owner=owner;
  new_fire->health=1;
  new_fire->next=*fire_list;
  *fire_list=new_fire;
}

void draw_fire(Fire f)
{
  GLfloat x=f.x,y=f.y;
  GLfloat size=fire[f.type].size;
  glLoadIdentity();
  glTranslatef(x,y,0.2);
  glRotatef(f.angle,0,0,1);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_DST_COLOR,GL_ZERO);
  glBindTexture(GL_TEXTURE_2D,fire[f.type].mask);
  glBegin(GL_QUADS);
    vertex(0,0,size,0,0,0);
    vertex(0,0,size,0,1,0);
    vertex(0,0,size,1,1,0);
    vertex(0,0,size,1,0,0);
  glEnd();
  glBindTexture(GL_TEXTURE_2D,fire[f.type].texture);
  glBlendFunc(GL_ONE,GL_ONE);
  glBegin(GL_QUADS);
    vertex(0,0,size,0,0,0);
    vertex(0,0,size,0,1,0);
    vertex(0,0,size,1,1,0);
    vertex(0,0,size,1,0,0);
  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

void charge_bomb()
{  
	int i;
	Plane *p;
	Fire *f;
	if(bombs==0 || bombing_delay>0) return;
	bombing_delay=100;
	bombs--; 
	// all plane health reduced by 100
	for(p=*plane_list;p;p=p->next)
		p->health-=100;	
	// all fires removed
	for(f=*fire_list;f;f=f->next) 
		f->health=0;
	// all boss guns health reduced by 100
	if(level_time.time>level_data.total_time)
	{
		for(i=0;i<boss.ngun;i++)
		{
			boss.gun[i].health=(boss.gun[i].health<=100)?0:boss.gun[i].health-100;
		}
	}
	shield+=1;
	PlaySound("audio/bomb.wav",SLOT_BLAST,0);
	crash(player_plane.x,player_plane.y,2.0);
}

void load_plane_data()
{
	FILE *fp;
	char garbage,fname[50],fpath[100];
	int i;
  
	/* initialization of lists */
	plane_list=(Plane**)malloc(sizeof(Plane*));
	*plane_list=NULL;
  
	/* Plane Type definitions */  
	if((fp=fopen("data/plane","r"))==NULL)
	{
		MessageBox(NULL,"Error","File \"data/plane\" missing",MB_ICONWARNING);
		exit(1);
	}
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%d",&pn);
	plane=(PlaneType*)malloc(sizeof(PlaneType)*pn);  
	while((garbage=getc(fp))!=')');
	for(i=0;i<pn;i++)
	{
		fscanf(fp,"%f%d%f%f%d%s",	&plane[i].speed,
									&plane[i].max_firing_delay,
									&plane[i].power,
									&plane[i].size,
									&plane[i].max_health,fname);
		sprintf(fpath,"image/plane/%s",fname);
		load_texture(&plane[i].texture,fpath);
		fscanf(fp,"%s%d",fname,&plane[i].score);
		sprintf(fpath,"image/plane/%s",fname);
		load_texture(&plane[i].mask,fpath);
	}
	fclose(fp); 
  
	reset_player();  
}

void reset_player()
{
  player_plane.x=0.0;
  player_plane.y=-1.5;
  player_plane.plane_type=0;
  player_plane.fire_type=0;
  player_plane.angle=0;
  player_plane.rotation=0;
  player_plane.health=plane[0].max_health;
  player_plane.firing_delay=0;  
  player_plane.gun_f=1;
  player_plane.gun_af=0;
  player_plane.gun_r=0;
  player_plane.gun_s=0;
  player_plane.gun_sf=0;
  player_plane.gun_sr=0;
  player_plane.powerup_type=0;
  player_plane.next=NULL;  
  shield+=3;
}

void handle_player()
{
	GLfloat x,y,speed,size;				/* Current plane position,speed,size */
	if(gameover) return;
	if(player_plane.health==0)
	{
		crash(player_plane.x,player_plane.y,plane[0].size);
		if(planes==0) gameover=1;
		else
		{
			reset_player();
			planes--;
		}
	}
	
	player_plane.firing_delay++;
  
	x=player_plane.x,y=player_plane.y;
	speed=plane[0].speed;
	size=plane[0].size;
    
	/* Rotation */
	glLoadIdentity();  
	glTranslatef(x,y,0.0);
	if(xdest>x) glRotatef((x-xdest)*100, 0.0, 1.0, 0.0);  
	else if(xdest<x) glRotatef((xdest-x)*100, 0.0,-1.0, 0.0);
	if(ydest>y) glRotatef((y-ydest)*50,-1.0,0.0,0.0);
	else if(ydest<y) glRotatef((ydest-y)*50, 1.0,0.0, 0.0);
	glTranslatef(-x,-y,0.0);
  
	/* Updating position */
	if(xdest<x+speed && xdest>x-speed) x=xdest;
	else if(xdest>x) x=x+speed;
	else if(xdest<x) x=x-speed;
	if(ydest<y+speed && ydest>y-speed) y=ydest;
	else if(ydest>y) y=y+speed;
	else if(ydest<y) y=y-speed;
	player_plane.x=x;
	player_plane.y=y;
  
	/* Drawing */
	draw_plane(player_plane);
} 

void handle_enemy()
{
	Plane **temp,*del;
	for(temp=plane_list;*temp;)
	{
		/* Deleting Plane */
		if((*temp)->health<=0 || (*temp)->x>1.5 || (*temp)->x<-1.5 || (*temp)->y>1.5 || (*temp)->y<-1.5)
		{
			if((*temp)->health<=0)
			{
				score+=plane[(*temp)->plane_type].score;
				crash((*temp)->x,(*temp)->y,plane[(*temp)->plane_type].power+plane[(*temp)->plane_type].size);
				new_powerup((*temp)->x,(*temp)->y,(*temp)->powerup_type);
			}
			del=*temp;
			*temp=(*temp)->next;
			free(del);
			continue;
		}
		/* Updating Plane */   
		glLoadIdentity();
		(*temp)->firing_delay++;
		draw_plane(**temp);
		(*temp)->x+=sin(-(*temp)->angle*3.141592654/180)*plane[(*temp)->plane_type].speed;
		(*temp)->y+=cos(-(*temp)->angle*3.141592654/180)*plane[(*temp)->plane_type].speed-.001;
		(*temp)->angle+=(*temp)->rotation;
		temp=&(*temp)->next;
	}
  
	/* New enemy arrival */
	if(level_entry_index< level_data.n && level_time.time==level_data.time[level_entry_index])
	{
		Plane *new_plane=(Plane*)malloc(sizeof(Plane));
		new_plane->x=level_data.x[level_entry_index];
		new_plane->y=level_data.y[level_entry_index];
		new_plane->plane_type=level_data.plane_type[level_entry_index];
		new_plane->fire_type=level_data.fire_type[level_entry_index];
		new_plane->angle=level_data.angle[level_entry_index];
		new_plane->rotation=level_data.rotation[level_entry_index];
		new_plane->health=plane[new_plane->plane_type].max_health;
		new_plane->firing_delay=0;
		new_plane->gun_f=level_data.gun_f[level_entry_index];
		new_plane->gun_af=level_data.gun_af[level_entry_index];
		new_plane->gun_r=level_data.gun_r[level_entry_index];
		new_plane->gun_s=level_data.gun_s[level_entry_index];
		new_plane->gun_sf=level_data.gun_sf[level_entry_index];
		new_plane->gun_sr=level_data.gun_sr[level_entry_index];
		new_plane->powerup_type=level_data.powerup_type[level_entry_index];
		new_plane->next=*plane_list;
		*plane_list=new_plane;
		level_entry_index++;
	}
}

void draw_plane(Plane p)
{
	GLfloat size=plane[p.plane_type].size;
  
	glTranslatef(p.x,p.y,0.0);
	glRotatef(p.angle, 0.0, 0.0, 1.0);
	glEnable(GL_TEXTURE_2D);
	switch(p.plane_type)
	{
		case 0:
			glEnable(GL_DEPTH_TEST);
			glBindTexture(GL_TEXTURE_2D,plane[0].texture);
			glBegin(GL_TRIANGLE_STRIP);
				vertex(0,0,size,0.512,0.060, 0.00);
				vertex(0,0,size,0.570,0.345, 0.00);
				vertex(0,0,size,0.518,0.370,-0.20);
				vertex(0,0,size,0.550,0.758, 0.00);
				vertex(0,0,size,0.508,0.824,-0.20);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				vertex(0,0,size,0.512,0.060, 0.00);
				vertex(0,0,size,0.467,0.345, 0.00);
				vertex(0,0,size,0.518,0.370,-0.20);
				vertex(0,0,size,0.475,0.758, 0.00);
				vertex(0,0,size,0.508,0.824,-0.20);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				vertex(0,0,size,0.512,0.060, 0.00);
				vertex(0,0,size,0.643,0.300, 0.00);
				vertex(0,0,size,0.580,0.344, 0.00);
				vertex(0,0,size,0.640,0.500, 0.00);
				vertex(0,0,size,0.548,0.780, 0.00);
				vertex(0,0,size,0.962,0.691, 0.05);
				vertex(0,0,size,1.000,0.752, 0.05);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP );
				vertex(0,0,size,0.512,0.060, 0.00);
				vertex(0,0,size,0.372,0.278, 0.00);
				vertex(0,0,size,0.460,0.312, 0.00);
				vertex(0,0,size,0.380,0.525, 0.00);
				vertex(0,0,size,0.468,0.791, 0.00);
				vertex(0,0,size,0.047,0.765, 0.10);
				vertex(0,0,size,0.020,0.812, 0.10);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				vertex(0,0,size,0.795,0.474, 0.00);
				vertex(0,0,size,0.851,0.553, 0.00);
				vertex(0,0,size,0.792,0.600, 0.00);
				vertex(0,0,size,0.810,0.790, 0.00);   
				vertex(0,0,size,0.755,0.825, 0.00);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				vertex(0,0,size,0.795,0.474,-0.10);
				vertex(0,0,size,0.738,0.561, 0.00);
				vertex(0,0,size,0.792,0.600,-0.20);
				vertex(0,0,size,0.715,0.760, 0.00);   
				vertex(0,0,size,0.755,0.825,-0.20);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				vertex(0,0,size,0.223,0.520,-0.10);
				vertex(0,0,size,0.274,0.611, 0.00);
				vertex(0,0,size,0.217,0.641,-0.20);
				vertex(0,0,size,0.307,0.785, 0.00);   
				vertex(0,0,size,0.270,0.860,-0.20);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				vertex(0,0,size,0.223,0.520,-0.10);
				vertex(0,0,size,0.170,0.607, 0.00);
				vertex(0,0,size,0.217,0.641,-0.20);
				vertex(0,0,size,0.217,0.834, 0.00);   
				vertex(0,0,size,0.270,0.860,-0.20);
			glEnd();
			glBegin(GL_LINES);
				vertex(0,0,size,0.274,0.854,-0.20);
				vertex(0,0,size,0.323,1.000, 0.00);
				vertex(0,0,size,0.755,0.824,-0.20);
				vertex(0,0,size,0.717,1.000, 0.00);
			glEnd();   
			glDisable(GL_DEPTH_TEST);
			break;
		default:
			glEnable(GL_BLEND);
			glBlendFunc(GL_DST_COLOR,GL_ZERO);
			glBindTexture(GL_TEXTURE_2D,plane[p.plane_type].mask);
			glBegin(GL_QUADS);
				vertex(0,0,size,0,0,0);
				vertex(0,0,size,0,1,0);
				vertex(0,0,size,1,1,0);
				vertex(0,0,size,1,0,0);
			glEnd();
			glBindTexture(GL_TEXTURE_2D,plane[p.plane_type].texture);
			glBlendFunc(GL_ONE,GL_ONE);
			glBegin(GL_QUADS);
				vertex(0,0,size,0,0,0);
				vertex(0,0,size,0,1,0);
				vertex(0,0,size,1,1,0);
				vertex(0,0,size,1,0,0);
			glEnd();
			glDisable(GL_BLEND);
			break;
	}
	glDisable(GL_TEXTURE_2D);
}
void load_crash_data()
{
	int i;
	char fname[30];

	/* initialization of list */
	crash_list=(Crash**)malloc(sizeof(Crash*));
	*crash_list=NULL;
  
	/* Crash Textures */
	load_texture(&crash_tex,"image/crash/crash.bmp");
	load_texture(&crash_texmask,"image/crash/crash_mask.bmp");
  
	/* Smoke Texture */  
	for(i=0;i<4;i++)
	{
		sprintf(fname,"image/smoke/smoke%d.bmp",i);
		load_texture(&smoke_tex[i],fname);
		sprintf(fname,"image/smoke/smoke%d_mask.bmp",i);
		load_texture(&smoke_texmask[i],fname);
	}
}

void handle_collision()
{    
	Fire *f;
	Plane *p;

	draw_crash();

	for(f=*fire_list;f;f=f->next)
	{
		/* Player fire hits enemy plane */
		for(p=*plane_list;p;p=p->next)
		{
			if(f->owner==&player_plane
					&& f->x+fire[f->type].size/2 >= p->x-plane[p->plane_type].size/4 
					&& f->x-fire[f->type].size/2 <= p->x+plane[p->plane_type].size/4 
					&& f->y+fire[f->type].size/2 >= p->y-plane[p->plane_type].size/4 
					&& f->y-fire[f->type].size/2 <= p->y+plane[p->plane_type].size/4)
			{
				if(fire[f->type].sound==BULLET)
					PlaySound("audio/hit.wav",SLOT_FIRE,0);
				p->health-=fire[f->type].power;
				p->health=(p->health<0)?0:p->health;
				f->health=0;
				if(fire[f->type].power>20) 
					crash(f->x,f->y,fire[f->type].size);
			}      
		}
		/* Enemy fire hits player plane */
		if(!gameover && f->owner!=&player_plane
					&& f->x+fire[f->type].size/2 >= player_plane.x-plane[player_plane.plane_type].size/4 
					&& f->x-fire[f->type].size/2 <= player_plane.x+plane[player_plane.plane_type].size/4
					&& f->y+fire[f->type].size/2 >= player_plane.y-plane[player_plane.plane_type].size/4 
					&& f->y-fire[f->type].size/2 <= player_plane.y+plane[player_plane.plane_type].size/4)
		{
			if(fire[f->type].sound==BULLET)
					PlaySound("audio/hit.wav",SLOT_FIRE,0);
			if(shield==0)
			{
				if(armor>0)
				{
					armor-=fire[f->type].power;
					if(armor<0)
					{
						player_plane.health+=armor;
						player_plane.health=(player_plane.health<0)?0:player_plane.health;	
						armor=0;
					}
				}
				else
				{
					player_plane.health-=fire[f->type].power;
					player_plane.health=(player_plane.health<0)?0:player_plane.health;
				}
			}
			f->health=0;
			if((shield==0 && fire[f->type].power>=10) || player_plane.health==0) 
				lost_powerup();
			if(fire[f->type].power>20)
				crash(f->x,f->y,fire[f->type].size);
		}
	}
  
	/* Enemy plane hits player plane */
	for(p=*plane_list;!gameover && p;p=p->next)
	{
		if(p->x >= player_plane.x-plane[p->plane_type].size/4-plane[0].size/4 
				&& p->x <= player_plane.x+plane[p->plane_type].size/4+plane[0].size/4
				&& p->y >= player_plane.y-plane[p->plane_type].size/4-plane[0].size/4 
				&& p->y <= player_plane.y+plane[p->plane_type].size/4+plane[0].size/4)
		{
			if(shield==0)
			{
				if(armor>0)
				{
					armor-=plane[p->plane_type].max_health;
					if(armor<0)
					{
						player_plane.health+=armor;
						player_plane.health=(player_plane.health<0)?0:player_plane.health;	
						armor=0;
					}
				}
				else
				{
					player_plane.health-=2*plane[p->plane_type].max_health;
					player_plane.health=(player_plane.health<0)?0:player_plane.health;
				}
				lost_powerup();
			}
			p->health-=2*plane[0].max_health;	
		}
	}
}

void crash(GLfloat x,GLfloat y,GLfloat size)
{
	Plane *temp;
	Crash *new_crash=(Crash*)malloc(sizeof(Crash));
	new_crash->x=x;
	new_crash->y=y;
	new_crash->size=0.01;
	new_crash->maxsize=size;
	new_crash->next=*crash_list;
	*crash_list=new_crash;
	for(temp=*plane_list;temp;temp=temp->next)
	{
		if(	temp->x<=x+size/2 &&
			temp->x>=x-size/2 &&
			temp->y<=y+size/2 &&
			temp->y>=y-size/2 )
		{
			temp->health-=size*100;
			temp->health=(temp->health<0)?0:temp->health;
		}
	}
	if(	shield==0 &&
		player_plane.x<=x+size/2 &&
		player_plane.x>=x-size/2 &&
		player_plane.y<=y+size/2 &&
		player_plane.y>=y-size/2 )
	{
		armor-=size*100;
		if(armor<0)
			player_plane.health+=armor;
		armor=(armor<0)?0:armor;
		player_plane.health=(player_plane.health<0)?0:player_plane.health;
	}
	if(size<.2)
		PlaySound("audio/blast0.wav",SLOT_BLAST,0);
	if(size<.5)
		PlaySound("audio/blast1.wav",SLOT_BLAST,0);
	if(size<1)
		PlaySound("audio/blast2.wav",SLOT_BLAST,0);
	else
		PlaySound("audio/blast3.wav",SLOT_BLAST,0);
}

void draw_crash()
{
	Crash **temp,*del;
	GLfloat x,y;

	for(temp=crash_list;*temp;)
	{
		/* Updating list */
		if((*temp)->size>=(*temp)->maxsize)
		{
			del=(*temp);
			*temp=(*temp)->next;
			free(del);
			continue;
		}
		(*temp)->size+=(*temp)->maxsize/50;
    
		/* Drawing Big crash */
		if((*temp)->maxsize>=1)
		{
			glLoadIdentity();
			glTranslatef((*temp)->x,(*temp)->y,0);
			glRotatef((*temp)->size*(loop_number%10)*36,0,0,1);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);
			glBegin(GL_POLYGON);
	
			for(x=-(*temp)->size;x<=(*temp)->size;x+=.01)
			{
				y=sqrt((*temp)->size*(*temp)->size-x*x);
				glColor4f(1.0,1.0,0.0,0.8);		glVertex3d(x,y,-0.9);
			}
			for(x=(*temp)->size;x>=-(*temp)->size;x-=.01)
			{
				y=sqrt((*temp)->size*(*temp)->size-x*x);
				glColor4f(1.0,0.0,0.0,0.8);		glVertex3d(x,-y,-0.9);
			}
			glEnd();
			glDisable(GL_BLEND);
		}    
		/* Drawing small crash*/
		else
		{
			glLoadIdentity();
			glColor4f(1,1,1,1);
			glTranslatef((*temp)->x,(*temp)->y,0);
			glRotatef((*temp)->size*(*temp)->x*(*temp)->y*(loop_number%10)*3600,0,0,1);
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_DST_COLOR,GL_ZERO);
			glBindTexture(GL_TEXTURE_2D,crash_texmask);
			glBegin(GL_QUADS);
				vertex(0,0,(*temp)->size,0,0,0);
				vertex(0,0,(*temp)->size,0,1,0);
				vertex(0,0,(*temp)->size,1,1,0);
				vertex(0,0,(*temp)->size,1,0,0);
			glEnd();
			glBindTexture(GL_TEXTURE_2D,crash_tex);
			glBlendFunc(GL_ONE,GL_ONE);
			glBegin(GL_QUADS);
				vertex(0,0,(*temp)->size,0,0,0);
				vertex(0,0,(*temp)->size,0,1,0);
				vertex(0,0,(*temp)->size,1,1,0);
				vertex(0,0,(*temp)->size,1,0,0);
			glEnd();
			glDisable(GL_BLEND);
			glDisable(GL_TEXTURE_2D);
		}   
		temp=&(*temp)->next;
	}
}

void draw_smoke(GLfloat x,GLfloat y)
{
	GLfloat size=.1;
	int index=(loop_number%64)/16;

	glLoadIdentity();
	glColor4f(1,1,1,1);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR,GL_ZERO);
	glBindTexture(GL_TEXTURE_2D,smoke_texmask[index]);
	glBegin(GL_QUADS);
		vertex(x,y,size,0,0,0);
		vertex(x,y,size,0,1,0);
		vertex(x,y,size,1,1,0);
		vertex(x,y,size,1,0,0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,smoke_tex[index]);
	glBlendFunc(GL_ONE,GL_ONE);
	glBegin(GL_QUADS);
		vertex(x,y,size,0,0,0);
		vertex(x,y,size,0,1,0);
		vertex(x,y,size,1,1,0);
		vertex(x,y,size,1,0,0);
	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

void load_powerup_data()
{
	int i;
	char fname[30];

	/* initialization of list */
	powerup_list=(Powerup**)malloc(sizeof(Powerup*));
	*powerup_list=NULL;
  
	pun=20+fn;
	powerup=(PowerupType*)malloc(sizeof(PowerupType)*pun);
  
  
	for(i=1;i<pun;i++)
	{
		sprintf(fname,"image/powerup/powerup%d.bmp",i);
		load_texture(&powerup[i].texture,fname);
	}
}

void new_powerup(GLfloat x,GLfloat y,char powerup_type)
{
	Powerup *pu;

	if(powerup_type==0 || powerup_type>=pun) return;
	
	pu=(Powerup*)malloc(sizeof(Powerup));
	pu->x=x;
	pu->y=y;
	pu->dirx=(GLfloat)rand()/RAND_MAX/100;
	pu->diry=-(GLfloat)rand()/RAND_MAX/100;
	pu->angle=0;
	pu->vanish_time=level_time.time+15;
	pu->type=powerup_type;
	pu->next=*powerup_list;
	*powerup_list=pu;
}

void handle_powerup()
{
	Powerup **temp,*del;
	for(temp=powerup_list;*temp;)
	{
		/* Delete Powerup */
		if((*temp)->vanish_time==level_time.time)
		{
			del=*temp;
			*temp=(*temp)->next;
			free(del);
			continue;
			}
    
		/* Update Powerup*/
		(*temp)->angle++;
		(*temp)->x+=(*temp)->dirx;
		(*temp)->dirx=((*temp)->x>=1.0)?-(GLfloat)rand()/RAND_MAX/100:((*temp)->x<=-1.0)?(GLfloat)rand()/RAND_MAX/100:(*temp)->dirx;
		(*temp)->y+=(*temp)->diry;
		(*temp)->diry=((*temp)->y>=1.0)?-(GLfloat)rand()/RAND_MAX/100:((*temp)->y<=-1.0)?(GLfloat)rand()/RAND_MAX/100:(*temp)->diry;
		draw_powerup(**temp);
    
		/* Taken Powerup */
		if((*temp)->x>=player_plane.x-plane[0].size/4 && (*temp)->x<=player_plane.x+plane[0].size/4 && (*temp)->y>=player_plane.y-plane[0].size/4 && (*temp)->y<=player_plane.y+plane[0].size/4)
		{
			take_powerup((*temp)->type);
			del=*temp;
			*temp=(*temp)->next;
			free(del);
			continue;
		}      
		temp=&(*temp)->next;
	}
}

void lost_powerup()
{
	/* Fire type powerup */
	player_plane.fire_type=0;
	new_powerup(player_plane.x+.1,player_plane.y+.1,player_plane.powerup_type);
	player_plane.powerup_type=0;
  
	/* Gun activation powerup */
	if(player_plane.gun_af==1)
	{
		player_plane.gun_af=0;
		new_powerup(player_plane.x-.1,player_plane.y+.2,15);
	}
	if(player_plane.gun_r==1)
	{
		player_plane.gun_r=0;
		new_powerup(player_plane.x+.21,player_plane.y-.1,16);
	}
	if(player_plane.gun_s==1)
	{
		player_plane.gun_s=0;
		new_powerup(player_plane.x-.1,player_plane.y-.1,17);
	}
	if(player_plane.gun_sf==1)
	{
		player_plane.gun_sf=0;
		new_powerup(player_plane.x-.2,player_plane.y-.2,18);
	}
	if(player_plane.gun_sr==1)
	{
		player_plane.gun_sr=0;
		new_powerup(player_plane.x-.2,player_plane.y+.2,19);
	}
}

void draw_powerup(Powerup pu)
{
	GLfloat x=pu.x,y=pu.y;
	GLfloat size=.1;
	glLoadIdentity();
	glTranslatef(x,y,0);
	glRotatef(pu.angle,1,1,1);
	glTranslatef(-x,-y,0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,powerup[pu.type].texture);
	glBegin(GL_TRIANGLES);
		glTexCoord2f(0.509,0.090); glVertex3d(x+size*(0.509-.5),y+size*(.5-0.090),-0.500*size);
		glTexCoord2f(0.980,0.899); glVertex3d(x+size*(0.980-.5),y+size*(.5-0.899),-0.500*size);
		glTexCoord2f(0.031,0.899); glVertex3d(x+size*(0.031-.5),y+size*(.5-0.899),-0.500*size);
    
		glTexCoord2f(0.509,0.090); glVertex3d(x+size*(0.507-.5),y+size*(.5-0.629), 0.449*size);
		glTexCoord2f(0.980,0.899); glVertex3d(x+size*(0.980-.5),y+size*(.5-0.899),-0.500*size);
		glTexCoord2f(0.031,0.899); glVertex3d(x+size*(0.031-.5),y+size*(.5-0.899),-0.500*size);
    
		glTexCoord2f(0.509,0.090); glVertex3d(x+size*(0.509-.5),y+size*(.5-0.090),-0.500*size);
		glTexCoord2f(0.980,0.899); glVertex3d(x+size*(0.507-.5),y+size*(.5-0.629), 0.449*size);
		glTexCoord2f(0.031,0.899); glVertex3d(x+size*(0.031-.5),y+size*(.5-0.899),-0.500*size);
    
		glTexCoord2f(0.509,0.090); glVertex3d(x+size*(0.509-.5),y+size*(.5-0.090),-0.500*size);
		glTexCoord2f(0.980,0.899); glVertex3d(x+size*(0.980-.5),y+size*(.5-0.899),-0.500*size);
		glTexCoord2f(0.031,0.899); glVertex3d(x+size*(0.507-.5),y+size*(.5-0.629), 0.449*size);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void take_powerup(int type)
{
	switch(type)
	{
		case 1:									// health +20
			player_plane.health=(player_plane.health>80)?100:player_plane.health+20;
			score+=20;
			break;
		case 2:									// health +50
			player_plane.health=(player_plane.health>50)?100:player_plane.health+50;
			score+=20;
			break;  
		case 3:									// health +100
			player_plane.health=100;
			score+=50;
			break;
		case 4:									// Armor +100
			armor=100;
			score+=20;
			break;      
		case 5:									// Armor +50
			armor=(armor>=50)?100:armor+50;
			score+=50;
			break;      
		case 6:									// Shield 10 sec
			shield+=10;
			score+=20;
			break;      
		case 7:									// Bomb +1
			bombs++;
			score+=20;
			break;      
		case 8:									// Instant Bomb
			score+=20;
			break;      
		case 9:									// Plane +1
			planes++;
			score+=20;
			break;      
		case 10:									// rapid fire
			plane[0].max_firing_delay=0;
			score+=50;
			break;      
		case 11:									// score +100
			score+=100;
			break;      
		case 12:									// score +500
			score+=500;
			break;      
		case 13:									// score +1000
			score+=1000;
			break;      
		case 14:									// All Guns
			player_plane.gun_af=player_plane.gun_r=player_plane.gun_s=player_plane.gun_sf=player_plane.gun_sr=1;
			score+=20;
			break;      
		case 15:									// Additional front gun
			player_plane.gun_af=1;
			score+=20;
			break;      
		case 16:									// Rear Gun
			player_plane.gun_r=1;
			score+=20;
			break;      
		case 17:									// Side Gun
			player_plane.gun_s=1;
			score+=20;
			break;      
		case 18:									// Side front gun
			player_plane.gun_sf=1;
			score+=20;
			break;      
		case 19:									// Side front gun
			player_plane.gun_sr=1;
			score+=20;
			break; 
		default:
		if(type>=20)
		{
			player_plane.fire_type=type-20;
			player_plane.powerup_type=type;
		}
		score+=20;
		break;
	}
	PlaySound("audio/powerup.wav",SLOT_POWERUP,1);
}

void handle_shield()
{
	static shield_time;
	if(shield==0) shield_time=level_time.time;
	if(shield>0)
	{
		if(level_time.time-shield_time>=1)
		{
			shield--;
			shield_time++;
		}
		glLoadIdentity();
		glTranslatef(player_plane.x,player_plane.y,0);
		glRotatef((loop_number%10)*36,0,0,1);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glBegin(GL_QUADS);
			glColor4f(0.1,0.0,0.5,0.3);		glVertex3d( 0.0, 0.3,-0.9);
			glColor4f(0.1,0.0,0.5,0.3);		glVertex3d( 0.3, 0.0,-0.9);
			glColor4f(0.1,0.0,0.5,0.3);		glVertex3d( 0.0,-0.3,-0.9);
			glColor4f(0.1,0.0,0.5,0.3);		glVertex3d(-0.3, 0.0,-0.9);
		glEnd();
		glDisable(GL_BLEND);
	}
}


void load_font()
{
	int i;
	char fname[30];
	for(i=32;i<=90;i++)
	{
		sprintf(fname,"font/%d.bmp",i);
		load_texture(&font_tex[i],fname);
		sprintf(fname,"font/%d_mask.bmp",i);
		load_texture(&font_texmask[i],fname);
	}
}

void GL_printf(GLfloat x,GLfloat y,GLfloat size,const char *fmt,...)
{
	int i;
	char string[100];
	va_list ap;
	va_start(ap,fmt);
	vsprintf(string,fmt,ap);
	va_end(ap);
	glColor4f(1,1,1,1);
	for(i=0;i<strlen(string);i++)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR,GL_ZERO);
		glBindTexture(GL_TEXTURE_2D,font_texmask[string[i]]);
		glBegin(GL_QUADS);
			vertex(x,y,size,0,0,0);
			vertex(x,y,size,0,1,0);
			vertex(x,y,size,1,1,0);
			vertex(x,y,size,1,0,0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D,font_tex[string[i]]);
		glBlendFunc(GL_ONE,GL_ONE);
		glBegin(GL_QUADS);
			vertex(x,y,size,0,0,0);
			vertex(x,y,size,0,1,0);
			vertex(x,y,size,1,1,0);
			vertex(x,y,size,1,0,0);
		glEnd();
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		x+=size/2;
	}
}

void load_boss_data(int level)
{
	FILE *fp;
	char garbage,fname[50],fpath[100];
	int i,j;
	sprintf(fname,"data/boss%d",level);
	if((fp=fopen(fname,"r"))==NULL)
	{
		char s[120];
		sprintf(s,"File \"%s\" missing",fname);
		MessageBox(NULL,"Error",s,MB_ICONWARNING);
		exit(0);
	}
	boss.x=0;
	boss.y=1.5;
	boss.angle=180;
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%f",&boss.size);
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%f",&boss.speed);
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%d",&boss.ngun);
	while((garbage=getc(fp))!=')');
	boss.health=boss.maxhealth=0;
	for(i=0;i<boss.ngun;i++)
	{
		fscanf(fp,"%f%f%f%f%d%d",&boss.gun[i].x,&boss.gun[i].y,&boss.gun[i].size,&boss.gun[i].angle,&boss.gun[i].fire_type,&boss.gun[i].maxhealth);
		boss.gun[i].health=boss.gun[i].maxhealth;
		boss.health+=boss.gun[i].health;
		boss.maxhealth+=boss.gun[i].maxhealth;
	}  
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%d",&boss.nstate);
	while((garbage=getc(fp))!=')');
	for(i=0;i<boss.nstate;i++)
	{
		fscanf(fp,"%f%f%f%d",&boss.state[i].x,&boss.state[i].y,&boss.state[i].angle,&boss.state[i].duration);
		for(j=0;j<boss.ngun;j++)
			fscanf(fp,"%d",&boss.state[i].gun_flag[j]);
	}
	boss.current_state=0;
	boss.state_end_time=0;
	while((garbage=getc(fp))!=')');
	fscanf(fp,"%s",&fname);
	sprintf(fpath,"image/plane/%s",fname);
	load_texture(&boss.texture,fpath);
	fscanf(fp,"%s",&fname);
	sprintf(fpath,"image/plane/%s",fname);
	load_texture(&boss.mask,fpath);
	fclose(fp);
}

void handle_boss()
{	
	static flag=0;
	int i;
	GLfloat x,y;
	Fire *f;

	if(boss.state_end_time==0) boss.state_end_time=level_time.time+boss.state[boss.current_state].duration;
	if(level_time.time==boss.state_end_time)
	{
		boss.current_state=(boss.current_state+1)%boss.nstate;
		boss.state_end_time=level_time.time+boss.state[boss.current_state].duration;
	}
	boss.x-=(boss.x>=boss.state[boss.current_state].x)?boss.speed:0.0;
	boss.x+=(boss.x<boss.state[boss.current_state].x)?boss.speed:0.0;
	boss.y-=(boss.y>=boss.state[boss.current_state].y)?boss.speed:0.0;
	boss.y+=(boss.y<boss.state[boss.current_state].y)?boss.speed:0.0;
	boss.angle=(99*boss.angle+boss.state[boss.current_state].angle)/100;
  
	draw_boss();
  
	for(boss.health=i=0;i<boss.ngun;i++)
	{
		x=boss.x-boss.gun[i].x*cos(-boss.angle*3.141592654/180)-boss.gun[i].y*sin(-boss.angle*3.141592654/180);
		y=boss.y+boss.gun[i].x*sin(-boss.angle*3.141592654/180)-boss.gun[i].y*cos(-boss.angle*3.141592654/180);
		/* Boss Fires */
		if(boss.gun[i].firing_delay>fire[boss.gun[i].fire_type].firing_delay)
		{
			if(boss.state[boss.current_state].gun_flag[i]==1 && boss.gun[i].health>0)
			{
				boss.gun[i].firing_delay=(boss.gun[i].health-boss.gun[i].maxhealth)/20;
				create_fire(x,y,boss.gun[i].fire_type,boss.angle+boss.gun[i].angle,&boss);
				switch(fire[boss.gun[i].fire_type].sound)
				{
					case BULLET:
						PlaySound("audio/bullet.wav",SLOT_FIRE,0);
						break;
					case LASER:
						PlaySound("audio/laser.wav",SLOT_FIRE,0);
						break;
					case PLASMA:
						PlaySound("audio/plasma.wav",SLOT_FIRE,0);
						break;
					case ROCKET:
						PlaySound("audio/rocket.wav",SLOT_FIRE,0);
						break;
				}
			}
		}
		else
			boss.gun[i].firing_delay++;
    
		/* Boss' gun hit by player fire */		
		for(f=*fire_list;f;f=f->next)
		{      
			if(f->owner == &player_plane && boss.gun[i].health>0
					&& f->x >= x-boss.gun[i].size/4 
					&& f->x <= x+boss.gun[i].size/4
					&& f->y >= y-boss.gun[i].size/4 
					&& f->y <= y+boss.gun[i].size/4)
			{
				if(fire[f->type].sound==BULLET)
					PlaySound("audio/hit.wav",SLOT_FIRE,0);
				boss.gun[i].health=(boss.gun[i].health<fire[f->type].power)?0:boss.gun[i].health-fire[f->type].power;
				f->health=0;
				if(boss.gun[i].health<=0)
				crash(x,y,boss.gun[i].size);
			}
		}    
		boss.health+=boss.gun[i].health;
		if(boss.gun[i].health<=boss.gun[i].maxhealth/2)
		draw_smoke(x,y+boss.gun[i].size/4);
	}
    
	/* Boss hit by player plane */
	if(shield==0 
			&&boss.x >= player_plane.x-boss.size/4-plane[0].size/4 
			&& boss.x <= player_plane.x+boss.size/4+plane[0].size/4
			&& boss.y >= player_plane.y-boss.size/4-plane[0].size/4 
			&& boss.y <= player_plane.y+boss.size/4+plane[0].size/4)
	{
		player_plane.health=0;
		lost_powerup();
		armor=0;
	}

	if(flag==0 && level_time.time==level_data.total_time+1)
	{
		flag=1;
		PlaySound("audio/boss.wav",SLOT_MUSIC1,1);
		PlaySound("audio/bossentry.wav",SLOT_MUSIC2,1);
	}
  
	/* Level complete */
	if(boss.health==0)
	{
		flag=0;
		level_complete=1;
		shield+=10000;
		score+=boss.maxhealth;
		crash(boss.x,boss.y,1);
		crash(boss.x,boss.y,boss.size);
		crash(boss.x+boss.size/4,boss.y+boss.size/4,boss.size);
		crash(boss.x-boss.size/4,boss.y+boss.size/4,boss.size);
		crash(boss.x-boss.size/4,boss.y-boss.size/4,boss.size);
		crash(boss.x+boss.size/4,boss.y-boss.size/4,boss.size);
		level_complete_time=level_time.time;		
	}
}

void draw_boss()
{
	glLoadIdentity();
	glColor4f(1,1,1,1);
	glTranslatef(boss.x,boss.y,0.0);
	glRotatef(boss.angle, 0.0, 0.0, 1.0);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR,GL_ZERO);
	glBindTexture(GL_TEXTURE_2D,boss.mask);
	glBegin(GL_QUADS);
		vertex(0,0,boss.size,0,0,0);
		vertex(0,0,boss.size,0,1,0);
		vertex(0,0,boss.size,1,1,0);
		vertex(0,0,boss.size,1,0,0);
	glEnd();
	glBindTexture(GL_TEXTURE_2D,boss.texture);
	glBlendFunc(GL_ONE,GL_ONE);
	glBegin(GL_QUADS);
		vertex(0,0,boss.size,0,0,0);
		vertex(0,0,boss.size,0,1,0);
		vertex(0,0,boss.size,1,1,0);
		vertex(0,0,boss.size,1,0,0);
	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

void newgame()
{
	score=0;
	planes=3;
	armor=0;
	shield=0;
	bombs=3;
	bombing_delay=0;  
	level=0;
	new_level();
	gameover=0;
	paused=0;
	reset_player();
	plane[0].max_firing_delay=10;
}

void new_level()
{
	Plane **ptemp,*pdel;
	Fire **ftemp,*fdel;
	level++;
	load_level_data(level);
	if(game_complete)
		return;
	reset_timer(&level_time);
	level_complete=0;
	level_complete_time=0;
	loop_number=0;
	level_entry_index=0;
	bgnum=0;
	bgpart=0.0;
	shield=3;
	/* Deleting Planes */
	for(ptemp=plane_list;*ptemp;)
	{
		pdel=*ptemp;
		*ptemp=(*ptemp)->next;
		free(pdel);
	}
	/* Deleting Fires */
	for(ftemp=fire_list;*ftemp;)
	{
		fdel=*ftemp;
		*ftemp=(*ftemp)->next;
		free(fdel);
	}
	PlaySound(level_data.music,SLOT_MUSIC1,1);
}

void vertex(GLfloat X,GLfloat Y,GLfloat size,GLfloat x,GLfloat y,GLfloat z)
{
	glTexCoord2f(x,y);
	glVertex3d(X+size*(x-.5),Y+size*(.5-y),z*size);
}

void mixaudio(void *unused, Uint8 *stream, int len)
{
    int i;
    Uint32 amount;

    for ( i=0; i<SLOTS; ++i )
	{
        amount = (sounds[i].dlen-sounds[i].dpos);
        if ( amount > len )
		{
            amount = len;
        }
        SDL_MixAudio(stream, &sounds[i].data[sounds[i].dpos], amount, SDL_MIX_MAXVOLUME);
        sounds[i].dpos += amount;
    }
}

void InitAudio()
{
	SDL_AudioSpec fmt;
	if(SDL_Init(SDL_INIT_AUDIO)==-1)
	{
		MessageBox(NULL,SDL_GetError(),"Error",MB_ICONWARNING);
		exit(1);
	}

	fmt.freq = 22050;
	fmt.format = AUDIO_S16;
	fmt.channels = 2;
	fmt.samples = 512; 
	fmt.callback = mixaudio;
	fmt.userdata = NULL;

	if ( SDL_OpenAudio(&fmt,NULL) < 0 )
	{
		MessageBox(NULL,SDL_GetError(),"Error",MB_ICONWARNING);    
		exit(1);
	}
	SDL_PauseAudio(0);
}

void PlaySound(char *file,int start_slot,int end_slot,int force)
{
	int index;
	SDL_AudioSpec wave;
	Uint8 *data;
	Uint32 dlen;
	SDL_AudioCVT cvt;
 
	
	for ( index=start_slot; index<=end_slot; ++index )
		if ( sounds[index].dpos == sounds[index].dlen )
			break;
	if (!force && index>end_slot)
		return;
	else if(force)
	{
		SDL_LockAudio();
		index=start_slot;
	}

	if ( SDL_LoadWAV(file, &wave, &data, &dlen) == NULL )
	{
		MessageBox(NULL,SDL_GetError(),"Error",MB_ICONWARNING);    
		return;
	}
	SDL_BuildAudioCVT(&cvt, wave.format, wave.channels,wave.freq, AUDIO_S16, 2, 22050);
	cvt.buf = (Uint8*)malloc(dlen*cvt.len_mult);
	memcpy(cvt.buf, data, dlen);
	cvt.len = dlen;
	SDL_ConvertAudio(&cvt);
	SDL_FreeWAV(data);

	if ( sounds[index].data )
	{
		free(sounds[index].data);
	}
	if(!force)
		SDL_LockAudio();
	sounds[index].data = cvt.buf;
	sounds[index].dlen = cvt.len_cvt;
	sounds[index].dpos = 0;
	SDL_UnlockAudio();
}

void LoopSound(int start_slot,int end_slot)
{
	int index;
	SDL_LockAudio();
	for ( index=start_slot; index<=end_slot; ++index )
		if ( sounds[index].dpos == sounds[index].dlen )
			sounds[index].dpos=0;
	SDL_UnlockAudio();
}

void CloseAudio()
{ 
    SDL_CloseAudio();
	SDL_Quit();
}