#include"acllib.h"
#include <stdio.h>
#define W_WIDTH  300
#define W_HEIGHT 399
#define ARMY_WIDTH 40
#define ARMY_HEIGHT 47
#define ENEMY_WIDTH 30
#define ENEMY_HEIGHT 22
#define BLT_WIDTH 4
#define BLT_HEIGHT 10
#define TXTSIZE 20
#define TXTFONT "幼圆"
#define Frame   60.0f //游戏帧数
#define MAXENEMY 200		//场上最多敌机个数 
#define MAXBLT 50			//场上最多子弹个数 
#define TIMEINTERVAL 10 //20ms  50fps

static double TimePerFrame = 1000.0f / Frame;//每帧固定的时间差,此处限制fps为60帧每秒
  //记录上一帧的时间点
static ULONGLONG  lastTime = 0;
enum Direction {//方向枚举类型
	INIT=0,
	UP = 2,
	DOWN = 4,
	RIGHT=3,
	LEFT=1
};
//二维坐标
typedef struct {
	int x; //横坐标
	int y;//纵坐标
}Vec2;
//背景
typedef struct {
	ACL_Image image;//图片
	int x, y1, y2;//背景图片长度，宽度
}Background;
//飞机
typedef struct {
	ACL_Image image;//图片
	Vec2 size;//大小
	Vec2 pos;//位置
	Vec2 speed;//速度
	enum Direction dir;//方向
	int isAlive;//是否活着
}Plane;
//游戏全局数据
typedef struct {
	int enemyspeed ;//敌方飞机速度
	int gaptimems ;//敌方飞机刷新间隔
	int yg;      //游戏结束logo 纵坐标
	int cnt ;		//右侧子弹序号 
	int cnt2 ;  //左侧子弹序号
	int enemycnt ;	//敌机序号 
	int heat ;		//炮管热量 
	int gaptime ;	//敌机出现间隔时间
	int score ;  //得分
	char txt[5] ; //得分标签
	int isFire;		//是否开火 
	int isPlaySound;//是否播放声音
}GameGlobalData;
//子弹
typedef struct {
	ACL_Image image;//图片
	Vec2 size;//大小
	Vec2 pos;//位置
	Vec2 speed;//速度
	int isAlive;//是否活着
}Bullet;
//构造新二维坐标
Vec2 new_vec2(int x, int y)
{
	Vec2 v = { x,y };
	return v;
}
//游戏场景类型
typedef struct {

	Plane player;//我方玩家
	Plane enemy[MAXENEMY];//敌方飞机
	Bullet blt[MAXBLT];//右侧子弹
	Bullet blt2[MAXBLT];//左侧子弹
	Background bg;// 游戏背景
	ACL_Image gameover;// 游戏结束logo
	ACL_Sound firesound;//发射子弹声音
	ACL_Sound bgm;//游戏背景音乐1
	ACL_Sound bgm2;//游戏背景音乐2
}Scene;
//游戏对象
typedef struct {
	Scene  mainScene;//游戏场景
	GameGlobalData data;//游戏数据
}GameObj;

