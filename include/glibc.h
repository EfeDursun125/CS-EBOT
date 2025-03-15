// objdump -T ebot.so | grep GLIBC_
// to see glibc versions if its correct
#ifndef WIN32
#define _FORTIFY_SOURCE 0
#define D__NO_ISOC23__
__asm__(".symver fputs,fputs@GLIBC_2.0");
__asm__(".symver abort,abort@GLIBC_2.0");
__asm__(".symver sprintf,sprintf@GLIBC_2.0");
__asm__(".symver mkdir,mkdir@GLIBC_2.0");
__asm__(".symver memcmp,memcmp@GLIBC_2.0");
__asm__(".symver realloc,realloc@GLIBC_2.0");
__asm__(".symver vsprintf,vsprintf@GLIBC_2.0");
__asm__(".symver localtime,localtime@GLIBC_2.0");
__asm__(".symver strchr,strchr@GLIBC_2.0");
__asm__(".symver vsnprintf,vsnprintf@GLIBC_2.0");
__asm__(".symver system,system@GLIBC_2.0");
__asm__(".symver fgets,fgets@GLIBC_2.0");
__asm__(".symver gettext,gettext@GLIBC_2.0");
__asm__(".symver strerror_r,strerror_r@GLIBC_2.0");
__asm__(".symver free,free@GLIBC_2.0");
__asm__(".symver fseek,fseek@GLIBC_2.0");
__asm__(".symver fclose,fclose@GLIBC_2.0");
__asm__(".symver stderr,stderr@GLIBC_2.0");
__asm__(".symver fopen,fopen@GLIBC_2.0");
__asm__(".symver unlink,unlink@GLIBC_2.0");
__asm__(".symver fgetc,fgetc@GLIBC_2.0");
__asm__(".symver ftell,ftell@GLIBC_2.0");
__asm__(".symver printf,printf@GLIBC_2.0");
__asm__(".symver fwrite,fwrite@GLIBC_2.0");
__asm__(".symver time,time@GLIBC_2.0");
__asm__(".symver malloc,malloc@GLIBC_2.0");
__asm__(".symver fputc,fputc@GLIBC_2.0");
__asm__(".symver vfprintf,vfprintf@GLIBC_2.0");
__asm__(".symver fread,fread@GLIBC_2.0");
__asm__(".symver strftime,strftime@GLIBC_2.0");
__asm__(".symver snprintf,snprintf@GLIBC_2.0");
#endif
