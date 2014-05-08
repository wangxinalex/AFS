/*
 * =====================================================================================
 *
 *       Filename:  encrypt.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/08/14 10:21:50
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangxinalex (), wangxinalex@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include "md5.h"
#include "encrypt.h"
using namespace std;
string encrypt(string text, string key){
	MD5 md5;
	md5.reset();
	md5.update(key);
	string sec_key = md5.toString().substr(0,KEY_LEN);
	for(size_t i = 0; i < text.size();i++){
		text[i]+=sec_key[i%KEY_LEN];
	}
	return text;
}
string decrypt(string text, string key){
	MD5 md5;
	md5.reset();
	md5.update(key);
	string sec_key = md5.toString().substr(0,KEY_LEN);
	for(size_t i = 0; i < text.size();i++){
		text[i]-=sec_key[i%KEY_LEN];
	}
	return text;

}
/*  
int main(){
	string s("open nfsjia 1");
	string encrypted=encrypt(s,ENCRYPT_KEY);
	cout << encrypted<<endl;
	string plain = decrypt(encrypted,ENCRYPT_KEY);
	cout << plain<<endl;
	return 0;
}*/
