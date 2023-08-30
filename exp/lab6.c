#include <stdio.h>
#include "../common/common.h"

#define COLOR_BACKGROUND	FB_COLOR(0xff,0xff,0xff)
#define COLOR			FB_COLOR(0x0,0x0,0x0)
#define RED	FB_COLOR(255,0,0)
#define BLUE	FB_COLOR(0,0,255)

static int touch_fd;
static int bluetooth_fd;
int state=0;
int msg[3];


void clear() {
	fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, FB_COLOR(255, 255, 255));

	fb_draw_rect(0, 0, 100, 50, COLOR_BACKGROUND);
	fb_draw_border(0, 0, 100, 50, COLOR);
	fb_draw_text(10, 30, "CLEAR", 25, COLOR);
	/* draw eraser */
	fb_draw_rect(100, 0, 100, 50, COLOR_BACKGROUND);
	fb_draw_border(100, 0, 100, 50, COLOR);
	fb_draw_text(110, 30, "eraser", 25, COLOR);
	/* draw start */

	fb_draw_rect(200, 0, 100, 50, COLOR_BACKGROUND);
	fb_draw_border(200, 0, 100, 50, COLOR);
	fb_draw_text(210, 30, "black", 25, COLOR);

	fb_draw_rect(300, 0, 100, 50, RED);
	fb_draw_border(300, 0, 100, 50, COLOR);
	fb_draw_text(310, 30, "red", 25, COLOR);

	fb_draw_rect(400, 0, 100, 50, BLUE);
	fb_draw_border(400, 0, 100, 50, COLOR);
	fb_draw_text(410, 30, "blue", 25, COLOR);

	fb_update();
}
void draw(int x,int y) {
	if (state == 0) {
		fb_draw_circle(x, y, 5, COLOR);
	}
	else if (state == 3) {
		fb_draw_circle(x, y, 5, RED);
	}
	else if (state == 4) {
		fb_draw_circle(x, y, 5, BLUE);
	}
	else {
		fb_draw_circle(x, y, 15, COLOR_BACKGROUND);
	}
	fb_update();
}

static void touch_event_cb(int fd)
{
	int type,x,y,finger;
	type = touch_read(fd, &x,&y,&finger);
	switch(type){
	case TOUCH_PRESS:
		if(x<=200 && y<=50 && x>100){
		    state=1;
        }
		else if(x<=100 && y<=50){
			state=2;
		}
		else if(x>200 && x<=300 && y<=50){
		    state=0;
        }
		else if(x>300 && x<=400 && y<=50){
		    state=3;
        }
		else if(x>400 && x<=500 && y<=50){
		    state=4;
        }
		else{
			draw();
		}
		msg[0]=x;msg[1]=y;msg[2]=state;
		myWrite_nonblock(bluetooth_fd, msg, 16);
		if (state == 2) {
			clear();
			state = 0;
		}
		break;

        case TOUCH_MOVE:
		if(!(x <= 500 && y <= 60)){
			draw();
			msg[0]=x;msg[1]=y;msg[2]=state;
			myWrite_nonblock(bluetooth_fd, msg, 16);
		}
		break;

	case TOUCH_ERROR:
		printf("close touch fd\n");
		task_delete_file(fd);
		close(fd);
		break;
	default:
		return;
	}
	return;
}

static void bluetooth_tty_event_cb(int fd)
{
	char buf[128];
	int n, *p=buf;
	n = myRead_nonblock(fd, buf, 127);
	if(n <= 0) return;
	int x=p[0], y=p[1], state=p[2];
	
    if(state==2){
		clear();
		state = 0;
    }
	else if(!(x<=500 && y<=60)){
		draw();
	}
	return;

}

static int bluetooth_tty_init(const char *dev)
{
	int fd = open(dev, O_RDWR|O_NOCTTY|O_NONBLOCK); /*非阻塞模式*/
	if(fd < 0){
		printf("bluetooth_tty_init open %s error(%d): %s\n", dev, errno, strerror(errno));
		return -1;
	}
	return fd;
}

int main(int argc, char *argv[])
{
	fb_init("/dev/fb0");
	font_init("./font.ttc");

	clear();

	touch_fd = touch_init("/dev/input/event2");
	task_add_file(touch_fd, touch_event_cb);

	bluetooth_fd = bluetooth_tty_init("/dev/rfcomm0");
	if(bluetooth_fd == -1) return 0;
	task_add_file(bluetooth_fd, bluetooth_tty_event_cb);

	task_loop();
	return 0;
}
