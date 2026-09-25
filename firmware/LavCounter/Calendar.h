#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
namespace lav {
struct Date {int year=2000,month=1,day=1,hour=0,minute=0,second=0;};
inline bool leap(int y){return y%4==0&&(y%100!=0||y%400==0);}
inline bool validDate(const Date& d){
 static const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
 return d.year>=2000&&d.year<=2099&&d.month>=1&&d.month<=12&&d.day>=1&&d.day<=days[d.month-1]+(d.month==2&&leap(d.year))&&d.hour>=0&&d.hour<24&&d.minute>=0&&d.minute<60&&d.second>=0&&d.second<60;
}
inline int bcd(uint8_t b){if((b&15)>9||(b>>4)>9)return -1;return (b>>4)*10+(b&15);}
inline uint8_t encodeBcd(int v){return (uint8_t)((v/10)*16+v%10);}
inline bool decodeRtc(const uint8_t* b,uint8_t status,Date& d){
 if(status&0x80)return false;
 if((b[0]&0x80)||(b[1]&0x80)||(b[2]&0x80)||(b[4]&0xc0)||(b[5]&0xe0))return false;
 d.second=bcd(b[0]);d.minute=bcd(b[1]);
 if(b[2]&0x40){int h=bcd(b[2]&0x1f);if(h<1||h>12)return false;d.hour=(h%12)+((b[2]&0x20)?12:0);}
 else d.hour=bcd(b[2]&0x3f);
 if(b[3]<1||b[3]>7)return false;
 d.day=bcd(b[4]);d.month=bcd(b[5]);d.year=2000+bcd(b[6]);
 return bcd(b[6])>=0&&validDate(d);
}
inline int64_t epoch(const Date& d){
 int64_t days=0;for(int y=1970;y<d.year;y++)days+=leap(y)?366:365;
 static const int md[]={31,28,31,30,31,30,31,31,30,31,30,31};
 for(int m=1;m<d.month;m++)days+=md[m-1]+(m==2&&leap(d.year));
 return ((days+d.day-1)*24+d.hour)*3600+d.minute*60+d.second;
}
inline Date fromEpoch(int64_t e){
 Date d;int64_t days=e/86400;int sec=(int)(e%86400);d.hour=sec/3600;d.minute=(sec%3600)/60;d.second=sec%60;
 d.year=1970;while(days>=(leap(d.year)?366:365)){days-=leap(d.year)?366:365;d.year++;}
 static const int md[]={31,28,31,30,31,30,31,31,30,31,30,31};
 d.month=1;while(days>=md[d.month-1]+(d.month==2&&leap(d.year))){days-=md[d.month-1]+(d.month==2&&leap(d.year));d.month++;}d.day=(int)days+1;return d;
}
inline void encodeRtc(const Date& d,uint8_t* b){b[0]=encodeBcd(d.second);b[1]=encodeBcd(d.minute);b[2]=encodeBcd(d.hour);b[3]=(uint8_t)((epoch(d)/86400+4)%7+1);b[4]=encodeBcd(d.day);b[5]=encodeBcd(d.month);b[6]=encodeBcd(d.year-2000);}
inline bool parseDate(const char* s,Date& d){
 size_t n=strlen(s);if(n!=16&&n!=19)return false;
 if(s[4]!='-'||s[7]!='-'||s[10]!='T'||s[13]!=':'||(n==19&&s[16]!=':'))return false;
 for(size_t i=0;i<n;i++)if(i!=4&&i!=7&&i!=10&&i!=13&&i!=16&&(s[i]<'0'||s[i]>'9'))return false;
 d.second=0;int count=sscanf(s,"%4d-%2d-%2dT%2d:%2d:%2d",&d.year,&d.month,&d.day,&d.hour,&d.minute,&d.second);return count>=5&&validDate(d);
}
inline void formatDate(int64_t e,char* out,size_t size){Date d=fromEpoch(e);snprintf(out,size,"%04d-%02d-%02dT%02d:%02d:%02d",d.year,d.month,d.day,d.hour,d.minute,d.second);}
}
