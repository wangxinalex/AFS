/*
 * =====================================================================================
 *
 *       Filename:  encrypt.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/08/14 10:35:04
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangxinalex (), wangxinalex@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef ENCRYPT_H_INC
#define ENCRYPT_H_INC
#define KEY_LEN 10
#define ENCRYPT_KEY "wangxinalex@gmail.com"
using namespace std;
string encrypt(string text, string key);
string decrypt(string text, string key);
#endif   /* ----- #ifndef ENCRYPT_H_INC  ----- */
