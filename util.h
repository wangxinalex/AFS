/*
 * =====================================================================================
 *
 *       Filename:  util.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/06/14 20:02:20
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  wangxinalex (), wangxinalex@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef  UTIL_H_INC
#define  UTIL_H_INC
#define CLIENT_QUIT 8
#define MAX_CLIENT 25
#define	MAX_BUFF 1024			/*  */
#define RWRWRW (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#define MAX_FILE 200
#define	MAX_RESPONSE 40			/*  */
#define	MAX_NAME 30			/*  */
#define FORMAT_ERR 1
#define SOCKET_ERR 2
#define FILE_ERR 3
#define LOCK_ERR 4
using namespace std;
const char* LOCK_MES  = "Locked";
const char* TRANS_FILE_START = "Transmission_Start";
const char* TRANS_FILE_START_ACK = "Transmission_Start_ACK";

#endif   /* ----- #ifndef UTIL_H_INC  ----- */
