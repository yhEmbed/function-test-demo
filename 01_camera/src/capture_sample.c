#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
 /*
 imx335
 root@atk-rk3568-evb:~# v4l2-ctl -d /dev/video0 --list-formats-ext
ioctl: VIDIOC_ENUM_FMT
        Type: Video Capture Multiplanar

        [0]: 'UYVY' (UYVY 4:2:2)
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [1]: '422P' (Planar YUV 4:2:2)
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [2]: 'NV16' (Y/CbCr 4:2:2)
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [3]: 'NV61' (Y/CrCb 4:2:2)
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [4]: 'YM16' (Planar YUV 4:2:2 (N-C))
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [5]: 'NV21' (Y/CrCb 4:2:0)
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [6]: 'NV12' (Y/CbCr 4:2:0)
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [7]: 'NM21' (Y/CrCb 4:2:0 (N-C))
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [8]: 'NM12' (Y/CbCr 4:2:0 (N-C))
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [9]: 'YU12' (Planar YUV 4:2:0)
                Size: Stepwise 32x16 - 2592x1944 with step 8/8
        [10]: 'YM24' (Planar YUV 4:4:4 (N-C))
                Size: Stepwise 32x16 - 2592x1944 with step 8/8

 */

#define FMT_NUM_PLANES 1
#define CAPTURE_TYPE V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
typedef struct camera_format {
 unsigned char description[32]; //字符串描述信息
 unsigned int pixelformat; //像素格式
} cam_fmt;
 
static cam_fmt cam_fmts[2]; 
 
int lcdfd = 0; //LCD设备文件描述符
int *lcdptr = NULL; //LCD映射到内存的指针
int lcd_w=1080, lcd_h=1920 ; //LCD屏幕的分辨率
int video_width = 1080, video_height= 1920; //摄像头采集数据的分辨率
 
static int fb_dev_init(void) 
{
	//打开LCD设备文件
	lcdfd = open("/dev/fb0", O_RDWR);
	if (lcdfd < 0) 
	{
		perror("LCD open failed:");
	}
	/*获取LCD信息*/
	struct fb_var_screeninfo info;
	int lret = ioctl(lcdfd, FBIOGET_VSCREENINFO, &info);
	if (lret < 0) 
	{
		perror("get info failed:");
	}
 
	//获取LCD的分辨率
	lcd_w = info.xres;
	lcd_h = info.yres;
	//映射LCD到内存
	lcdptr = (int *)mmap(NULL, lcd_w*lcd_h*4,PROT_READ | PROT_WRITE, MAP_SHARED, lcdfd, 0);
	if (lcdptr == NULL) 
	{
		perror("lcd mmap failed:");
	}
 
	//清空LCD屏幕并填充白色背景
	memset(lcdptr, 0xFF, lcd_w*lcd_h*4);
	return 0;
}
 //将uyvy格式的数据转换为RGB格式
void uyvy_to_rgb(unsigned char *yuyvdata, unsigned char * rgbdata, int w, int h)
{
	int r1, g1, b1;
	int r2, g2, b2;
	for (int i = 0; i < w*h/2; i++) 
	{
		char data[4];
		memcpy(data, yuyvdata + i*4, 4);
		unsigned char  V1= data[0];		// 设置的输出是
		unsigned char Y0 = data[1];
		unsigned char U0 = data[2];
		unsigned char Y1 = data[3];
		r1 = Y0 + 1.4075*(V1 - 128);
		if (r1 > 255)
			r1 = 255;
		if (r1 < 0)
			r1 = 0;
		g1 = Y0 - 0.3455*(U0 - 128) - 0.7169*(V1 - 128);
		if (g1 > 255)
			g1 = 255;
		if (g1 < 0)
			g1 = 0;
		b1 = Y0 + 1.779*(U0 - 128);
		if (b1 > 255)
			b1 = 255;
		if (b1 < 0)
			b1 = 0;
 
		r2 = Y1 + 1.4075*(V1 - 128);
		if (r2 > 255)
			r2 = 255;
		if (r2 < 0)
			r2 = 0;
		g2 = Y1 - 0.3455*(U0 - 128) - 0.7169*(V1 - 128);
		if (g2 > 255)
			g2 = 255;
		if (g2 < 0)
			g2 = 0;
		b2 = Y1 + 1.779*(U0 - 128);
		if (b2 > 255)
			b2 = 255;
		if (b2 < 0)
			b2 = 0;
 
		rgbdata[i*6 + 0] = r1;
		rgbdata[i*6 + 1] = g1;
		rgbdata[i*6 + 2] = b1;
		rgbdata[i*6 + 3] = r2;
		rgbdata[i*6 + 4] = g2;
		rgbdata[i*6 + 5] = b2;
	}
}
//将YUYV格式的数据转换为RGB格式
void yuyv_to_rgb(unsigned char *yuyvdata, unsigned char * rgbdata, int w, int h)
{
	int r1, g1, b1;
	int r2, g2, b2;
	for (int i = 0; i < w*h/2; i++) 
	{
		char data[4];
		memcpy(data, yuyvdata + i*4, 4);
		unsigned char Y0 = data[0];
		unsigned char U0 = data[1];
		unsigned char Y1 = data[2];
		unsigned char V1 = data[3];
		r1 = Y0 + 1.4075*(V1 - 128);
		if (r1 > 255)
			r1 = 255;
		if (r1 < 0)
			r1 = 0;
		g1 = Y0 - 0.3455*(U0 - 128) - 0.7169*(V1 - 128);
		if (g1 > 255)
			g1 = 255;
		if (g1 < 0)
			g1 = 0;
		b1 = Y0 + 1.779*(U0 - 128);
		if (b1 > 255)
			b1 = 255;
		if (b1 < 0)
			b1 = 0;
 
		r2 = Y1 + 1.4075*(V1 - 128);
		if (r2 > 255)
			r2 = 255;
		if (r2 < 0)
			r2 = 0;
		g2 = Y1 - 0.3455*(U0 - 128) - 0.7169*(V1 - 128);
		if (g2 > 255)
			g2 = 255;
		if (g2 < 0)
			g2 = 0;
		b2 = Y1 + 1.779*(U0 - 128);
		if (b2 > 255)
			b2 = 255;
		if (b2 < 0)
			b2 = 0;
 
		rgbdata[i*6 + 0] = r1;
		rgbdata[i*6 + 1] = g1;
		rgbdata[i*6 + 2] = b1;
		rgbdata[i*6 + 3] = r2;
		rgbdata[i*6 + 4] = g2;
		rgbdata[i*6 + 5] = b2;
	}
}
 
 
 