void getText(int score,char * txt)
{
	int s = score;
	int n = 0;
	while (s > 0) {
		s = s / 10;
		n++;
	}
	s = score;
	while (n > 0) {
		txt[n - 1] = s % 10 + 48;
		s = s / 10;
		n--;
	}
}
void initArmy(Plane* plane)
{
	loadImage("data/army.bmp", &(plane->image));
	plane->isAlive = 1;
	plane->size = new_vec2(ARMY_WIDTH, ARMY_HEIGHT);
	plane->pos = new_vec2(130, 320);
	plane->speed = new_vec2(plane->pos.x + plane->size.x - 1, plane->pos.y + plane->size.y - 1);
	plane->dir = INIT;
	
}
void initEnemy(Plane* enemy)
{
	loadImage("data/enemy.bmp", &(enemy->image));
	enemy->isAlive = 1;
	enemy->size = new_vec2(ENEMY_WIDTH, ENEMY_HEIGHT);
	enemy->pos = new_vec2(rand() % 271, -enemy->size.y);
	enemy->speed = new_vec2(enemy->pos.x + enemy->size.x - 1, enemy->pos.y + enemy->size.y - 1);
	enemy->dir = INIT;

}
Bullet initBLT(Bullet blt, Plane* f, int xw, int yw) {
	loadImage("data/bullet.bmp", &blt.image);
	blt.size = new_vec2(BLT_WIDTH, BLT_HEIGHT);
	blt.isAlive = 0;
	//cordinary
	blt.pos = new_vec2(f->pos.x + xw, f->pos.y + yw);
	blt.speed = new_vec2(blt.pos.x + blt.size.x - 1, blt.pos.y + blt.size.y - 1);
	return blt;
}
Background initBG(Background bg) {
	loadImage("data/bg.bmp", &bg.image);
	bg.x = 0;
	bg.y1 = 0;
	bg.y2 = -W_HEIGHT + 1;
	return bg;
}
void initScene(Scene* scene)
{
	loadImage("data/gameover.bmp", &scene->gameover);
	loadSound("data/fire.wav", &scene->firesound);
	loadSound("data/bgm1.mp3", &scene->bgm);
	loadSound("data/gameover.mp3", &scene->bgm2);
	scene->bg=initBG(scene->bg);
	initArmy(&(scene->player));
	for (int i = 0; i < MAXENEMY; i++) {
		initEnemy(&(scene->enemy[i]));
	}
	for (int i = 0; i < MAXBLT; i++) {
		scene->blt[i] = initBLT(scene->blt[i], &scene->player, 5, 8);
		scene->blt2[i] = initBLT(scene->blt2[i], &scene->player, 32, 8);
	}
	
}
void initGame(GameObj* game)
{
	initScene(&(game->mainScene));
	GameGlobalData data = {
		.cnt = 0,
		.cnt2 = 0,
		.enemycnt = 0,
		.enemyspeed = 1,
		.gaptime = 0,
		.gaptimems = 20,
		.heat = 0,
		.isFire = 0,
		.isPlaySound = 0,
		.txt = {48,},
		.yg = -50,
		.score = 0,

	};
	game->data = data;


}
//paint function
void paintFlighter(Plane* f) {
	if (f->isAlive == 1) {
		putImageTransparent(&(f->image), f->pos.x, f->pos.y, f->size.x, f->size.y, WHITE);
	}
}
void paintBLT(Bullet* blt) {
	if (blt->isAlive == 1) {
		putImageTransparent(&(blt->image),blt->pos.x, blt->pos.y, blt->size.x, blt->size.y, WHITE);
	}
}
void paintBG(Background* bg) {
	putImage(&(bg->image), bg->x, bg->y1);
	putImage(&(bg->image), bg->x, bg->y2);
}
void keyAction(Plane* player,int * isFire,int key, int event) {

	static int isMove = 0;
	if (isMove == 0 && (key >= 37 && key <= 40) && event == 0) {
		isMove = 1;
		switch (key) {
		case 37:	player->dir= LEFT;	break;
		case 38:	player->dir = UP;	break;
		case 39:	player->dir = RIGHT;	break;
		case 40:	player->dir = DOWN;	break;
		}
	}
	else
		if (isMove == 1 && (key >= 37 && key <= 40) && event == 1) {
			isMove = 0;
			player->dir = 0;
	
		}
	if (*isFire == 0 && key == 32 && event == 0) {
		*isFire = 1;
	}
	else
		if (*isFire == 1 && key == 32 && event == 1) {
			*isFire = 0;
		}
}
//motion
void moveEnemy(Plane* f,int * enemyspeed) {
	if (f->isAlive == 1) {
		switch (f->dir) {
		case LEFT:	(f->pos.x==0) ? (f->pos.x-=*enemyspeed, f->speed.x -= *enemyspeed) : 1;
			break;
		case RIGHT:	(f->speed.x <= W_WIDTH) ? (f->pos.x += *enemyspeed, f->speed.x += *enemyspeed) : 1;
			break;
		case UP:	(f->pos.y >= 0) ? (f->pos.y -= *enemyspeed, f->speed.y -= *enemyspeed) : 1;
			break;
		case DOWN:	(1) ? (f->pos.y += *enemyspeed, f->speed.y += *enemyspeed) : 1;

	
			if (f->pos.y > W_HEIGHT) {
				f->isAlive = 0;
			}
			break;
		default:
			;
		}
	}
}
void moveArmy(Plane* f) {
	switch (f->dir) {
	case LEFT:	(f->pos.x>= 0) ? (f->pos.x -= 3, f->speed.x -= 3) : 1;
		break;
	case RIGHT:	(f->speed.x <= W_WIDTH) ? (f->pos.x += 3, f->speed.x += 3) : 1;
		break;
	case UP:	(f->pos.y >= 0) ? (f->pos.y -= 3, f->speed.y -= 3) : 1;
		break;
	case DOWN:	(f->speed.y <= W_HEIGHT) ? (f->pos.y += 3, f->speed.y += 3) : 1;
		break;
	default:
		;
	}
}
void moveBLT(Bullet* blt) {
	if (blt->isAlive == 1) {
		blt->pos.y -= 6;
	}
	if (blt->pos.y< 0) {
		blt->isAlive = 0;
	}
}
void moveBG(Background* bg) {
	bg->y1++;
	bg->y2++;
	if (bg->y1 == W_HEIGHT) {
		bg->y1 = -W_HEIGHT + 1;
	}
	if (bg->y2 == W_HEIGHT) {
		bg->y2 = -W_HEIGHT + 1;
	}
}
void movePosition(Plane *player,Plane*enemy,Bullet* blt,Bullet*blt2,Background* bg,int* yg,int* enemyspeed) {
	int i = 0;
	if (player->isAlive == 1) {
		moveArmy(player);
	}
	if (player->isAlive == 0) {
		if (*yg <= 180) *yg += 1;
	}
	for (i = 0; i < MAXENEMY; i++) {
		moveEnemy(enemy++,enemyspeed);
	
	}
	for (i = 0; i < MAXBLT; i++) {
		moveBLT(blt++);
		moveBLT(blt2++);
	}
	moveBG(bg);
}
//view
void paintGameOver(ACL_Image * gameover,int * yg) {
	putImageTransparent(gameover, 50, *yg, 200, 40, WHITE);
}

