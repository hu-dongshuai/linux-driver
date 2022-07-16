#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <sys/ioctl.h>

/*显示屏相关头文件*/
#include <linux/fb.h>
#include <sys/mman.h>

extern unsigned char F16x16[];
extern unsigned char F6x8[][6];
extern unsigned char F8x16[][16];
extern unsigned char BMP1[];

unsigned int test_picture[] = {0};
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

void lcd_show_pixel(unsigned char *fbmem, int line_length, int bpp,
            int x, int y, unsigned int color)
{
  unsigned char  *p8  = (unsigned char *)(fbmem + y * line_length + x * bpp / 8);
  unsigned short *p16 = (unsigned short *)p8;
  unsigned int   *p32 = (unsigned int *)p8;

  unsigned int red, green, blue;

  switch(bpp)
  {
    case 8:
    {
      *p8 = (unsigned char)(color & 0xf);
      break;
    }

    case 16:
    {
      red   = (color >> 16) & 0xff;
      green = (color >> 8) & 0xff;
      blue  = (color >> 0) & 0xff;
      *p16  = (unsigned short)(((red >> 3) << 11) | ((green >> 2) <<5) | ((blue >> 3) << 0));
      break;
    }
    case 32:
    {
      *p32 = color;
      break;
    }
    default:
    {
      printf("can not support %d bpp", bpp);
      break;
    }
  }
}
int oled_show_one_letter( unsigned char *fbp,  unsigned char x, unsigned char y, int size, unsigned char *data)
{
    int  i,b, offset=0;
    unsigned char byte;
    for (i = 0; i < size; i++)
    {
        if (i>=8)
            offset = 8;
        byte = data[i];
        for (b = 7; b >= 0; b--)
        {
            if (byte & (1<<b))
            {
                /* show */
                lcd_show_pixel(fbp, finfo.line_length, vinfo.bits_per_pixel,  y+i%8, x+b+offset,0xff);
            }
            else
            {
                /* hide */
                lcd_show_pixel(fbp, finfo.line_length, vinfo.bits_per_pixel,  y+i%8, x+b+offset, 0x00);
            }
        }
    }
}

int main()
{
  unsigned int *temp;
  unsigned int color;
  unsigned char *fbp = 0;
  int fp = 0;
  int x, y, i, j, k;

  fp = open("/dev/fb0", O_RDWR);
  if (fp < 0)
  {
      printf("Error : Can not open framebuffer device/n");
      exit(1);
  }

  if (ioctl(fp, FBIOGET_FSCREENINFO, &finfo))
  {
      printf("Error reading fixed information/n");
      exit(2);
  }

  if (ioctl(fp, FBIOGET_VSCREENINFO, &vinfo))
  {
      printf("Error reading variable information/n");
      exit(3);
  }
  printf("The mem is :%d\n", finfo.smem_len);
  printf("The line_length is :%d\n", finfo.line_length);
  printf("The xres is :%d\n", vinfo.xres);
  printf("The yres is :%d\n", vinfo.yres);
  printf("bits_per_pixel is :%d\n", vinfo.bits_per_pixel);

   /*这就是把fp所指的文件中从开始到screensize大小的内容给映射出来，得到一个指向这块空间的指针*/
  fbp =(unsigned char *) mmap (0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fp,0);
  if ((int) fbp == -1)
  {
     printf ("Error: failed to map framebuffer device to memory./n");
     exit (4);
  }

  for (i=0; i<10000; i++)
  {
        x=5;y=10;
        for(j=0; j<10; j++)
        {
            
            for (k=0; k<3; k++)
                oled_show_one_letter(fbp, x+k*16, y+10*j, 16, F8x16[j+k*10]);
        }

       // usleep(20);
          x=5;y=10;
        for(j=0; j<10; j++)
        {
            
            for (k=0; k<3; k++)
                oled_show_one_letter(fbp, x+k*16, y+10*j, 16, F8x16[30+j+k*10]);
        }
        //usleep(20);
        
  }


  munmap (fbp, finfo.smem_len); /*解除映射*/
  close(fp);
  /*显示图片*/

  /*
  temp = (unsigned int *)test_picture;
  for (y = 0; y < vinfo.yres; y++)
  {
    for(x = 0; x < vinfo.xres; x++)
    {
      lcd_show_pixel(fbp, finfo.line_length, vinfo.bits_per_pixel, x, y, *temp);
      temp++;
    }
  }
  */


  return 0;
}