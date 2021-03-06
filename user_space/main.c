#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define DEV_NAME "/dev/keyboard_inc"
#define DEV_TIME_NAME "/dev/keyboard_time"
#define MSG_MAX 32
#define MSG_RESET "0"

int32_t convert_to_int(unsigned char* data) {
    int32_t x = 0;

    x  = (int32_t)data[0];
    x |= (int32_t)(data[1]) << 8;
    x |= (int32_t)(data[2]) << 16;
    x |= (int32_t)(data[3]) << 24;

    return x;
}

int32_t kernel_interface_keyboard_counter(unsigned char* msg, int32_t counter) {
    if (msg) {
        int fd;
        ssize_t num_read;
        /* Open kernel */
        fd = open(DEV_NAME, O_RDWR);
        if (fd == -1)
        {
            perror("Cannot open file description from kernel\n");
            return -1;
        }
        /* Read value data from kernel */
        num_read = read(fd, msg, MSG_MAX);
        if (num_read == -1)
        {
            perror("Cannot read information from kernel \n");
            return -1;
        }
        if (num_read) {
            counter = convert_to_int(msg);
            printf("returned: %d from the system call, num bytes read: %d\n", counter, (int)num_read);
        }
        close(fd);
        
        return counter;
    }
    
    return -1;
}

int kernel_interface_keyboard_timer(unsigned char* msg) {
    if (msg) {
        double factor = 4.29496730204;
        int32_t s = 0;
        
        int fd;
        ssize_t num_read;
        /* Open kernel */
        fd = open(DEV_TIME_NAME, O_RDWR);
        if (fd == -1)
        {
            perror("Cannot open file description from kernel\n");
            return -1;
        }
        /* Read value data from kernel */
        num_read = read(fd, msg, MSG_MAX);
        if (num_read == -1)
        {
            perror("Cannot read information from kernel \n");
            return -1;
        }
        if (num_read) {
            printf("Recv %d bytes\n", num_read);
            for (int i = 0; i < num_read; i++) {
                printf("%2.2X ", msg[i]);
            }
            printf("\n");
                        
            memcpy(&s, &(msg[4]), 4);
            // time_t t = (int)(s * factor);
        }
        close(fd);
        
        return (int)(s * factor);
    }
    
    return -1;
}

void kernel_interface_reset() {
    int fd;
    ssize_t num_written;
    
    /* Open kernel */
    fd = open(DEV_NAME, O_RDWR);
    if (fd == -1)
    {
        perror("Cannot open file description from kernel\n");
        return;
    }
    
    num_written = write(fd, MSG_RESET, sizeof(MSG_RESET));
    if (num_written == -1) {
        perror("Cannot send message to kernel module \n");
    }
    
    close(fd);
}

#define NK_XLIB_IMPLEMENTATION
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_xlib.h"


#define DTIME           20
#define WINDOW_WIDTH    400
#define WINDOW_HEIGHT   200
#define GUI_SIZE_ELEM   45

typedef struct XWindow XWindow;
struct XWindow {
    Display *dpy;
    Window root;
    Visual *vis;
    Colormap cmap;
    XWindowAttributes attr;
    XSetWindowAttributes swa;
    Window win;
    int screen;
    XFont *font;
    unsigned int width;
    unsigned int height;
    Atom wm_delete_window;
};

static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

static long
timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec/1000);
}

static void
sleep_for(long t)
{
    struct timespec req;
    const time_t sec = (int)(t/1000);
    const long ms = t - (sec * 1000);
    req.tv_sec = sec;
    req.tv_nsec = ms * 1000000L;
    while(-1 == nanosleep(&req, &req));
}