void lcd_show_rgb(unsigned char *rgbdata, int w, int h)
{
	unsigned int *ptr = lcdptr;
	for (int i = 0; i < h; i++) 
	{
		for (int j = 0; j < w; j++) 
		{
			memcpy(ptr + j, rgbdata + j*3, 3);
		}
		ptr += lcd_w;//偏移一行
		rgbdata += w*3;//偏移一行
	}
}
 
int main(int argc,char *argv[])
{
    int ret,i;
	int fd;
    unsigned short *base;
    unsigned short *start;
    int min_w, min_h;
    int j;
 
 
    fb_dev_init();
     /* 步骤一，打开视频设备 */
	printf("setup1: open device:%s \n", argv[1]); 
    fd = open(argv[1], O_RDWR);
    if (fd < 0) 
	{
        printf("file open error\n");
        return -1;
    }
    /* 步骤二，查询设备能力 */
	//查询设备的基本信息
	printf("setup2: VIDIOC_QUERYCAP"); 
	struct v4l2_capability cap; // 定义一个v4l2_capability结构体的变量cap
	// 使用ioctl函数发送VIDIOC_QUERYCAP命令来获取视频设备的基本信息，并将结果保存到cap变量中 
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) 
	{ 
		perror("VIDIOC_QUERYCAP"); 
		return -1; 
	} 
	//查看支持的图像格式、分辨率、帧率
	printf(" VIDIOC_ENUM_FMT\n"); 
	struct v4l2_fmtdesc fmtdesc = {0};//定义支持的像素格式结构体
	struct v4l2_frmsizeenum frmsize = {0};//定义支持的分辨率结构体
	struct v4l2_frmivalenum frmival = {0};//定义支持的帧率结构体
	fmtdesc.type = CAPTURE_TYPE;//设置视频采集类型为 V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
	fmtdesc.index = 0;
	while (!ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))//获取支持的像素格式
	{
		strcpy(cam_fmts[fmtdesc.index].description, fmtdesc.description);
		cam_fmts[fmtdesc.index].pixelformat = fmtdesc.pixelformat;
		fmtdesc.index++;
		printf("fmtdesc.description:%s, fmtdesc.pixelformat:%d\n", fmtdesc.description, fmtdesc.pixelformat); 
	}
	frmsize.type = CAPTURE_TYPE;
	frmival.type = CAPTURE_TYPE;
	for(i=0;i<fmtdesc.index;i++)//枚举每一种像素格式
	{
		printf("description:%s\npixelformat:0x%x\n", cam_fmts[i].description,cam_fmts[i].pixelformat );
		frmsize.index = 0;
		frmsize.pixel_format = cam_fmts[i].pixelformat;
		frmival.pixel_format = cam_fmts[i].pixelformat;
		// 2.枚举出摄像头所支持的所有视频采集分辨率
		while (0 == ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) 
		{
			printf("size<%d*%d> ",frmsize.discrete.width,frmsize.discrete.height);
			frmsize.index++;
			frmival.index = 0;
			frmival.width = frmsize.discrete.width;
			frmival.height = frmsize.discrete.height;
			// 3. 获取摄像头视频采集帧率
			while (0 == ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival)) 
			{
				printf("<%dfps>", frmival.discrete.denominator/frmival.discrete.numerator);
				frmival.index++;
			}
			printf("\n");
		}
		printf("\n");
	}
    /*步骤三，设置采集参数，视频帧宽度、高度、格式、视频帧率等信息*/
	printf("setup3: VIDIOC_S_FMT\n"); 
    struct v4l2_format fmt = {0};
    fmt.type = CAPTURE_TYPE;//type 类型
    fmt.fmt.pix.width = video_width; //设置视频帧宽度
    fmt.fmt.pix.height = video_height;//设置视频帧高度
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY; //设置像素格式 
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) 
	{
        printf("ioctl error: VIDIOC_S_FMT\n");
        return -1;
    }
 
    struct v4l2_streamparm streamparm = {0};
    streamparm.type = CAPTURE_TYPE;
    ioctl(fd, VIDIOC_G_PARM, &streamparm);
    if (V4L2_CAP_TIMEPERFRAME & streamparm.parm.capture.capability) 
	{
        streamparm.parm.capture.timeperframe.numerator = 1;
        streamparm.parm.capture.timeperframe.denominator = 30;//30fps
        if (0 > ioctl(fd, VIDIOC_S_PARM, &streamparm)) 
		{
            printf("ioctl error: VIDIOC_S_PARM");
            return -1;
        }
    }
    /*步骤四，请求帧缓冲*/
	printf("setup4: VIDIOC_REQBUFS\n"); 
	struct v4l2_requestbuffers reqbuffer;
	reqbuffer.type = CAPTURE_TYPE;
	reqbuffer.count = 4;//缓存数量
	reqbuffer.memory = V4L2_MEMORY_MMAP;//映射方式
	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuffer);
	if (ret < 0) 
	{
		printf("Request Queue space failed \n");
		return -1;
	}
	/*步骤五，映射帧缓冲*/
	printf("setup5: VIDIOC_QUERYBUF and mmap\n"); 
	struct v4l2_buffer mapbuffer;
	struct v4l2_plane planes[FMT_NUM_PLANES];
	 memset(&planes, 0, sizeof(planes));
	unsigned char *mptr[4];
	unsigned int size[4];//存储大小，方便释放
	mapbuffer.type = CAPTURE_TYPE;
	if(mapbuffer.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
		mapbuffer.m.planes = planes;
    	mapbuffer.length = FMT_NUM_PLANES;
	}
	for (i = 0; i < 4; i++) 
	{
		mapbuffer.index = i;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &mapbuffer);
		if (ret < 0) 
		{
			printf("Kernel space queue failed\n");
			return -1;
		}
		if(mapbuffer.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE){
			mptr[i] = (unsigned char *)mmap(NULL, mapbuffer.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mapbuffer.m.planes[0].m.mem_offset);
			size[i] = mapbuffer.m.planes[0].length;	
		}
		else{
			mptr[i] = (unsigned char *)mmap(NULL, mapbuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mapbuffer.m.offset);
			size[i] = mapbuffer.length;	
		}

		//使用完毕,入队
		ret = ioctl(fd, VIDIOC_QBUF, &mapbuffer);
		if (ret < 0) 
		{
			printf("ioctl error: VIDIOC_QBUF \n");
			return -1;
		}
	}
 
	/*步骤六，开启视频采集*/
	printf("setup6: VIDIOC_STREAMON\n"); 
	int type = CAPTURE_TYPE;
	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (ret < 0)
	{
		printf("ioctl error: VIDIOC_STREAMON \n");
		return -1;
	}
	//步骤七，读取数据、对数据进行处理
	unsigned char rgbdata[video_width*video_height*3];
	while (1) 
	{
		struct v4l2_buffer readbuffer;
		//出队列
		readbuffer.type = CAPTURE_TYPE;
		if (V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE == readbuffer.type) {
            readbuffer.m.planes = planes;
            readbuffer.length = FMT_NUM_PLANES;
        }
		ret = ioctl(fd, VIDIOC_DQBUF, &readbuffer);
		if (ret < 0) 
		{
			printf("ioctl error:VIDIOC_DQBUF \n");
		}
		//显示在LCD上
		uyvy_to_rgb(mptr[readbuffer.index], rgbdata, video_width, video_height);
		// yuyv_to_rgb(mptr[readbuffer.index], rgbdata, video_width, video_height);
		lcd_show_rgb(rgbdata, video_width, video_height);
		//通知内核已经使用完毕，入队列
		ret = ioctl(fd, VIDIOC_QBUF, &readbuffer);
		if (ret < 0) 
		{
			printf("ioctl error:VIDIOC_QBUF \n");
		}
	}
	//步骤八，停止视频采集和释放资源
	ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if (ret < 0) 
	{
		printf("ioctl error:VIDIOC_STREAMOFF \n");
		return -1;
	}
 
	//释放映射空间
	for (i = 0; i < 4; i++) 
	{
		munmap(mptr[i], size[i]);
	}
	close(fd);
	return 0;
}
