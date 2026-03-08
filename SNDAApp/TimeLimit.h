#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <regex>
#define N  999 //精度为小数点后面3位

using std::cout; using std::endl;

//匹配串
std::string regex_str("\\d+$");
std::regex IntNumber(regex_str, std::regex::icase);

int TimeAll = 90;
std::string keySTR = "JUIxJUQ4JURGJTkyViU5RiU5NkglREElQjElRjhjJTkyOLSFAEDHOILSNFOIASNFDOAdsaASODNSIOANcUHIiBIUOJNSADLKFDNLKAASFDLKNVCSADFbhjBJKBOUPLNKJP";	
int len = 39;

struct dtime
{
	int year;
	int month;
	int day;
};

class TimeLimit
{
public:
	//写key文件
	void writeFirstTime()
	{
		//获取第一次运行的时间
		SYSTEMTIME sys;
		GetLocalTime(&sys);

		//对第一次运行时间加密
		srand((int)time(0));		
		char call[50];
		for (int i = 0; i < len; i++)
		{
			int x;											//x表示这个字符的ascii码 ，s表示这个字符的大小写  
			double s = rand() % (N + 1) / (float)(N + 1);;  //随机使s为1或0，为1就是大写，为0就是小写 
			if (s > 0.2)          
				x = rand() % ('Z' - 'A' + 1) + 'A';			//将x赋为大写字母的ascii码 
			else
				x = rand() % ('z' - 'a' + 1) + 'a';			//如果s=0，x赋为小写字母的ascii码 
			
			call[i] = char(x);
		}
		std::string addSTR(call, call + len);
		int norm = 123999;
		std::string oldstr = std::to_string(rand()) +","+ std::to_string(sys.wYear + norm)+"," +std::to_string(rand()) + ","+ std::to_string(sys.wMonth + norm)+ ","+ std::to_string(rand()) + "," + std::to_string(sys.wDay + norm)+","+std::to_string(rand());
		std::string newstr = addSTR + encode_base64(oldstr, false);

		std::ofstream out;
		out.open("NRA.pdb");
		out << newstr;

		out.close();
	}

	//判断是否过期
	bool isOutTime()
	{
		//探测是否已经存在key文件，即是否第一次运行
		std::fstream _file;
		_file.open("NRA.pdb", std::ios::in);
		if (!_file)
		{			
			return false;
		}
		else	//如果不是，则计算时间是否超过了90天
		{
			std::string str,temp;
			std::getline(_file, str);
			_file.close();

			if (str == keySTR)		//首次运行
			{
				writeFirstTime();
				return true;
			}
			else		//非首次运行
			{
				//解密
				str = str.substr(len, str.length());
				std::string newstr = decode_base64(str, false);
				//计算时间
				dtime dt_first, dt_now;

				//获取首次运行时间
				std::stringstream input(newstr);
				int num = 1;
				while (std::getline(input, temp, ','))
				{
					if (!(std::regex_match(temp, IntNumber)))
						return false;

					int norm = 123999;
					if (num == 2)
						dt_first.year = atoi(temp.c_str()) - norm;
					if (num == 4)
						dt_first.month = atoi(temp.c_str()) - norm;
					if (num == 6)
						dt_first.day = atoi(temp.c_str()) - norm;
					++num;

					if (num > 8)
						return false;
				}

				//获取当前时间
				SYSTEMTIME sys;
				GetLocalTime(&sys);
				dt_now.year = sys.wYear, dt_now.month = sys.wMonth, dt_now.day = sys.wDay;

				//计算已经运行的天数
				int old = 365 * dt_first.year + 30 * dt_first.month + dt_first.day;
				int now = 365 * dt_now.year + 30 * dt_now.month + dt_now.day;
				int delta = now - old;
				if (delta >= 0 && delta <= 90)
					return true;
			}						
		}
		
		return false;
	}

	std::string encode_base64(const std::string& d, bool base64url = false)
	{
		const char alphabet_base64[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
			"ghijklmn" "opqrstuv" "wxyz0123" "456789+/";
		const char alphabet_base64url[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
			"ghijklmn" "opqrstuv" "wxyz0123" "456789-_";
		const char *const alphabet = base64url ? alphabet_base64url : alphabet_base64;
		const char padchar = '=';
		int padlen = 0;

		std::string tmp;
		tmp.resize((d.size() + 2) / 3 * 4);

		int i = 0;
		char *out = &tmp[0];
		while (i < d.size()) {
			// encode 3 bytes at a time
			int chunk = 0;
			chunk |= int(uchar(d.data()[i++])) << 16;
			if (i == d.size()) {
				padlen = 2;
			}
			else {
				chunk |= int(uchar(d.data()[i++])) << 8;
				if (i == d.size())
					padlen = 1;
				else
					chunk |= int(uchar(d.data()[i++]));
			}

			int j = (chunk & 0x00fc0000) >> 18;
			int k = (chunk & 0x0003f000) >> 12;
			int l = (chunk & 0x00000fc0) >> 6;
			int m = (chunk & 0x0000003f);
			*out++ = alphabet[j];
			*out++ = alphabet[k];

			if (padlen > 1) {
				//if ((options & OmitTrailingEquals) == 0)
				*out++ = padchar;
			}
			else {
				*out++ = alphabet[l];
			}
			if (padlen > 0) {
				//if ((options & OmitTrailingEquals) == 0)
				*out++ = padchar;
			}
			else {
				*out++ = alphabet[m];
			}
		}
		//assert(/*(options & OmitTrailingEquals) ||*/ (out == tmp.size() + tmp.data()));
		//if (options & OmitTrailingEquals)
		//    tmp.truncate(out - tmp.data());
		return tmp;
	}

	std::string decode_base64(const std::string base64, bool base64url = false)
	{
		unsigned int buf = 0;
		int nbits = 0;
		std::string tmp;
		tmp.resize((base64.size() * 3) / 4);


		int offset = 0;
		for (int i = 0; i < base64.size(); ++i) {
			int ch = base64.at(i);
			int d;

			if (ch >= 'A' && ch <= 'Z')
				d = ch - 'A';
			else if (ch >= 'a' && ch <= 'z')
				d = ch - 'a' + 26;
			else if (ch >= '0' && ch <= '9')
				d = ch - '0' + 52;
			else if (ch == '+' && (base64url) == 0)
				d = 62;
			else if (ch == '-' && (base64url) != 0)
				d = 62;
			else if (ch == '/' && (base64url) == 0)
				d = 63;
			else if (ch == '_' && (base64url) != 0)
				d = 63;
			else
				d = -1;

			if (d != -1) {
				buf = (buf << 6) | d;
				nbits += 6;
				if (nbits >= 8) {
					nbits -= 8;
					tmp[offset++] = buf >> nbits;
					buf &= (1 << nbits) - 1;
				}
			}
		}

		//tmp.truncate(offset);
		if (offset < tmp.size())
			tmp.resize(offset);

		return tmp;
	}
};