void PaintTitle(int* score,int * enemyspeed,char*txt)
{
	getText(*score,txt);
	paintText(2, 2, "Score:");
	paintText(60, 2, txt);
	memset(txt, 0, strlen(txt));
	getText(*enemyspeed, txt);
	paintText(160, 2, "Enemy Speed:");
	paintText(280, 2, txt);

	
	
}
//control

void paint(Scene* scene,GameGlobalData* data) {
	int i = 0;
	movePosition(&scene->player,scene->enemy,scene->blt,scene->blt2,&scene->bg,&data->yg,&data->enemyspeed);
	beginPaint();
	clearDevice();
	setTextColor(BLACK);
	setTextSize(TXTSIZE);
	setTextFont(TXTFONT);
	paintBG(&scene->bg);
	PaintTitle(&data->score,&data->enemyspeed,data->txt);
	for (i = 0; i < MAXENEMY; i++) {
		paintFlighter(&scene->enemy[i]);
	}
	for (i = 0; i < MAXBLT; i++) {
		paintBLT(&scene->blt[i]);
		paintBLT(&scene->blt2[i]);
	}
	if (scene->player.isAlive == 1) {
		paintFlighter(&scene->player);
	}
	else if (scene->player.isAlive == 0) {
		paintGameOver(&scene->gameover,&data->yg);
	}
	endPaint();
}
void fire(Scene* scene,GameGlobalData* data) {
	if (data->isFire== 1) {
		if (data->heat == 0) {
			data->heat = 6;
			scene->blt[data->cnt] = initBLT(scene->blt[data->cnt], &scene->player, 5, 8);
			scene->blt2[data->cnt2] = initBLT(scene->blt2[data->cnt2], &scene->player, 32, 8);
			scene->blt[data->cnt].isAlive = 1;
			scene->blt2[data->cnt2].isAlive = 1;
			playSound(scene->firesound, 0);
		
			if (++data->cnt == MAXBLT) data->cnt = 0;
			if (++data->cnt2 == MAXBLT) data->cnt2 = 0;
		}
		else
			data->heat--;
	}
}