int
main(void)
{
    unsigned char msg[MSG_MAX];
    unsigned char buf[MSG_MAX];
    unsigned char date[2 * MSG_MAX];
    snprintf(date, MSG_MAX, "was no reset yet");
    
    time_t reset_time;
    
    int32_t counter = 0;

    for (int i = 0; i < MSG_MAX; i++) {
        msg[i] = 0;
        buf[i] = 0;
    }
    
    long dt;
    long started;
    int running = 1;
    XWindow xw;
    struct nk_context *ctx;

    /* X11 */
    memset(&xw, 0, sizeof xw);
    xw.dpy = XOpenDisplay(NULL);
    if (!xw.dpy) die("Could not open a display; perhaps $DISPLAY is not set?");
    xw.root = DefaultRootWindow(xw.dpy);
    xw.screen = XDefaultScreen(xw.dpy);
    xw.vis = XDefaultVisual(xw.dpy, xw.screen);
    xw.cmap = XCreateColormap(xw.dpy,xw.root,xw.vis,AllocNone);

    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask| ButtonMotionMask |
        Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask|
        PointerMotionMask | KeymapStateMask;
    xw.win = XCreateWindow(xw.dpy, xw.root, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
        XDefaultDepth(xw.dpy, xw.screen), InputOutput,
        xw.vis, CWEventMask | CWColormap, &xw.swa);

    XStoreName(xw.dpy, xw.win, "Michal Brach Keyboard Counter");
    XMapWindow(xw.dpy, xw.win);
    xw.wm_delete_window = XInternAtom(xw.dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(xw.dpy, xw.win, &xw.wm_delete_window, 1);
    XGetWindowAttributes(xw.dpy, xw.win, &xw.attr);
    xw.width = (unsigned int)xw.attr.width;
    xw.height = (unsigned int)xw.attr.height;

    /* GUI */
    xw.font = nk_xfont_create(xw.dpy, "fixed");
    ctx = nk_xlib_init(xw.font, xw.dpy, xw.screen, xw.win, xw.width, xw.height);
    
    while (running)
    {
        counter = kernel_interface_keyboard_counter(msg, counter);
        if (counter < 0) {
            exit(1);
        }
        /* Input */
        XEvent evt;
        started = timestamp();
        nk_input_begin(ctx);
        while (XPending(xw.dpy)) {
            XNextEvent(xw.dpy, &evt);
            if (evt.type == ClientMessage) goto cleanup;
            if (XFilterEvent(&evt, xw.win)) continue;
            nk_xlib_handle_event(xw.dpy, xw.screen, xw.win, &evt);
        }
        nk_input_end(ctx);

        if (nk_begin(ctx, "Keyboard Interrupt Counter", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT),
            NK_WINDOW_BORDER|NK_WINDOW_TITLE))
        {  
            nk_layout_row_dynamic(ctx, GUI_SIZE_ELEM, 2);
            
            snprintf(buf, MSG_MAX, "%d", counter);
            
            nk_label(ctx, "Counter pressed keys: ", NK_TEXT_LEFT);
            nk_label(ctx, buf, NK_TEXT_LEFT);
            
            nk_layout_row_dynamic(ctx, GUI_SIZE_ELEM, 1);
            
            if (nk_button_label(ctx, "RESET")) {
                kernel_interface_reset();
                reset_time = kernel_interface_keyboard_timer(msg);
                // reset_time = time(NULL);                

                snprintf(date, strlen(ctime(&reset_time)), "%s", ctime(&reset_time));
            }
            
            nk_layout_row_dynamic(ctx, GUI_SIZE_ELEM, 2);
            
            nk_label(ctx, "Counter reset time: ", NK_TEXT_LEFT);
            nk_label(ctx, date, NK_TEXT_LEFT);
        }
        nk_end(ctx);
          
        /* Draw */
        XClearWindow(xw.dpy, xw.win);
        nk_xlib_render(xw.win, nk_rgb(30,30,30));
        XFlush(xw.dpy);

        /* Timing */
        dt = timestamp() - started;
        if (dt < DTIME)
            sleep_for(DTIME - dt);
    }

cleanup:
    nk_xfont_del(xw.dpy, xw.font);
    nk_xlib_shutdown();
    XUnmapWindow(xw.dpy, xw.win);
    XFreeColormap(xw.dpy, xw.cmap);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}