void enemyOut(Scene* scene, GameGlobalData* data) {
	if( data->gaptime <= 0) {
		initEnemy(&scene->enemy[data->enemycnt]);
		scene->enemy[data->enemycnt].dir = DOWN;
		data->gaptime = rand() % data->gaptimems * 4 + data->gaptimems;
		if (++data->enemycnt == MAXENEMY)  data->enemycnt = 0;
	}
	else
	{
		data->gaptime -= data->enemyspeed;
		if (data->score > 0 && data->score % 20 == 0) {
			data->enemyspeed = data->score / 20 + 1;
			if (data->gaptime >= 10) {
				data->gaptimems = 20 - data->enemyspeed;
			}
		}
	}
	
}
void isHitbyBLT(Bullet* blt, Plane* f, GameGlobalData* data) {
	if (blt->isAlive == 1 && f->isAlive == 1 && blt->pos.y <= f->speed.y && blt->speed.y >= f->pos.y) {
		if (!(blt->speed.x <f->pos.x || blt->pos.x > f->speed.x)) {
			f->isAlive = 0;
			blt->isAlive = 0;
			data->score++;
		}
	}
}
void isCrash(Plane* enemy, Plane* army) {
	if (enemy->isAlive == 1 && army->isAlive == 1 && enemy->speed.y >= army->pos.y && enemy->pos.y <= army->speed.y) {
		if (!(army->speed.x <enemy->pos.x || army->pos.x > enemy->speed.x)) {
			enemy->isAlive = army->isAlive = 0;
		}
	}
}

void crash(Scene* scene,GameGlobalData* data) {
	int i, j;
	for (i = 0; i < MAXBLT; i++) {
		for (j = 0; j < MAXENEMY; j++) {
			isHitbyBLT(&scene->blt[i], &scene->enemy[j],data);
			isHitbyBLT(&scene->blt2[i], &scene->enemy[j],data);
		}
	}
	for (j = 0; j < MAXENEMY; j++) {
		isCrash(&scene->enemy[j], &scene->player);
	}
}

//游戏渲染器，是整个游戏的核心
void renderer(int id,void* param) {
	
	GameObj* game = (GameObj*)param;
	ULONGLONG nowTime = GetTickCount64();     //获得当前帧的时间点

	ULONGLONG deltaTime = nowTime - lastTime;  //计算这一帧与上一帧的时间差
	lastTime = nowTime;                 //更新上一帧的时间点

	if (id == 0) {
		if (game->mainScene.player.isAlive == 1) {//判断玩家是否活着
			fire(&game->mainScene,&game->data);//发射子弹
			crash(&game->mainScene, &game->data);//检测物理碰撞事件
		}
		enemyOut(&game->mainScene, &game->data);//刷新敌方飞机
		paint(&game->mainScene,&game->data);//渲染
		//music
		if (game->data.isPlaySound == 0 && game->mainScene.player.isAlive == 1) {
			playSound(game->mainScene.bgm, 1);
			game->data.isPlaySound = 1;
		}
		else if (game->data.isPlaySound == 1 && game->mainScene.player.isAlive == 0) {
			game->data.isPlaySound = 2;
			playSound(game->mainScene.bgm2, 0);
		}
	}
	//若 实际时间差 少于 每帧固定时间差，则让机器休眠 少于的部分时间。
	if (deltaTime <= TimePerFrame)
		Sleep(TimePerFrame - deltaTime);
	
	

}

void getKey(int key, int event,void* param)//检测键盘事件
{
	GameObj* g = (GameObj*)param;
	keyAction(&g->mainScene.player,&g->data.isFire,key, event);
};

int Setup()
{
	//限制帧数的循环  <60fps

	static GameObj game;//全局唯一游戏对象
	lastTime = GetTickCount64();//获取游戏开始时的时间
	initWindow("AirCraft", 500, 200, W_WIDTH, W_HEIGHT);//创建Windows应用程序界面
	srand(time(NULL));//初始化随机数种子
	initGame(&game);//初始化游戏实体
	registerKeyboardEvent(getKey,&game);//注册游戏键盘事件
	registerTimerEvent(renderer, &game);  //注册游戏渲染定时器
	startTimer(0, TIMEINTERVAL);//游戏渲染定时器开始
	return 0;
}
